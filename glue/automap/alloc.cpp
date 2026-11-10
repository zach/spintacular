// Automap allocation facade, allocation contract 1.0.0.
// SPDX-License-Identifier: MIT-0
#include "automap/alloc.h"

#include <cstdlib>
#include <cstring>
#include <map>
#include <mutex>
#include <set>
#include <string>

#include "automap/choices.h"

#if defined(AUTOMAP_ALLOCATION_ALLOCATOR_ALLOCATOR_SLOT)
// Defined by the allocator slot's binding.
void* am_allocator_slot_allocate(std::size_t size, std::size_t alignment);
void* am_allocator_slot_reallocate(void* block, std::size_t size);
void am_allocator_slot_release(void* block);
#endif

namespace am::alloc {
namespace {

struct State {
    std::mutex mutex;
    std::map<std::string, std::uint64_t> routed;
    std::set<std::string> foreign;
#ifdef AUTOMAP_PROBE
    std::set<void*> live;
#endif
};

// Never destroyed: libraries may release blocks while the process exits.
State& state() {
    static State* s = new State;
    return *s;
}

void served(const char* library, void* block) {
    State& s = state();
    std::lock_guard<std::mutex> lock(s.mutex);
    ++s.routed[library];
#ifdef AUTOMAP_PROBE
    if (block != nullptr) s.live.insert(block);
#else
    (void)block;
#endif
}

// Records that library is giving back block. In probe builds, a block the
// facade never served is noted against the library and reported foreign:
// it belongs to some other allocator, which the facade's would reject (and
// an allocator such as mimalloc stops the process on), so the probe run
// leaks it and lives to report the rule.
bool returned(const char* library, void* block) {
#ifdef AUTOMAP_PROBE
    State& s = state();
    std::lock_guard<std::mutex> lock(s.mutex);
    if (s.live.erase(block) != 0) return false;
    s.foreign.insert(library);
    return true;
#else
    (void)library;
    (void)block;
    return false;
#endif
}

// backend_allocate serves size bytes; an aligned request always takes the
// aligned path, so release_aligned can match it.
void* backend_allocate(std::size_t size, std::size_t alignment, bool aligned) {
    if (size == 0) size = 1;
#if defined(AUTOMAP_ALLOCATION_ALLOCATOR_ALLOCATOR_SLOT)
    return am_allocator_slot_allocate(size, aligned ? alignment : alignof(std::max_align_t));
#else
    if (!aligned) return std::malloc(size);
    if (alignment < sizeof(void*)) alignment = sizeof(void*);
#if defined(_WIN32)
    return _aligned_malloc(size, alignment);
#else
    void* block = nullptr;
    return posix_memalign(&block, alignment, size) == 0 ? block : nullptr;
#endif
#endif
}

void backend_release(void* block, bool aligned) {
#if defined(AUTOMAP_ALLOCATION_ALLOCATOR_ALLOCATOR_SLOT)
    (void)aligned;
    am_allocator_slot_release(block);
#elif defined(_WIN32)
    if (aligned) _aligned_free(block);
    else std::free(block);
#else
    (void)aligned;
    std::free(block);
#endif
}

} // namespace

void* allocate(const char* library, std::size_t size) {
    void* block = backend_allocate(size, alignof(std::max_align_t), false);
    served(library, block);
    return block;
}

void* allocate_zeroed(const char* library, std::size_t count, std::size_t size) {
    void* block = allocate(library, count * size);
    if (block != nullptr) std::memset(block, 0, count * size);
    return block;
}

void* reallocate(const char* library, void* block, std::size_t size) {
    if (block == nullptr) return allocate(library, size);
    if (returned(library, block)) return allocate(library, size); // a foreign block: its contents are unknown
#if defined(AUTOMAP_ALLOCATION_ALLOCATOR_ALLOCATOR_SLOT)
    void* moved = am_allocator_slot_reallocate(block, size);
#else
    void* moved = std::realloc(block, size == 0 ? 1 : size);
#endif
    served(library, moved);
    return moved;
}

void release(const char* library, void* block) {
    if (block == nullptr) return;
    if (returned(library, block)) return;
    backend_release(block, false);
}

void* allocate_aligned(const char* library, std::size_t size, std::size_t alignment) {
    void* block = backend_allocate(size, alignment, true);
    served(library, block);
    return block;
}

void release_aligned(const char* library, void* block) {
    if (block == nullptr) return;
    if (returned(library, block)) return;
    backend_release(block, true);
}

std::uint64_t routed_count(const char* library) {
    State& s = state();
    std::lock_guard<std::mutex> lock(s.mutex);
    auto it = s.routed.find(library);
    return it == s.routed.end() ? 0 : it->second;
}

bool released_foreign_block(const char* library) {
    State& s = state();
    std::lock_guard<std::mutex> lock(s.mutex);
    return s.foreign.count(library) != 0;
}

} // namespace am::alloc
