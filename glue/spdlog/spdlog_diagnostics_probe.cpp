// Probes for spdlog's binding to the diagnostics contract.
// SPDX-License-Identifier: MIT-0
#ifdef AUTOMAP_PROBE
#include <atomic>
#include <memory>
#include <string>

#include <spdlog/sinks/callback_sink.h>
#include <spdlog/sinks/dist_sink.h>
#include <spdlog/spdlog.h>

#include "automap/diag.h"
#include "automap/probe.h"

spdlog::logger& am_spdlog_logger();

namespace {

// diagnostics.routed-through-facade: a message logged through the facade
// reaches the sinks of spdlog's logger.
void am_probe_spdlog_diagnostics(am::Context&) {
    std::atomic<bool> seen{false};
    auto sinks = std::static_pointer_cast<spdlog::sinks::dist_sink_mt>(am_spdlog_logger().sinks().front());
    auto watch = std::make_shared<spdlog::sinks::callback_sink_mt>([&](const spdlog::details::log_msg& msg) {
        if (std::string(msg.payload.data(), msg.payload.size()).find("spdlog-probe: routing") != std::string::npos) seen = true;
    });
    sinks->add_sink(watch);
    am::diag::log(am::diag::Level::info, "spdlog-probe", "routing");
    sinks->remove_sink(watch);
    am::probe::check("diagnostics.routed-through-facade", "spdlog", seen.load(), "DIAG_NOT_ROUTED(spdlog)");
}
const am::probe::Registration registration("spdlog--diagnostics", &am_probe_spdlog_diagnostics);

} // namespace
#endif
