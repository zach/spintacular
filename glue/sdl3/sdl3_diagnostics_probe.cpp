// Probes for SDL3's binding to the diagnostics contract.
// SPDX-License-Identifier: MIT-0
#ifdef AUTOMAP_PROBE
#include <SDL3/SDL.h>

#include "automap/diag.h"
#include "automap/probe.h"

namespace {

// diagnostics.routed-through-facade: a message SDL logs reaches the facade.
void am_probe_sdl3_diagnostics(am::Context&) {
    auto before = am::diag::message_count("sdl3");
    SDL_Log("automap probe: diagnostics routing");
    bool routed = am::diag::message_count("sdl3") > before;
    am::probe::check("diagnostics.routed-through-facade", "sdl3", routed, "DIAG_NOT_ROUTED(sdl3)");
}
const am::probe::Registration registration("sdl3--diagnostics", &am_probe_sdl3_diagnostics);

} // namespace
#endif
