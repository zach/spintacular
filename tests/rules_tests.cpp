// Tests of the rules core against the design docs' worked examples, and of
// saving and loading a run. Usage: spintacular-tests <tables.json>
#include <cstdio>
#include <string>

#include "data/profile.h"
#include "data/save.h"
#include "data/tables_json.h"
#include "rules/run.h"

using namespace spin;

static int failures = 0;
#define CHECK(cond)                                                        \
    do {                                                                   \
        if (!(cond)) {                                                     \
            std::printf("FAIL %s:%d: %s\n", __FILE__, __LINE__, #cond);    \
            ++failures;                                                    \
        }                                                                  \
    } while (0)

static Load load_of(std::initializer_list<Cargo> cargo) {
    Load l;
    for (auto& c : cargo) l.cargo.push_back(c);
    return l;
}

// Plays a run with a fixed, simple policy and returns a fingerprint of it.
static std::string play(Run& r, int max_actions, std::string* mid_save = nullptr, int save_at = -1) {
    std::string trace;
    for (int n = 0; n < max_actions && r.phase != Phase::Lost && r.phase != Phase::Won; ++n) {
        if (n == save_at && mid_save) *mid_save = run_to_json(r);
        switch (r.phase) {
        case Phase::Trip:
            if (r.must_deliver() || r.load.cargo.size() >= 3) {
                trace += "D" + std::to_string(r.deliver().score.total) + " ";
            } else {
                int best = 0;
                for (int i = 1; i < static_cast<int>(r.deal.size()); ++i)
                    if (r.spoil_chance(i) < r.spoil_chance(best)) best = i;
                trace += "s" + std::to_string(r.spin(best).index);
            }
            break;
        case Phase::Rescue: r.rescue(r.rescue_allowed(Rescue::Respin) ? Rescue::Respin : Rescue::NudgeLeft); break;
        case Phase::Rod: r.rod_remove(0); break;
        case Phase::Shop:
            for (int i = 0; i < static_cast<int>(r.offers.size()); ++i)
                if (r.offers[i].kind == OfferKind::Item || r.offers[i].kind == OfferKind::Wedge || r.offers[i].kind == OfferKind::Consumable)
                    if (r.buy(i)) trace += "b" + r.offers[i].id + " ";
            r.leave_shop();
            trace += "| ";
            break;
        default: break;
        }
    }
    return trace + "$" + std::to_string(r.money) + " r" + std::to_string(r.round);
}

int main(int argc, char** argv) {
    Tables t = load_tables(argc > 1 ? argv[1] : "assets/data/tables.json");
    RunConfig cfg;
    cfg.seed = 7;

    { // scoring.md: R2, G3, B4 is 9 points x (3 cargo + 2 for a run of three) = 45; at a quota of 9 or less, 90.
        Run r(t, cfg);
        Load l = load_of({{0, 2, 0, 1}, {1, 3, 0, 2}, {2, 4, 0, 3}});
        r.quota = 10;
        CHECK(r.score(l).total == 45);
        r.quota = 9;
        std::vector<Step> steps;
        Score s = r.score(l, &steps);
        CHECK(s.total == 90 && s.quota_hit && !s.full);
        CHECK(steps.size() == 7); // three cargo, size, run, quota, total
        CHECK(steps.back().kind == Step::Kind::Total);
    }
    { // A full truck of six values pays x1.5; six of a kind color adds the biggest set bonus.
        Run r(t, cfg);
        r.quota = 1000;
        Load l = load_of({{0, 1, 0, 1}, {0, 2, 0, 2}, {0, 3, 0, 3}, {0, 4, 0, 4}, {0, 5, 0, 5}, {0, 6, 0, 6}});
        Score s = r.score(l);
        CHECK(s.full && s.flush == 6 && s.straight == 6);
        CHECK(s.total == static_cast<long long>(21 * (6 + 11 + 11) * 1.5));
    }
    { // No repeats: a landed wedge, or a value already in the truck.
        Run r(t, cfg);
        Wedge a = r.deal[0][0];
        CHECK(!r.clashes(0, a));
        SpinResult first = r.spin(0);
        CHECK(first.ok && first.chance == 0.0); // a first spin can never spoil a load
        const Wedge landed = r.deal[0][first.index];
        CHECK(r.clashes(0, landed));
        Wedge same_value = landed;
        same_value.uid = 9999;
        CHECK(r.clashes(1, same_value));
        Wedge tarp;
        tarp.uid = 9998;
        tarp.kind = WedgeKind::Tarp;
        CHECK(!r.clashes(1, tarp));
        CHECK(r.spoil_chance(0) > 0.3);
    }
    { // The same seed replays the same run, and a run survives saving and loading mid-way.
        Run a(t, cfg), b(t, cfg);
        std::string saved;
        std::string ta = play(a, 400, &saved, 23);
        CHECK(ta == play(b, 400));
        Run c;
        CHECK(!saved.empty());
        CHECK(run_from_json(t, saved, c));
        Run d(t, cfg);
        play(d, 23);
        CHECK(run_to_json(c) == run_to_json(d));
        play(c, 400 - 23);
        CHECK(run_to_json(c) == run_to_json(a));
        Run junk;
        CHECK(!run_from_json(t, "{\"format\":\"nope\"}", junk));
        CHECK(!run_from_json(t, "not json", junk));
    }
    { // Rerolling the shop changes neither the deal nor the spins.
        Run a(t, cfg), b(t, cfg);
        auto to_shop = [](Run& r) {
            while (r.phase != Phase::Shop && r.phase != Phase::Lost) {
                if (r.phase == Phase::Trip) {
                    if (r.must_deliver() || r.load.cargo.size() >= 3) r.deliver();
                    else r.spin(static_cast<int>(r.load.spins) % static_cast<int>(r.deal.size()));
                } else if (r.phase == Phase::Rescue) r.rescue(Rescue::Decline);
                else if (r.phase == Phase::Rod) r.rod_remove(0);
            }
        };
        to_shop(a);
        to_shop(b);
        if (a.phase == Phase::Shop && b.phase == Phase::Shop) {
            b.money += 20;
            CHECK(b.reroll() && b.reroll());
            b.money = a.money;
            a.leave_shop();
            b.leave_shop();
            CHECK(a.deal.size() == b.deal.size());
            for (std::size_t w = 0; w < a.deal.size(); ++w)
                for (std::size_t k = 0; k < a.deal[w].size(); ++k) CHECK(a.deal[w][k].uid == b.deal[w][k].uid);
            CHECK(a.spin(0).index == b.spin(0).index);
        }
    }
    { // Rigs and stakes edit the tables.
        Tables hot = apply_setup(t, "hotshot", 1);
        CHECK(hot.run.start_money == 10 && hot.round.tank == t.round.tank - 3 && hot.start.bag.size() == t.start.bag.size() + 2);
        Tables s8 = apply_setup(t, "standard", 8);
        CHECK(s8.run.max_items == t.run.max_items - 1 && s8.round.spoil_cost == 2 && s8.round.inspection_every == 2);
        CHECK(s8.targets[0] == t.targets[0] && s8.targets[1] > t.targets[1] && s8.shop.price_zap == t.shop.price_zap + 1);
    }
    { // The profile: unlocks, rigs and stakes.
        Profile p;
        CHECK(p.locked_now(t).size() == t.locked.size());
        CHECK(p.unlock("big_rig") && !p.unlock("big_rig"));
        CHECK(p.locked_now(t).size() == t.locked.size() - 1);
        CHECK(p.rig_unlocked(t, "standard") && !p.rig_unlocked(t, "tanker") && p.max_stake(t, "standard") == 1);
        p.wins = 1;
        p.best_stake["standard"] = 1;
        Profile q = profile_from_json(profile_to_json(p));
        CHECK(q.rig_unlocked(t, "tanker") && q.max_stake(t, "standard") == 2 && q.is_unlocked("big_rig"));
        CHECK(profile_from_json("garbage").wins == 0);
    }
    { // Every locked piece has a way to unlock it, and every unlock names real content.
        CHECK(t.unlocks.size() == t.locked.size());
        for (auto& u : t.unlocks) CHECK(t.item(u.id) || t.wheel(u.id) || t.upgrade(u.id) || t.consumable(u.id));
    }
    std::printf(failures ? "%d check(s) failed\n" : "all rules tests passed\n", failures);
    return failures ? 1 : 0;
}
