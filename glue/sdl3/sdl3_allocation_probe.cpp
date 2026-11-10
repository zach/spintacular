// Probes for SDL3's binding to the allocation contract.
// SPDX-License-Identifier: MIT-0
#ifdef AUTOMAP_PROBE
#include <SDL3/SDL.h>

#include "automap/alloc.h"
#include "automap/probe.h"

void* SDLCALL am_sdl3_malloc(size_t size);
void* SDLCALL am_sdl3_calloc(size_t count, size_t size);
void* SDLCALL am_sdl3_realloc(void* block, size_t size);
void SDLCALL am_sdl3_free(void* block);

namespace {

// allocation.routed-through-facade: SDL's installed hooks are the facade's
// trampolines, and the facade served SDL during the run.
void am_probe_sdl3_allocation(am::Context&) {
    SDL_malloc_func m = nullptr;
    SDL_calloc_func c = nullptr;
    SDL_realloc_func r = nullptr;
    SDL_free_func f = nullptr;
    SDL_GetMemoryFunctions(&m, &c, &r, &f);
    bool routed = m == am_sdl3_malloc && c == am_sdl3_calloc && r == am_sdl3_realloc && f == am_sdl3_free &&
                  am::alloc::routed_count("sdl3") > 0;
    am::probe::check("allocation.routed-through-facade", "sdl3", routed, "ALLOC_NOT_ROUTED(sdl3)");
}
const am::probe::Registration routed("sdl3--allocation", &am_probe_sdl3_allocation);

// allocation.hooks-before-first-allocation: after SDL_Quit, SDL has released
// nothing the facade didn't allocate.
void am_probe_sdl3_hooks_early(am::Context&) {
    bool early = !am::alloc::released_foreign_block("sdl3");
    am::probe::check("allocation.hooks-before-first-allocation", "sdl3", early, "ALLOC_HOOK_LATE(sdl3)");
}
const am::probe::Registration early("sdl3--allocation", &am_probe_sdl3_hooks_early, am::probe::When::after_shutdown);

} // namespace
#endif
