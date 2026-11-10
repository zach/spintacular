// Tracy's binding to the diagnostics contract: Tracy in the profiling slot
// records every zone the diagnostics facade opens.
// SPDX-License-Identifier: MIT-0
#include <cstring>
#include <memory>
#include <vector>

#include <tracy/Tracy.hpp>

namespace {

// The zones open on this thread, innermost last: the facade opens and closes
// zones in stack order on each thread.
thread_local std::vector<std::unique_ptr<tracy::ScopedZone>> zones;

} // namespace

void am_profiling_slot_zone_begin(const char* name) {
    static const char source[] = __FILE__;
    static const char function[] = "am::diag::Zone";
    zones.push_back(std::make_unique<tracy::ScopedZone>(__LINE__, source, sizeof source - 1, function, sizeof function - 1,
                                                        name, std::strlen(name), 0));
}

void am_profiling_slot_zone_end() {
    if (!zones.empty()) zones.pop_back();
}
