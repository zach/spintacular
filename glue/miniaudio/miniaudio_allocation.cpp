// miniaudio's binding to the allocation contract.
// SPDX-License-Identifier: MIT-0
#include "automap/alloc.h"
#include "automap/context.h"
#include "automap/lifecycle.h"
#include "miniaudio_setup.h"

void* am_miniaudio_malloc(size_t size, void*) { return am::alloc::allocate("miniaudio", size); }

void* am_miniaudio_realloc(void* block, size_t size, void*) { return am::alloc::reallocate("miniaudio", block, size); }

void am_miniaudio_free(void* block, void*) { am::alloc::release("miniaudio", block); }

// Routes miniaudio's allocations through the allocation facade. Runs in the
// memory phase, before the log, context and engine copy the callbacks.
bool am_bind_miniaudio_allocation(am::Context&) {
    ma_allocation_callbacks& callbacks = am_miniaudio_allocation();
    callbacks.onMalloc = &am_miniaudio_malloc;
    callbacks.onRealloc = &am_miniaudio_realloc;
    callbacks.onFree = &am_miniaudio_free;
    am::lifecycle::init_step("miniaudio:allocationCallbacks");
    return true;
}
