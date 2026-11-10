// Automap diagnostics facade, diagnostics contract 1.0.0.
// SPDX-License-Identifier: MIT-0
#include "automap/diag.h"

#include <cstdio>
#include <map>
#include <mutex>
#include <string>

#include "automap/choices.h"

#if defined(AUTOMAP_DIAGNOSTICS_LOG_LOGGING_SLOT)
// Defined by the logging slot's binding.
void am_logging_slot_write(int level, const char* library, const char* message);
#endif
#if defined(AUTOMAP_DIAGNOSTICS_PROFILER_PROFILING_SLOT)
// Defined by the profiling slot's binding.
void am_profiling_slot_zone_begin(const char* name);
void am_profiling_slot_zone_end();
#endif

namespace am::diag {
namespace {

struct State {
    std::mutex mutex;
    std::map<std::string, std::uint64_t> counts;
};

// Never destroyed: libraries may log while the process exits.
State& state() {
    static State* s = new State;
    return *s;
}

void count(const char* library) {
    State& s = state();
    std::lock_guard<std::mutex> lock(s.mutex);
    ++s.counts[library];
}

void write(Level level, const char* library, const char* message) {
#if defined(AUTOMAP_DIAGNOSTICS_LOG_LOGGING_SLOT)
    am_logging_slot_write(static_cast<int>(level), library, message);
#else
    static const char* const names[] = {"trace", "info", "warn", "error"};
    std::fprintf(stderr, "[%s] %s: %s\n", names[static_cast<int>(level)], library, message);
#endif
}

} // namespace

void log(Level level, const char* library, const char* message) {
    count(library);
    write(level, library, message ? message : "");
}

bool assert_failed(const char* library, const char* expression, const char* message, const char* file, int line) {
    count(library);
    char text[1024];
    std::snprintf(text, sizeof text, "assertion failed: %s%s%s (%s:%d)", expression ? expression : "", message ? ": " : "",
                  message ? message : "", file ? file : "?", line);
    write(Level::error, library, text);
    return false;
}

std::uint64_t message_count(const char* library) {
    State& s = state();
    std::lock_guard<std::mutex> lock(s.mutex);
    auto it = s.counts.find(library);
    return it == s.counts.end() ? 0 : it->second;
}

#if defined(AUTOMAP_DIAGNOSTICS_PROFILER_PROFILING_SLOT)
Zone::Zone(const char* name) { am_profiling_slot_zone_begin(name); }
Zone::~Zone() { am_profiling_slot_zone_end(); }
#else
Zone::Zone(const char*) {}
Zone::~Zone() {}
#endif

} // namespace am::diag
