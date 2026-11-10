// The balance simulation: bots play thousands of runs on the real rules, and the
// report checks the shape of the difficulty (docs/design/session-rubric.md, part 1).
#pragma once

#include <cstdint>
#include <map>
#include <string>
#include <vector>

#include "bots.h"
#include "rules/run.h"

namespace spin::sim {

// What every run of a simulation is set up with.
struct Setup {
    std::string rig = "standard";
    int stake = 1;
    bool starter = false; // the first runs' shop: the tables' locked content is never offered
};

struct RunResult {
    bool won = false;
    int lost_round = 0; // 1-based; 0 for a win
    std::vector<std::string> bought;
    Metrics m;
};

// One bot plays one run to a win or a loss. The same seed replays the same run.
// trace, when given, gets one line for each step of the run.
RunResult play_run(const Tables& tables, std::uint64_t seed, const std::string& bot, Stats& stats,
                   const Setup& setup = {}, std::string* trace = nullptr);

// The bots the rubric is judged on: the no-engine bots, reckless, then every focus with every style.
std::vector<std::string> roster();
bool is_no_engine(const std::string& bot);
bool is_engine(const std::string& bot);

struct BotRuns {
    std::string name;
    std::vector<RunResult> runs;
    Stats stats;
};

// Each bot plays `runs` runs, on `threads` threads (0: one for each core).
// The results don't depend on the number of threads.
std::vector<BotRuns> run_bots(const Tables& tables, const std::vector<std::string>& names, int runs,
                              const Setup& setup = {}, unsigned threads = 0);

struct Check {
    std::string id, label;
    bool ok = false;
    std::string detail;
};

struct Report {
    std::string text;
    std::vector<Check> checks;        // A1, A2, B1, B2, B3, C1, C2, D1, E1, E2
    std::map<std::string, double> win, r1; // by bot: share of runs won, and lost in round 1
    double best = 0;                  // the best engine bot's win rate
    double no_engine = 0;             // the best no-engine bot's win rate
    // Press your luck, engine bots:
    double cargo_per_delivery = 0, spoiled = 0, quota = 0;

    int passed() const;
    bool pass() const { return passed() == static_cast<int>(checks.size()); }
};

// Runs the roster and judges the rubric. report().pass() says whether every check passed.
Report report(const Tables& tables, int runs, const Setup& setup = {}, unsigned threads = 0);

} // namespace spin::sim
