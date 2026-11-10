// Saves a run to JSON and loads it back, so a run can be left and resumed at
// any shop or spin (docs/design/progression.md).
#pragma once

#include <string>

#include "rules/run.h"

namespace spin {

std::string run_to_json(const Run& run);
// Rebuilds the run on the given base tables. Returns false if the text isn't a saved run.
bool run_from_json(const Tables& base, const std::string& text, Run& out);

} // namespace spin
