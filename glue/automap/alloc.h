// Automap allocation facade, allocation contract 1.0.0.
// SPDX-License-Identifier: MIT-0
//
// Every library's allocator hooks route through this facade, tagged with
// the library's id, so the composition has one allocator and probes can see
// who allocated what. The allocation choice in the choices header picks
// what backs it: the system allocator, or the library in the allocator
// slot (whose binding defines the am_allocator_slot_* functions).
#pragma once
#include <cstddef>
#include <cstdint>

namespace am::alloc {

void* allocate(const char* library, std::size_t size);
void* allocate_zeroed(const char* library, std::size_t count, std::size_t size);
void* reallocate(const char* library, void* block, std::size_t size);
void release(const char* library, void* block);

void* allocate_aligned(const char* library, std::size_t size, std::size_t alignment);
void release_aligned(const char* library, void* block);

// Allocations the facade has served for a library.
std::uint64_t routed_count(const char* library);

// Probe builds only: whether a library released or reallocated a block the
// facade never allocated, which means its hooks were installed after it
// first allocated. Always false outside probe builds.
bool released_foreign_block(const char* library);

} // namespace am::alloc
