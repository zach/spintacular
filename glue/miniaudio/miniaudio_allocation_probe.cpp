// Probes for miniaudio's binding to the allocation contract.
// SPDX-License-Identifier: MIT-0
#ifdef AUTOMAP_PROBE
#include "automap/alloc.h"
#include "automap/probe.h"
#include "miniaudio_setup.h"

void* am_miniaudio_malloc(size_t size, void*);
void am_miniaudio_free(void* block, void*);

namespace {

// allocation.routed-through-facade: the engine and its context hold the
// facade's callbacks, and the facade served miniaudio during the run.
void am_probe_miniaudio_allocation(am::Context&) {
    ma_engine& engine = am_miniaudio_engine();
    ma_context* context = ma_engine_get_device(&engine)->pContext;
    bool routed = engine.allocationCallbacks.onMalloc == &am_miniaudio_malloc &&
                  engine.allocationCallbacks.onFree == &am_miniaudio_free &&
                  context->allocationCallbacks.onMalloc == &am_miniaudio_malloc && am::alloc::routed_count("miniaudio") > 0;
    am::probe::check("allocation.routed-through-facade", "miniaudio", routed, "ALLOC_NOT_ROUTED(miniaudio)");
}
const am::probe::Registration routed("miniaudio--allocation", &am_probe_miniaudio_allocation);

// allocation.hooks-before-first-allocation: after the engine, context and
// log are gone, miniaudio has released nothing the facade didn't allocate.
void am_probe_miniaudio_hooks_early(am::Context&) {
    bool early = !am::alloc::released_foreign_block("miniaudio");
    am::probe::check("allocation.hooks-before-first-allocation", "miniaudio", early, "ALLOC_HOOK_LATE(miniaudio)");
}
const am::probe::Registration after_shutdown("miniaudio--allocation", &am_probe_miniaudio_hooks_early,
                                             am::probe::When::after_shutdown);

} // namespace
#endif
