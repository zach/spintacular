// spdlog's binding to the diagnostics contract: spdlog in the logging slot
// writes every message the diagnostics facade receives.
// SPDX-License-Identifier: MIT-0
#include <memory>

#include <spdlog/sinks/dist_sink.h>
#include <spdlog/sinks/stdout_color_sinks.h>
#include <spdlog/spdlog.h>

#include "automap/context.h"
#include "automap/lifecycle.h"

// The logger the facade writes to, made the first time anything logs, which
// may be before the diagnostics phase. Its one sink distributes to the
// others, so sinks can be added while other threads log. Never destroyed:
// libraries may log while the process exits.
spdlog::logger& am_spdlog_logger() {
    static spdlog::logger* logger = [] {
        auto sinks = std::make_shared<spdlog::sinks::dist_sink_mt>();
        sinks->add_sink(std::make_shared<spdlog::sinks::stderr_color_sink_mt>());
        auto* made = new spdlog::logger("automap", sinks);
        made->set_pattern("[%l] %v");
        made->set_level(spdlog::level::trace);
        return made;
    }();
    return *logger;
}

void am_logging_slot_write(int level, const char* library, const char* message) {
    static const spdlog::level::level_enum levels[] = {spdlog::level::trace, spdlog::level::info, spdlog::level::warn,
                                                       spdlog::level::err};
    am_spdlog_logger().log(levels[level], "{}: {}", library, message);
}

// Makes the facade's logger. Runs in the diagnostics phase.
bool am_spdlog_setup_init(am::Context&) {
    am_spdlog_logger();
    am::lifecycle::init_step("spdlog:logger");
    return true;
}

// Writes out what the logger holds. Runs in the diagnostics phase at
// shutdown.
void am_spdlog_setup_shutdown(am::Context&) {
    am_spdlog_logger().flush();
    am::lifecycle::shutdown_step("spdlog:flush");
}
