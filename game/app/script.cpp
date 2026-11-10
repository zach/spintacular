// Scripted demos: the game plays itself through the same actions a player
// takes, for screenshots and for soak tests in CI.
//
//   --script tour   one run with a cautious policy; screenshots of every screen
//   --script soak   several whole runs at instant speed, buying everything
//   --script late   one run with the soak's policy; screenshots from round 8 on
#include <algorithm>
#include <set>

#include "automap/diag.h"
#include "game.h"

namespace spin {

namespace {

std::set<std::string> taken;
int soak_runs = 0;

bool shot(Game& g, const std::string& name) {
    if (g.opt.shots_dir.empty() || taken.count(name) || !g.pending_shot.empty()) return false;
    taken.insert(name);
    char prefix[8];
    std::snprintf(prefix, sizeof prefix, "%02d-", ++g.script_shots);
    g.pending_shot = g.opt.shots_dir + "/" + prefix + name + ".png";
    g.script_wait = 0.05f;
    return true;
}

void hover(Game& g, float x, float y) {
    ui::input().mx = x;
    ui::input().my = y;
    (void)g;
}

int safest_wheel(Game& g) {
    Run& r = g.run;
    int best = 0;
    for (int i = 1; i < static_cast<int>(r.deal.size()); ++i)
        if (r.spoil_chance(i) < r.spoil_chance(best)) best = i;
    return best;
}

void play_round(Game& g, bool tour) {
    Run& r = g.run;
    if (g.round_over) {
        if (tour && shot(g, "round-cleared")) return;
        g.round_over = false;
        if (r.phase == Phase::Won) end_run(g, true);
        else enter_shop(g);
        return;
    }
    switch (r.phase) {
    case Phase::Rescue:
        if (tour && shot(g, "rescue")) return;
        for (Rescue how : {Rescue::NudgeLeft, Rescue::NudgeRight, Rescue::Respin, Rescue::Decline})
            if (r.rescue_allowed(how)) {
                act_rescue(g, how);
                return;
            }
        break;
    case Phase::Rod: act_rod(g, 0); break;
    case Phase::Trip: {
        int cargo = static_cast<int>(r.load.cargo.size());
        int want = g.opt.script == "tour" ? (r.round >= 3 ? 99 : 3 + (r.round >= 2 ? 1 : 0)) : 4;
        int wheel = safest_wheel(g);
        bool stop = cargo >= want || (cargo >= 2 && r.spoil_chance(wheel) > 0.45 && want < 99);
        if (tour && cargo == 0 && r.load.spins == 0) {
            hover(g, -1000, -1000);
            if (shot(g, "round-" + std::to_string(r.round + 1) + "-start")) return;
        }
        if (stop && r.can_deliver()) {
            if (tour) {
                float cx, cy, rad;
                wheel_layout(g, wheel, &cx, &cy, &rad);
                hover(g, cx + rad * 0.3f, cy - rad * 0.4f);
                if (shot(g, "load-" + std::to_string(cargo) + "-cargo")) return;
            }
            act_deliver(g);
        } else if (r.can_spin()) {
            if (r.count_consumable("band") > 0 && r.spoil_chance(wheel) >= 0.3) g.band_armed = true;
            act_spin(g, wheel);
        }
        break;
    }
    default: break;
    }
}

void play_shop(Game& g, bool tour) {
    Run& r = g.run;
    if (tour) {
        hover(g, 200, 400);
        if (shot(g, "shop-round-" + std::to_string(r.round + 1))) return;
    }
    for (int i = 0; i < static_cast<int>(r.offers.size()); ++i) {
        const Offer& o = r.offers[i];
        if (o.sold || o.price > r.money || !r.can_take(o)) continue;
        if (o.kind == OfferKind::Zap) continue;
        if (o.kind == OfferKind::Upgrade) {
            for (auto& w : r.bag)
                if (r.upgrade_fits(o, w)) {
                    if (tour && taken.count("shop-choose-wedge") == 0) {
                        g.target_offer = i;
                        if (shot(g, "shop-choose-wedge")) return;
                    }
                    if (shop_buy(g, i, w.uid, o.id == "recut" ? 6 : 0)) return;
                    break;
                }
            continue;
        }
        if (shop_buy(g, i)) return;
    }
    if (g.opt.script != "tour" && r.money >= r.reroll_price + 6 && r.m.rerolls < 40 && r.reroll()) {
        for (auto& o : r.offers) see(g, o.id);
        return;
    }
    if (tour && r.round >= 1 && shot(g, "shop-after-buying")) return;
    shop_leave(g);
}

} // namespace

void script_update(Game& g) {
    bool late = g.opt.script == "late";
    if (late && g.has_run && g.run.round >= 7 && g.profile.settings.speed == 0) {
        g.profile.settings.speed = 1;
        apply_settings(g);
    }
    bool tour = g.opt.script == "tour" || (late && g.has_run && g.run.round >= 7);
    if (g.script_step == 0) { // first frame: settle the settings
        g.profile.settings.speed = g.opt.script == "tour" ? 1 : 0;
        apply_settings(g);
        g.script_step = 1;
        g.script_wait = 0.3f;
        return;
    }
    if (g.script_wait > 0) {
        g.script_wait -= g.dt;
        return;
    }
    if (!g.pending_shot.empty()) return;
    if (!g.cues.empty()) {
        if (tour) {
            if (g.payout_open && g.payout_step >= 4) shot(g, "payout");
            else if (g.spoiled_banner) shot(g, "spoiled");
            else if (g.save_t > 1.0f) shot(g, "saved");
            else if (!g.payout_open && g.screen == Screen::Round && g.run.load.spins == 2 && g.cues.front().t > 0.2f) shot(g, "spinning");
        }
        return;
    }
    switch (g.screen) {
    case Screen::Title:
        if (g.script_step == 1) {
            hover(g, 420, 560);
            if (tour && shot(g, "title")) return;
            g.script_step = 2;
            start_run(g, g.opt.seed ? g.opt.seed : 20261110, false);
        } else if (g.script_step == 3) {
            if (g.opt.script == "tour") {
                g.screen = Screen::Collection;
                g.script_step = 4;
            } else {
                g.quit = true;
            }
        }
        break;
    case Screen::Round: play_round(g, tour); break;
    case Screen::Shop: play_shop(g, tour); break;
    case Screen::RunEnd:
        if (tour && shot(g, "run-end")) return;
        ++soak_runs;
        am::diag::log(am::diag::Level::info, "spintacular",
                      ("script: run " + std::to_string(soak_runs) + " ended at round " + std::to_string(g.run.round + 1) +
                       (g.run_won ? " (won)" : " (lost)"))
                          .c_str());
        g.has_run = false;
        g.screen = Screen::Title;
        if (g.opt.script == "soak" && soak_runs < 6) {
            g.rig_index = soak_runs % static_cast<int>(g.tables.rigs.size());
            g.profile.wins = 10; // every rig, for coverage
            start_run(g, (g.opt.seed ? g.opt.seed : 777) + soak_runs * 101, false);
        } else {
            g.script_step = 3;
        }
        break;
    case Screen::Collection:
        if (shot(g, "collection")) return;
        g.settings_open = true;
        g.screen = Screen::Title;
        g.script_step = 5;
        break;
    }
    if (g.script_step == 5 && g.screen == Screen::Title) {
        if (shot(g, "settings")) return;
        g.settings_open = false;
        g.help_open = true;
        g.script_step = 6;
    } else if (g.script_step == 6) {
        if (shot(g, "how-to-play")) return;
        g.quit = true;
    }
}

} // namespace spin
