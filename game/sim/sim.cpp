#include "sim.h"

#include <algorithm>
#include <atomic>
#include <cstdio>
#include <thread>

namespace spin::sim {

namespace {

const char* const kStyles[] = {"spender", "saver", "reroller", "bargain"};
const char* const kFocuses[] = {"color", "straight", "safety", "quota", "adaptive"};
const char* const kNoEngine[] = {"passive", "random", "flat/spender"};

// ---- text, laid out as the prototype's report is

std::string pct(double v, int decimals) {
    char buf[32];
    std::snprintf(buf, sizeof buf, "%.*f%%", decimals, v * 100.0);
    return buf;
}

std::string left(const std::string& s, std::size_t width) {
    return s.size() >= width ? s : s + std::string(width - s.size(), ' ');
}

std::string right(const std::string& s, std::size_t width) {
    return s.size() >= width ? s : std::string(width - s.size(), ' ') + s;
}

std::string fixed(double v, int decimals) {
    char buf[32];
    std::snprintf(buf, sizeof buf, "%.*f", decimals, v);
    return buf;
}

// Each bot gets its own block of seeds, the same every time (CRC-32 of its name, as in the prototype).
std::uint64_t seed_base(const std::string& name) {
    std::uint32_t crc = 0xFFFFFFFFu;
    for (unsigned char c : name) {
        crc ^= c;
        for (int k = 0; k < 8; ++k) crc = (crc >> 1) ^ (0xEDB88320u & (0u - (crc & 1u)));
    }
    crc ^= 0xFFFFFFFFu;
    return 10000ull * (crc % 1000u);
}

std::string focus_of(const std::string& bot) { return bot.substr(0, bot.find('/')); }
std::string style_of(const std::string& bot) { return bot.substr(bot.find('/') + 1); }

void add(Metrics& a, const Metrics& b) {
    a.spins += b.spins;
    a.deliveries += b.deliveries;
    a.delivered_cargo += b.delivered_cargo;
    a.spoiled += b.spoiled;
    a.spoiled_cargo += b.spoiled_cargo;
    a.quota_hits += b.quota_hits;
    a.freight += b.freight;
    a.full_trucks += b.full_trucks;
    a.saves += b.saves;
    a.zaps += b.zaps;
    a.rerolls += b.rerolls;
    a.best_delivery = std::max(a.best_delivery, b.best_delivery);
}

} // namespace

// ---------------------------------------------------------------- one run

RunResult play_run(const Tables& tables, std::uint64_t seed, const std::string& bot_name, Stats& stats,
                   const Setup& setup, std::string* trace) {
    RunConfig cfg;
    cfg.seed = seed;
    cfg.rig = setup.rig;
    cfg.stake = setup.stake;
    if (setup.starter) cfg.locked = tables.locked;
    Run run(tables, cfg);
    // The bot's own stream: its choices never move the run's.
    std::unique_ptr<Bot> bot = make_bot(bot_name, Rng(seed ^ 0xB07B07B07B07B07Bull));
    RunResult res;
    for (;;) {
        if (trace) {
            char buf[160];
            std::snprintf(buf, sizeof buf, "round %d phase %d score %lld $%d trips %d spins %d cargo %d bag %d items %d\n",
                          run.round + 1, static_cast<int>(run.phase), run.round_score, run.money, run.trips_left,
                          run.load.spins, static_cast<int>(run.load.cargo.size()), static_cast<int>(run.bag.size()),
                          static_cast<int>(run.items.size()));
            *trace += buf;
        }
        switch (run.phase) {
        case Phase::Trip:
        case Phase::Rescue:
        case Phase::Rod: bot->act(run); break;
        case Phase::Shop:
            stats.count_offers(run);
            bot->shop(run, stats);
            run.leave_shop();
            break;
        case Phase::Won:
        case Phase::Lost:
            res.won = run.phase == Phase::Won;
            res.lost_round = res.won ? 0 : run.round + 1;
            res.bought = run.bought;
            res.m = run.m;
            return res;
        }
    }
}

// ---------------------------------------------------------------- many runs

std::vector<std::string> roster() {
    std::vector<std::string> names(std::begin(kNoEngine), std::end(kNoEngine));
    names.push_back("reckless");
    for (const char* s : kStyles)
        for (const char* f : kFocuses) names.push_back(std::string(f) + "/" + s);
    return names;
}

bool is_no_engine(const std::string& bot) {
    return std::find(std::begin(kNoEngine), std::end(kNoEngine), bot) != std::end(kNoEngine);
}

bool is_engine(const std::string& bot) { return !is_no_engine(bot) && bot != "reckless"; }

std::vector<BotRuns> run_bots(const Tables& tables, const std::vector<std::string>& names, int runs,
                              const Setup& setup, unsigned threads) {
    // The work is cut into chunks of runs. Each chunk keeps its own stats, merged in order
    // afterwards, so the result is the same on any number of threads.
    struct Chunk {
        int bot = 0, first = 0, count = 0;
        Stats stats;
    };
    const int chunk_size = 25;
    std::vector<BotRuns> out(names.size());
    std::vector<Chunk> chunks;
    for (int b = 0; b < static_cast<int>(names.size()); ++b) {
        out[b].name = names[b];
        out[b].runs.resize(static_cast<std::size_t>(std::max(0, runs)));
        for (int first = 0; first < runs; first += chunk_size) {
            Chunk c;
            c.bot = b;
            c.first = first;
            c.count = std::min(chunk_size, runs - first);
            chunks.push_back(c);
        }
    }
    std::atomic<std::size_t> next{0};
    auto work = [&]() {
        for (;;) {
            const std::size_t k = next.fetch_add(1);
            if (k >= chunks.size()) return;
            Chunk& c = chunks[k];
            const std::string& name = names[c.bot];
            const std::uint64_t base = seed_base(name);
            for (int i = c.first; i < c.first + c.count; ++i)
                out[c.bot].runs[i] = play_run(tables, base + static_cast<std::uint64_t>(i), name, c.stats, setup);
        }
    };
    if (threads == 0) threads = std::max(1u, std::thread::hardware_concurrency());
    threads = static_cast<unsigned>(std::min<std::size_t>(threads, std::max<std::size_t>(1, chunks.size())));
    std::vector<std::thread> pool;
    for (unsigned i = 1; i < threads; ++i) pool.emplace_back(work);
    work();
    for (auto& th : pool) th.join();
    for (auto& c : chunks) out[c.bot].stats.merge(c.stats);
    return out;
}

// ---------------------------------------------------------------- the report

int Report::passed() const {
    int n = 0;
    for (auto& c : checks) n += c.ok;
    return n;
}

Report report(const Tables& tables, int runs, const Setup& setup, unsigned threads) {
    const Tables t = apply_setup(tables, setup.rig, setup.stake);
    const std::vector<std::string> names = roster();
    const std::vector<BotRuns> res = run_bots(tables, names, runs, setup, threads);
    const int rounds = t.run.rounds;
    const double n_runs = std::max(1, runs);
    Report rep;
    std::vector<std::string> lines;

    std::string targets;
    for (std::size_t i = 0; i < t.targets.size(); ++i) targets += (i ? ", " : "") + std::to_string(t.targets[i]);
    lines.push_back(std::to_string(runs) + " runs per bot, " + std::to_string(rounds) + " rounds, targets [" + targets + "]");
    lines.push_back("");
    lines.push_back(left("bot", 20) + right("win", 7) + right("lose r1", 9) + "  losses by round");

    // The engine bots together: what they saw in the shop, what the winners bought, how their loads went.
    Stats stats;
    std::map<std::string, long long> presence;
    long long wins_total = 0;
    Metrics m;
    std::map<int, long long> losses; // engine-bot losses, by round
    for (auto& br : res) {
        long long won = 0, lost_r1 = 0;
        std::vector<long long> by(static_cast<std::size_t>(rounds) + 1, 0);
        for (auto& r : br.runs) {
            won += r.won;
            lost_r1 += r.lost_round == 1;
            if (!r.won && r.lost_round >= 1 && r.lost_round <= rounds) ++by[r.lost_round];
        }
        rep.win[br.name] = static_cast<double>(won) / n_runs;
        rep.r1[br.name] = static_cast<double>(lost_r1) / n_runs;
        std::string line = left(br.name, 20) + right(pct(rep.win[br.name], 1), 7) + right(pct(rep.r1[br.name], 1), 9) + "  ";
        for (int i = 1; i <= rounds; ++i) line += (i > 1 ? " " : "") + right(std::to_string(by[i]), 3);
        lines.push_back(line);
        if (!is_engine(br.name)) continue;
        stats.merge(br.stats);
        for (auto& r : br.runs) {
            add(m, r.m);
            if (r.won) {
                ++wins_total;
                for (auto& id : r.bought) ++presence[id];
            } else {
                ++losses[r.lost_round];
            }
        }
    }
    lines.push_back("");

    auto check = [&](const char* id, const char* label, bool ok, const std::string& detail) {
        rep.checks.push_back({id, label, ok, detail});
    };

    // A: the start.
    double worst_r1 = 0;
    for (auto& name : names)
        if (name != "reckless") worst_r1 = std::max(worst_r1, rep.r1[name]);
    check("A1", "Round 1 is almost impossible to lose (every non-reckless bot <= 1%)", worst_r1 <= 0.01,
          "worst bot loses round 1 " + pct(worst_r1, 1));
    check("A2", "An insane risk can lose round 1 (reckless >= 50%)", rep.r1["reckless"] >= 0.5,
          "reckless loses round 1 " + pct(rep.r1["reckless"], 1));

    // B: the wall, and the factories that get over it.
    for (auto& name : names)
        if (is_no_engine(name)) rep.no_engine = std::max(rep.no_engine, rep.win[name]);
    check("B1", "Winning without a point factory is almost impossible (no-engine bots <= 3%)", rep.no_engine <= 0.03,
          "best no-engine bot wins " + pct(rep.no_engine, 1));
    std::string best_bot;
    rep.best = -1;
    for (auto& name : names)
        if (is_engine(name) && rep.win[name] > rep.best) {
            rep.best = rep.win[name];
            best_bot = name;
        }
    rep.best = std::max(0.0, rep.best);
    check("B2", "A point factory can win (best engine bot >= 40%)", rep.best >= 0.4,
          "best engine bot " + best_bot + " wins " + pct(rep.best, 1));
    check("B3", "A point factory is still risky (no bot wins >= 90%)", rep.best < 0.9,
          "best engine bot wins " + pct(rep.best, 1));

    // C: more than one factory, more than one way to spend.
    int viable = 0;
    std::string fams;
    for (const char* f : kFocuses) {
        double top = 0;
        for (auto& name : names)
            if (is_engine(name) && focus_of(name) == f) top = std::max(top, rep.win[name]);
        viable += rep.best > 0 && top >= 0.5 * rep.best;
        fams += (fams.empty() ? "" : ", ") + std::string(f) + " " + pct(top, 0);
    }
    check("C1", "More than one kind of point factory is viable (>= 3 focuses within half the best)", viable >= 3, fams);
    std::vector<double> style_means;
    std::string styles;
    for (const char* s : kStyles) {
        double sum = 0;
        int n = 0;
        for (auto& name : names)
            if (is_engine(name) && style_of(name) == s) {
                sum += rep.win[name];
                ++n;
            }
        style_means.push_back(n ? sum / n : 0);
        styles += (styles.empty() ? "" : ", ") + std::string(s) + " " + pct(style_means.back(), 0);
    }
    std::sort(style_means.begin(), style_means.end());
    const double top_style = style_means.back(), median_style = style_means[style_means.size() / 2];
    check("C2", "No spending strategy dominates (best style's mean <= 1.5x the median style's)",
          top_style > 0 && top_style <= 1.5 * median_style, styles);

    // D: no cliff.
    long long worst = 0, lost_total = 0;
    int worst_round = 0;
    for (auto& [round, n] : losses) {
        lost_total += n;
        if (n > worst) {
            worst = n;
            worst_round = round;
        }
    }
    const double share = static_cast<double>(worst) / static_cast<double>(std::max<long long>(1, lost_total));
    check("D1", "No cliff: no single round causes > 35% of engine-bot losses", share <= 0.35,
          "round " + (worst_round ? std::to_string(worst_round) : std::string("None")) + " causes " + pct(share, 0) + " of losses");

    // E: the shop.
    const double choice = static_cast<double>(stats.shops_with_choice) / static_cast<double>(std::max<long long>(1, stats.shops_with_fit));
    check("E1", "Shopping isn't forced: of shops with a good fit, >= 50% have two or more", choice >= 0.5, pct(choice, 0));
    std::string top_id = "-";
    long long top_n = 0;
    for (auto& [id, n] : presence)
        if ((t.item(id) || t.wheel(id)) && n > top_n) {
            top_n = n;
            top_id = id;
        }
    const double wins = static_cast<double>(std::max<long long>(1, wins_total));
    check("E2", "No must-have: no item or wheel is bought in > 80% of winning runs", top_n / wins <= 0.8,
          top_id + " in " + pct(top_n / wins, 0) + " of " + std::to_string(wins_total) + " wins");

    lines.push_back("Rubric (docs/design/session-rubric.md)");
    for (auto& c : rep.checks) lines.push_back(std::string("  [") + (c.ok ? "PASS" : "FAIL") + "] " + c.id + " " + c.label + ": " + c.detail);

    // Press your luck.
    const long long loads = static_cast<long long>(m.deliveries) + m.spoiled;
    rep.cargo_per_delivery = static_cast<double>(m.delivered_cargo) / std::max(1, m.deliveries);
    rep.spoiled = static_cast<double>(m.spoiled) / static_cast<double>(std::max<long long>(1, loads));
    rep.quota = static_cast<double>(m.quota_hits) / std::max(1, m.deliveries);
    lines.push_back("");
    lines.push_back("Engine bots' loads: " + fixed(rep.cargo_per_delivery, 1) + " cargo per delivery, " + pct(rep.spoiled, 0) +
                    " of loads spoiled, " + pct(rep.quota, 0) + " of deliveries reach the quota");
    lines.push_back("");

    // Content telemetry, most present in wins first.
    lines.push_back("Telemetry, engine bots: pick rate = picked / offered; win presence = share of wins that bought it");
    lines.push_back("  " + left("content", 16) + right("offered", 8) + right("pick", 7) + right("win pres", 10));
    std::vector<std::string> ids;
    for (auto& [id, n] : stats.offered) ids.push_back(id);
    std::stable_sort(ids.begin(), ids.end(), [&](const std::string& a, const std::string& b) { return presence[a] > presence[b]; });
    for (auto& id : ids) {
        const double off = static_cast<double>(stats.offered[id]);
        lines.push_back("  " + left(id, 16) + right(std::to_string(stats.offered[id]), 8) + right(pct(stats.picked[id] / off, 0), 7) +
                        right(pct(presence[id] / wins, 0), 10));
    }
    lines.push_back("");
    lines.push_back(std::to_string(rep.passed()) + "/" + std::to_string(rep.checks.size()) + " rubric checks pass");

    for (std::size_t i = 0; i < lines.size(); ++i) rep.text += (i ? "\n" : "") + lines[i];
    return rep;
}

} // namespace spin::sim
