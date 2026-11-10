// Probes for miniaudio's binding to the diagnostics contract.
// SPDX-License-Identifier: MIT-0
#ifdef AUTOMAP_PROBE
#include "automap/diag.h"
#include "automap/probe.h"
#include "miniaudio_setup.h"

namespace {

// diagnostics.routed-through-facade: the context writes to the binding's
// log, and a message posted to that log reaches the facade.
void am_probe_miniaudio_diagnostics(am::Context&) {
    ma_context* context = ma_engine_get_device(&am_miniaudio_engine())->pContext;
    auto before = am::diag::message_count("miniaudio");
    ma_log_post(ma_context_get_log(context), MA_LOG_LEVEL_INFO, "automap probe: diagnostics routing");
    bool routed = ma_context_get_log(context) == &am_miniaudio_log() && am::diag::message_count("miniaudio") > before;
    am::probe::check("diagnostics.routed-through-facade", "miniaudio", routed, "DIAG_NOT_ROUTED(miniaudio)");
}
const am::probe::Registration registration("miniaudio--diagnostics", &am_probe_miniaudio_diagnostics);

} // namespace
#endif
