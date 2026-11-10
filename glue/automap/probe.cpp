// Automap probe harness.
// SPDX-License-Identifier: MIT-0
#include "automap/probe.h"

#include <cstdio>
#include <vector>

#include "automap/choices.h"
#include "automap/lifecycle.h"

namespace am::probe {
namespace {

struct Result {
    std::string rule, library, failure_code, reason;
    bool ok;
};

std::vector<Result>& results() {
    static std::vector<Result> r;
    return r;
}

struct Registered {
    std::string recipe;
    Probe probe;
    When when;
};

std::vector<Registered>& registry() {
    static std::vector<Registered> r;
    return r;
}

std::string& results_path() {
    static std::string p;
    return p;
}

// build-flags.language-settings-agree: the glue compiles with the C++
// standard the composition chooses. The harness checks it for every
// composition, since every composition makes the choice.
void check_language_standard(am::Context&) {
    // MSVC reports the standard in _MSVC_LANG; its __cplusplus stays 199711L
    // without /Zc:__cplusplus.
#ifdef _MSVC_LANG
    long standard = _MSVC_LANG;
#else
    long standard = __cplusplus;
#endif
    bool agree = standard / 100 % 100 == AUTOMAP_BUILD_FLAGS_CXX_STANDARD % 100;
    check("build-flags.language-settings-agree", "", agree, "LANG_SETTINGS_MISMATCH(cxx_standard)");
}
const Registration language_standard("automap", &check_language_standard);

std::string quote(const std::string& s) {
    std::string out = "\"";
    for (char c : s) {
        if (c == '"' || c == '\\') out += '\\';
        if (static_cast<unsigned char>(c) < 0x20) {
            char buf[8];
            std::snprintf(buf, sizeof buf, "\\u%04x", c);
            out += buf;
            continue;
        }
        out += c;
    }
    return out + "\"";
}

void write_steps(std::FILE* f, const char* key, const std::vector<std::string>& steps) {
    std::fprintf(f, "  %s: [", quote(key).c_str());
    for (std::size_t i = 0; i < steps.size(); ++i) std::fprintf(f, "%s%s", i ? ", " : "", quote(steps[i]).c_str());
    std::fprintf(f, "]");
}

} // namespace

Registration::Registration(const char* recipe, Probe probe, When when) {
    registry().push_back({recipe, probe, when});
}

void run(When when, am::Context& ctx) {
    for (const Registered& r : registry()) {
        if (r.when == when) r.probe(ctx);
    }
}

void begin(const std::string& path) { results_path() = path; }

void check(const std::string& rule, const std::string& library, bool ok, const std::string& failure_code,
           const std::string& reason) {
    for (Result& r : results()) {
        if (r.rule == rule && r.library == library) {
            r = {rule, library, failure_code, reason, ok};
            return;
        }
    }
    results().push_back({rule, library, failure_code, reason, ok});
}

void expect_failure(const std::string& rule, const std::string& library, const std::string& failure_code) {
    check(rule, library, false, failure_code);
    write_results();
}

bool write_results() {
    if (results_path().empty()) return false;
    std::FILE* f = std::fopen(results_path().c_str(), "w");
    if (f == nullptr) return false;
    bool all_ok = true;
    std::fprintf(f, "{\n  \"results\": [");
    for (std::size_t i = 0; i < results().size(); ++i) {
        const Result& r = results()[i];
        all_ok = all_ok && r.ok;
        std::fprintf(f, "%s\n    {\"rule\": %s, \"library\": %s, \"status\": %s", i ? "," : "", quote(r.rule).c_str(),
                     quote(r.library).c_str(), r.ok ? "\"pass\"" : "\"fail\"");
        if (!r.ok) std::fprintf(f, ", \"failure_code\": %s", quote(r.failure_code).c_str());
        if (!r.reason.empty()) std::fprintf(f, ", \"reason\": %s", quote(r.reason).c_str());
        std::fprintf(f, "}");
    }
    std::fprintf(f, "\n  ],\n");
    write_steps(f, "init_steps", lifecycle::init_steps());
    std::fprintf(f, ",\n");
    write_steps(f, "shutdown_steps", lifecycle::shutdown_steps());
    std::fprintf(f, "\n}\n");
    std::fclose(f);
    return all_ok;
}

} // namespace am::probe
