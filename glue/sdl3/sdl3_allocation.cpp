// SDL3's binding to the allocation contract.
// SPDX-License-Identifier: MIT-0
#include "sdl3_allocation.h"

#include <SDL3/SDL.h>

#include "automap/alloc.h"
#include "automap/lifecycle.h"

// SDL's hooks carry no user data, so one set of trampolines tags every block
// as SDL's. The probe compares SDL's installed hooks with these.
void* SDLCALL am_sdl3_malloc(size_t size) { return am::alloc::allocate("sdl3", size); }
void* SDLCALL am_sdl3_calloc(size_t count, size_t size) { return am::alloc::allocate_zeroed("sdl3", count, size); }
void* SDLCALL am_sdl3_realloc(void* block, size_t size) { return am::alloc::reallocate("sdl3", block, size); }
void SDLCALL am_sdl3_free(void* block) { am::alloc::release("sdl3", block); }

bool am_bind_sdl3_allocation(am::Context&) {
    if (!SDL_SetMemoryFunctions(am_sdl3_malloc, am_sdl3_calloc, am_sdl3_realloc, am_sdl3_free)) return false;
    am::lifecycle::init_step("sdl3:SDL_SetMemoryFunctions");
    return true;
}
