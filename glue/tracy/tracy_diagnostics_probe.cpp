// Probes for Tracy's binding to the diagnostics contract.
// SPDX-License-Identifier: MIT-0
#ifdef AUTOMAP_PROBE
#include <cstddef>

#include <tracy/Tracy.hpp>

#include "automap/diag.h"
#include "automap/probe.h"

namespace {

// diagnostics.routed-through-facade: a zone opened through the facade
// reaches the Tracy client. With no profiler connected the client holds what
// it records, so the zone's begin and end events wait in this thread's
// queue.
void am_probe_tracy_diagnostics(am::Context&) {
    std::size_t before = tracy::GetToken()->size_approx();
    { am::diag::Zone zone("automap probe: profiling routing"); }
    bool routed = TracyIsStarted && tracy::GetToken()->size_approx() >= before + 2;
    am::probe::check("diagnostics.routed-through-facade", "tracy", routed, "DIAG_NOT_ROUTED(tracy)");
}
const am::probe::Registration registration("tracy--diagnostics", &am_probe_tracy_diagnostics);

} // namespace
#endif
