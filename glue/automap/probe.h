// Automap probe harness.
// SPDX-License-Identifier: MIT-0
//
// Probe glue registers its probes; the executable runs them with --probe
// and the harness writes every result, with the recorded lifecycle steps,
// to the results file the CLI reads.
#pragma once
#include <string>

#include "automap/context.h"

namespace am::probe {

// Starts a probe run whose results go to path. The executable calls it
// before startup when given --probe <path>.
void begin(const std::string& path);

// rule: the rule id; library: who the result is about ("" for a pairing or
// the composition); failure_code: reported when !ok. A later result for the
// same rule and library replaces an earlier one. reason, when given, says
// under what condition the result holds, such as a check the build left out.
void check(const std::string& rule, const std::string& library, bool ok, const std::string& failure_code,
           const std::string& reason = "");

// Records a failure and writes the results file now, before a check that
// may abort the process, such as a library's own assertion; a later check
// replaces it.
void expect_failure(const std::string& rule, const std::string& library, const std::string& failure_code);

// A probe takes the composition's context, as entry points do.
using Probe = void (*)(am::Context& ctx);

// When a registered probe runs: after startup and the frames, or after
// shutdown (for checks that need the libraries to have released
// everything).
enum class When { running, after_shutdown };

// Registers a probe. Probe glue declares one static Registration per probe.
struct Registration {
    Registration(const char* recipe, Probe probe, When when = When::running);
};

// Runs every probe registered for when.
void run(When when, am::Context& ctx);

// Writes every recorded result and lifecycle step as JSON to the results
// file. Returns whether every check passed.
bool write_results();

} // namespace am::probe
