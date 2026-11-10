// Automap probe harness.
// SPDX-License-Identifier: MIT-0
#include "automap/lifecycle.h"

namespace am::lifecycle {
namespace {
std::vector<std::string>& init_list() {
    static std::vector<std::string> steps;
    return steps;
}
std::vector<std::string>& shutdown_list() {
    static std::vector<std::string> steps;
    return steps;
}
} // namespace

void init_step(const char* step) { init_list().emplace_back(step); }
void shutdown_step(const char* step) { shutdown_list().emplace_back(step); }

const std::vector<std::string>& init_steps() { return init_list(); }
const std::vector<std::string>& shutdown_steps() { return shutdown_list(); }

} // namespace am::lifecycle
