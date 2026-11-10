// spintacular-sim: bots play the real rules and judge the session rubric.
//
//   spintacular-sim sim [runs] [key=value ...] [--tables <path>] [rig=<id>] [stake=<n>] [content=starter]
//   spintacular-sim ladder [runs] [key=value ...] [--tables <path>]   every rig on stake 1, then Standard up the stakes
//   spintacular-sim selftest [--tables <path>]                        seeds replay; rerolls leave the deal and the spins alone
//
// key=value sets one number in the tables for this run, such as round.tank=14.
// sim exits non-zero when a rubric check fails.
#include <algorithm>
#include <cstdio>
#include <cstdlib>
#include <exception>
#include <string>
#include <vector>

#include "data/tables_json.h"
#include "sim.h"

namespace {

using namespace spin;
using namespace spin::sim;

std::string cell(const std::string& s, int width, bool right = true) {
    const std::size_t w = static_cast<std::size_t>(width);
    if (s.size() >= w) return s;
    return right ? std::string(w - s.size(), ' ') + s : s + std::string(w - s.size(), ' ');
}

std::string pct(double v, int decimals) {
    char buf[32];
    std::snprintf(buf, sizeof buf, "%.*f%%", decimals, v * 100.0);
    return buf;
}

// Every rig on stake 1, then the Standard rig up the stakes.
void ladder(const Tables& tables, int runs) {
    const std::string head = cell("", 22) + cell("engine", 8) + cell("no-eng", 8) + cell("worst r1", 9) + cell("reck r1", 9) +
                             cell("pass", 6) + "   best by focus";
    auto row = [&](const std::string& label, const Setup& setup) {
        const Report rep = report(tables, runs, setup);
        double worst = 0;
        std::vector<std::pair<std::string, double>> fams; // each focus's best style
        for (auto& name : roster()) {
            if (name != "reckless") worst = std::max(worst, rep.r1.at(name));
            if (!is_engine(name)) continue;
            const std::string focus = name.substr(0, name.find('/'));
            auto it = std::find_if(fams.begin(), fams.end(), [&](const auto& f) { return f.first == focus; });
            if (it == fams.end())
                fams.push_back({focus, rep.win.at(name)});
            else
                it->second = std::max(it->second, rep.win.at(name));
        }
        std::string line = cell(label, 22, false) + cell(pct(rep.best, 0), 8) + cell(pct(rep.no_engine, 1), 8) +
                           cell(pct(worst, 1), 9) + cell(pct(rep.r1.at("reckless"), 0), 9) +
                           cell(std::to_string(rep.passed()), 6) + "  ";
        for (auto& [focus, win] : fams) line += " " + focus + " " + pct(win, 0);
        std::printf("%s\n", line.c_str());
        std::fflush(stdout);
    };
    std::printf("%d runs per bot. Same targets for every rig.\n\nRigs, stake 1\n%s\n", runs, head.c_str());
    for (auto& rig : tables.rigs) {
        Setup s;
        s.rig = rig.id;
        row(rig.id, s);
    }
    std::printf("\nStakes, standard rig\n%s\n", head.c_str());
    for (int stake = 1; stake <= static_cast<int>(tables.stakes.size()) + 1; ++stake) {
        Setup s;
        s.stake = stake;
        row("stake " + std::to_string(stake), s);
    }
}

// Seeds: the same seed replays the same run, and shop rerolls change neither the deal nor the spins.
bool selftest(const Tables& tables) {
    bool ok = true;
    auto expect = [&](bool cond, const char* what) {
        if (!cond) {
            std::printf("FAIL: %s\n", what);
            ok = false;
        }
    };

    std::string traces[2];
    for (auto& trace : traces) {
        Stats stats;
        play_run(tables, 99, "quota/reroller", stats, {}, &trace);
    }
    expect(!traces[0].empty() && traces[0] == traces[1], "same seed, different run");

    // Two runs on one seed, played alike to the first shop. Then b rerolls the shop many times.
    bool tested = false;
    for (std::uint64_t seed = 5; seed < 25 && !tested; ++seed) {
        RunConfig cfg;
        cfg.seed = seed;
        Run a(tables, cfg), b(tables, cfg);
        for (Run* run : {&a, &b}) {
            Bot passive{Rng(seed)};
            while (run->phase == Phase::Trip || run->phase == Phase::Rescue || run->phase == Phase::Rod) passive.act(*run);
        }
        if (a.phase != Phase::Shop || b.phase != Phase::Shop) continue; // lost round 1: try the next seed
        tested = true;
        const int money = b.money;
        b.money = 1000000;
        int rerolls = 0;
        for (int i = 0; i < 25; ++i) rerolls += b.reroll();
        b.money = money;
        expect(rerolls == 25, "the shop didn't reroll");
        a.leave_shop();
        b.leave_shop();
        bool same_deal = a.deal.size() == b.deal.size() && a.inspection() == b.inspection();
        for (std::size_t wi = 0; same_deal && wi < a.deal.size(); ++wi) {
            same_deal = a.deal[wi].size() == b.deal[wi].size();
            for (std::size_t k = 0; same_deal && k < a.deal[wi].size(); ++k)
                same_deal = a.deal[wi][k].uid == b.deal[wi][k].uid && a.wedge_label(a.deal[wi][k]) == b.wedge_label(b.deal[wi][k]);
        }
        expect(same_deal, "rerolls changed the deal");
        bool same_spins = true;
        for (int i = 0; i < 5; ++i) same_spins = a.spin(i % static_cast<int>(a.deal.size())).index == b.spin(i % static_cast<int>(b.deal.size())).index && same_spins;
        for (int i = 0; i < 5; ++i) same_spins = a.rng_spin.next() == b.rng_spin.next() && same_spins;
        expect(same_spins, "rerolls changed the spins");
    }
    expect(tested, "no run reached a shop");
    if (ok) std::printf("seeds: same seed replays the same run; shop rerolls change neither the deal nor the spins\n");
    return ok;
}

bool all_digits(const std::string& s) { return !s.empty() && std::all_of(s.begin(), s.end(), [](char c) { return c >= '0' && c <= '9'; }); }

} // namespace

int main(int argc, char** argv) {
    try {
        const std::string mode = argc > 1 ? argv[1] : "sim";
        std::string path = "assets/data/tables.json";
        std::vector<std::string> overrides;
        Setup setup;
        int runs = 0;
        for (int i = 2; i < argc; ++i) {
            const std::string a = argv[i];
            if (a == "--tables" && i + 1 < argc) {
                path = argv[++i];
            } else if (a.rfind("rig=", 0) == 0) {
                setup.rig = a.substr(4);
            } else if (a.rfind("stake=", 0) == 0) {
                setup.stake = std::atoi(a.c_str() + 6);
            } else if (a.rfind("content=", 0) == 0) {
                setup.starter = a.substr(8) == "starter";
            } else if (a.find('=') != std::string::npos) {
                overrides.push_back(a);
            } else if (all_digits(a) && runs == 0) {
                runs = std::atoi(a.c_str());
            } else {
                std::fprintf(stderr, "unknown argument: %s\n", a.c_str());
                return 2;
            }
        }
        if (runs <= 0) runs = 300;
        std::string text = read_file(path);
        if (text.empty()) {
            std::fprintf(stderr, "can't read the tables at %s\n", path.c_str());
            return 2;
        }
        for (auto& ov : overrides) text = override_tables_json(text, ov);
        const Tables tables = tables_from_json(text);
        if (!tables.rig(setup.rig)) {
            std::fprintf(stderr, "unknown rig: %s\n", setup.rig.c_str());
            return 2;
        }

        if (mode == "sim") {
            const Report rep = report(tables, runs, setup);
            std::printf("%s\n", rep.text.c_str());
            return rep.pass() ? 0 : 1;
        }
        if (mode == "ladder") {
            ladder(tables, runs);
            return 0;
        }
        if (mode == "selftest") return selftest(tables) ? 0 : 1;
        std::fprintf(stderr, "usage: spintacular-sim sim|ladder|selftest [runs] [key=value ...] [--tables <path>] "
                             "[rig=<id>] [stake=<n>] [content=starter]\n");
        return 2;
    } catch (const std::exception& e) {
        std::fprintf(stderr, "%s\n", e.what());
        return 2;
    }
}
