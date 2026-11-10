// Automap diagnostics facade, diagnostics contract 1.0.0.
// SPDX-License-Identifier: MIT-0
//
// Every library's log, assert and profiling hooks route through this
// facade, tagged with the library's id. The diagnostics choices in the
// choices header pick where logs and profile zones go: stderr or the
// library in the logging slot, and nowhere or the library in the profiling
// slot (whose bindings define the am_logging_slot_* and am_profiling_slot_*
// functions).
#pragma once
#include <cstdint>

namespace am::diag {

enum class Level { trace, info, warn, error };

void log(Level level, const char* library, const char* message);

// Reports a failed assertion. Returns whether the caller should break into
// the debugger.
bool assert_failed(const char* library, const char* expression, const char* message, const char* file, int line);

// Log messages and assertion failures received from a library.
std::uint64_t message_count(const char* library);

// A profile zone: open for its lifetime.
class Zone {
public:
    explicit Zone(const char* name);
    ~Zone();
    Zone(const Zone&) = delete;
    Zone& operator=(const Zone&) = delete;
};

} // namespace am::diag
