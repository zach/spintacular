// Automap probe harness: lifecycle step recording.
// SPDX-License-Identifier: MIT-0
//
// Glue records each lifecycle step as it runs; verify compares the
// recorded order with the declared one.
#pragma once
#include <string>
#include <vector>

namespace am::lifecycle {

// step is "<owner>:<name>", as declared in the map.
void init_step(const char* step);
void shutdown_step(const char* step);

const std::vector<std::string>& init_steps();
const std::vector<std::string>& shutdown_steps();

} // namespace am::lifecycle
