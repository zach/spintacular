// miniaudio's binding to the diagnostics contract: the log the context and
// engine write to reports to the facade.
// SPDX-License-Identifier: MIT-0
#include <cstring>

#include "automap/context.h"
#include "automap/diag.h"
#include "automap/lifecycle.h"
#include "miniaudio_setup.h"

void am_miniaudio_log_message(void*, ma_uint32 level, const char* message) {
    // miniaudio ends its messages with a newline; the facade adds its own.
    char line[1024];
    std::size_t n = std::strlen(message);
    if (n >= sizeof line) n = sizeof line - 1;
    std::memcpy(line, message, n);
    while (n > 0 && (line[n - 1] == '\n' || line[n - 1] == '\r')) --n;
    line[n] = '\0';
    am::diag::Level facade = am::diag::Level::trace;
    switch (level) {
    case MA_LOG_LEVEL_ERROR: facade = am::diag::Level::error; break;
    case MA_LOG_LEVEL_WARNING: facade = am::diag::Level::warn; break;
    case MA_LOG_LEVEL_INFO: facade = am::diag::Level::info; break;
    default: break;
    }
    am::diag::log(facade, "miniaudio", line);
}

// Makes the log miniaudio writes to and routes it through the diagnostics
// facade. Runs in the diagnostics phase, before the systems phase gives the
// log to the context and engine.
bool am_bind_miniaudio_diagnostics(am::Context&) {
    if (ma_log_init(&am_miniaudio_allocation(), &am_miniaudio_log()) != MA_SUCCESS) return false;
    ma_log_register_callback(&am_miniaudio_log(), ma_log_callback_init(&am_miniaudio_log_message, nullptr));
    am::lifecycle::init_step("miniaudio:ma_log_init");
    return true;
}

// Removes the log once the context and engine are gone. Runs in the
// diagnostics phase at shutdown.
void am_unbind_miniaudio_diagnostics(am::Context&) {
    ma_log_uninit(&am_miniaudio_log());
    am::lifecycle::shutdown_step("miniaudio:ma_log_uninit");
}
