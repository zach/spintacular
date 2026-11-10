// The round screen: wheels across the middle, the wagon below, and the one
// decision the game is about: one more spin, or deliver?
// docs/design/presentation.md.
#include <algorithm>
#include <cmath>

#include <SDL3/SDL.h>

#include "game.h"

namespace spin {

using namespace gfx;

namespace {

constexpr float kWheelY = 452;
constexpr float kWheelR = 138;
constexpr float kBedY = 884;      // the top of the wagon's bed
constexpr float kWagonX = 440, kWagonW = 1040;
constexpr float kSlotX = 500, kSlotGap = 106, kTokenSize = 88;

// The angular layout of a wheel's wedges: start and width of each, in wheel space.
void wedge_arcs(const Game& g, int wheel, std::vector<float>& start, std::vector<float>& width) {
    const auto& wedges = g.run.deal[wheel];
    const WheelView& v = g.wheels[wheel];
    int n = static_cast<int>(wedges.size());
    float total = 0;
    width.assign(n, 1.0f);
    for (int i = 0; i < n; ++i) {
        if (v.band && i < static_cast<int>(v.band_clash.size()) && v.band_clash[i]) width[i] = 0.5f;
        total += width[i];
    }
    start.assign(n, 0.0f);
    float a = 0;
    for (int i = 0; i < n; ++i) {
        width[i] *= 2 * kPi / total;
        start[i] = a;
        a += width[i];
    }
}

float wrap(float a) {
    a = std::fmod(a, 2 * kPi);
    return a < 0 ? a + 2 * kPi : a;
}

// The wedge under the pointer for a wheel rotation.
int slot_under_pointer(const std::vector<float>& start, const std::vector<float>& width, float angle) {
    float local = wrap(-angle);
    for (int i = 0; i < static_cast<int>(start.size()); ++i)
        if (local >= start[i] && local < start[i] + width[i]) return i;
    return static_cast<int>(start.size()) - 1;
}

double shown_chance(Game& g, int wheel) {
    Run& r = g.run;
    if (r.covered(wheel, g.shown_load)) return 0;
    const auto& wedges = r.deal[wheel];
    int bad = 0;
    for (auto& w : wedges) bad += r.clashes(wheel, w, g.shown_load);
    return wedges.empty() ? 0 : static_cast<double>(bad) / wedges.size();
}

void slot_position(int index, float* x, float* y) {
    *x = kSlotX + index * kSlotGap;
    *y = kBedY - kTokenSize * 0.5f - 6;
}

void finish_action(Game& g);

void drop_load(Game& g) {
    for (int i = 0; i < static_cast<int>(g.shown_load.cargo.size()); ++i) {
        const Cargo& c = g.shown_load.cargo[i];
        Flyer f;
        f.wedge.kind = WedgeKind::Cargo;
        f.wedge.color = c.color;
        f.wedge.value = c.value;
        f.wedge.bonus = c.bonus;
        f.value = c.value;
        f.falling = true;
        slot_position(i, &f.x, &f.y);
        f.vx = 260 + i * 90.0f;
        f.vy = -520 - (i % 3) * 120.0f;
        f.size1 = kTokenSize;
        g.flyers.push_back(f);
    }
    g.shown_load = Load();
}

// What happens once the wheel has stopped on its wedge.
void resolve_landing(Game& g, SpinResult r, Load after) {
    Run& run = g.run;
    float cx, cy, rad;
    wheel_layout(g, r.wheel, &cx, &cy, &rad);
    WheelView& view = g.wheels[r.wheel];
    view.band = false;
    const Wedge wedge = run.deal[r.wheel][r.index];
    switch (r.landing) {
    case Landing::Cargo:
    case Landing::Tarp:
    case Landing::Crate:
    case Landing::Blank: {
        view.flash = 1;
        view.flash_color = ui::kSafe;
        if (r.landing == Landing::Blank) {
            sfx(g, audio::Sfx::LandBlank, 0.7f);
            popup(g, "Empty", cx, cy + 20, ui::kMuted, 40, 0.8f);
            cue(g, 0.25f, nullptr, nullptr, [&g, after] {
                g.shown_load = after;
                finish_action(g);
            });
            break;
        }
        Flyer f;
        f.wedge = wedge;
        f.value = r.landing == Landing::Cargo ? run.landed_value(r.wheel, wedge) : wedge.value;
        f.x0 = cx;
        f.y0 = cy - rad * 0.62f;
        f.size0 = rad * 0.42f;
        f.size1 = kTokenSize;
        if (r.landing == Landing::Cargo)
            slot_position(static_cast<int>(after.cargo.size()) - 1, &f.x1, &f.y1);
        else if (r.landing == Landing::Crate) {
            f.x1 = 1310;
            f.y1 = kBedY - 50 - (after.crates - 1) * 70.0f;
        } else {
            f.x1 = 1420;
            f.y1 = kBedY + 40;
        }
        f.dur = 0.32f * std::max(g.speed, 0.01f);
        if (g.speed > 0) g.flyers.push_back(f);
        bool near = r.near_miss;
        cue(g, 0.32f, nullptr, nullptr, [&g, r, after, near, cx, cy] {
            Run& run = g.run;
            long long before_points = 0, after_points = 0;
            for (auto& c : g.shown_load.cargo) before_points += c.value + c.bonus;
            for (auto& c : after.cargo) after_points += c.value + c.bonus;
            g.shown_load = after;
            g.shown_spins = run.spins_left();
            g.wagon_bounce = 1;
            if (r.landing == Landing::Cargo) {
                int n = static_cast<int>(after.cargo.size());
                sfx(g, audio::Sfx::LandCargo, 0.9f, 0.8f + 0.09f * n);
                if (run.quota > 0) {
                    float frac = static_cast<float>(after_points) / run.quota;
                    if (before_points < run.quota && after_points >= run.quota) {
                        sfx(g, audio::Sfx::QuotaBell, 0.9f);
                        popup(g, "QUOTA!", 960, kBedY + 40, ui::kGold, 60);
                        g.quota_glow = 1;
                    } else if (frac >= 0.5f && frac < 1) {
                        sfx(g, audio::Sfx::Whistle, 0.5f, 0.7f + frac * 0.8f);
                    }
                }
                if (r.chance >= 0.5) unlock_check(g, "risky_land", static_cast<int>(r.chance * 100 + 0.5));
            } else if (r.landing == Landing::Tarp) {
                sfx(g, audio::Sfx::LandTarp, 0.9f);
            } else {
                sfx(g, audio::Sfx::LandCrate, 0.9f);
            }
            if (near) popup(g, "Close!", cx, cy + 20, ui::kWarn, 46, 0.9f);
            finish_action(g);
        });
        break;
    }
    case Landing::Padded:
    case Landing::Clutch:
    case Landing::TarpSave: {
        view.flash = 1;
        view.flash_color = ui::kGold;
        g.save_banner = r.landing == Landing::Padded ? "THE PADDED WHEEL THROWS IT OUT!" : r.landing == Landing::Clutch ? "DOUBLE CLUTCH!" : "A TARP COVERS IT!";
        g.save_t = 1.6f;
        sfx(g, audio::Sfx::Save, 1.0f);
        g.shake = 0.35f;
        if (r.landing == Landing::TarpSave) unlock_check(g, "tarp_save", static_cast<int>(after.cargo.size()));
        cue(g, 0.9f, nullptr, nullptr, [&g, after] {
            g.shown_load = after;
            finish_action(g);
        });
        break;
    }
    case Landing::Rescue:
        view.flash = 1;
        view.flash_color = ui::kDanger;
        sfx(g, audio::Sfx::Refuse, 0.8f);
        finish_action(g); // the rescue prompt comes up
        break;
    case Landing::Spoiled: {
        view.flash = 1;
        view.flash_color = ui::kDanger;
        sfx(g, audio::Sfx::Spoil, 1.0f);
        audio::music(0, false);
        g.shake = 0.8f;
        g.spoiled_banner = true;
        g.spoiled_value = r.lost_value;
        g.spoiled_t = 1.7f;
        drop_load(g);
        if (r.insurance_pay > 0) popup(g, "Insurance pays " + ui::number(r.insurance_pay), 960, 640, ui::kSafe, 44, 1.8f);
        if (r.spoil_cost > 0) popup(g, "-$" + std::to_string(r.spoil_cost), kWidth - 120, 150, ui::kDanger, 44, 1.6f);
        cue(g, 1.5f, nullptr, nullptr, [&g] {
            g.spoiled_banner = false;
            finish_action(g);
        });
        break;
    }
    }
}

// Turns the wheel to the wedge the rules landed on, ticking past the others.
void animate_spin(Game& g, SpinResult r, Load after, float turns, float dur) {
    std::vector<float> start, width;
    wedge_arcs(g, r.wheel, start, width);
    WheelView& v = g.wheels[r.wheel];
    // Stop somewhere inside the wedge, not always dead center: near the edges is where the tension is.
    float jitter = (static_cast<float>((g.frame_no * 7919 + r.index * 104729) % 1000) / 1000.0f - 0.5f) * 0.7f;
    float local = start[r.index] + width[r.index] * (0.5f + jitter);
    float from = v.angle;
    float delta = wrap(-local - from) + 2 * kPi * turns;
    if (turns == 0 && delta > kPi) delta -= 2 * kPi; // a nudge goes the short way
    v.spinning = true;
    v.last_slot = slot_under_pointer(start, width, from);
    cue(
        g, dur, nullptr,
        [&g, r, from, delta, start, width](float t) {
            WheelView& v = g.wheels[r.wheel];
            float e = 1 - std::pow(1 - t, 3.2f);
            v.angle = from + delta * e;
            int slot = slot_under_pointer(start, width, v.angle);
            if (slot != v.last_slot) {
                v.last_slot = slot;
                v.tilt = 1;
                sfx(g, audio::Sfx::Tick, 0.35f + 0.5f * t, 0.9f + 0.25f * (1 - t));
            }
        },
        [&g, r, after] {
            WheelView& v = g.wheels[r.wheel];
            v.spinning = false;
            v.angle = wrap(v.angle);
            resolve_landing(g, r, after);
        });
}

void finish_action(Game& g) {
    Run& run = g.run;
    g.shown_money = run.money;
    g.shown_spins = run.spins_left();
    g.shown_trips = run.trips_left;
    unlock_check(g, "hold_money", run.money);
    save_run(g);
    if (run.phase == Phase::Trip && run.must_deliver() && !g.payout_open) {
        if (run.truck_full()) {
            popup(g, "FULL TRUCK!", 960, 620, ui::kGold, 84, 1.4f);
            sfx(g, audio::Sfx::FullTruck, 1.0f);
            audio::music(6, false);
            wait(g, 0.9f);
        } else if (!run.load.cargo.empty()) {
            popup(g, "Tank's empty: delivering", 960, 640, ui::kCream, 44, 1.2f);
            wait(g, 0.5f);
        }
        cue(g, 0, [&g] { act_deliver(g); });
        return;
    }
    if (run.phase == Phase::Lost) {
        g.shown_score = run.round_score;
        cue(g, 1.0f, nullptr, nullptr, [&g] { end_run(g, false); });
        return;
    }
    if (run.phase == Phase::Shop || run.phase == Phase::Won) {
        g.shown_score = run.round_score;
        g.shown_load = Load();
        unlock_check(g, "clear_round", run.round + 1);
        unlock_check(g, "trips_to_spare", std::max(0, run.trips_left));
        g.profile.best_round = std::max(g.profile.best_round, run.round + 1);
        cue(g, 0.4f, nullptr, nullptr, [&g] {
            g.round_over = true;
            sfx(g, audio::Sfx::RoundWon, 0.9f);
        });
        return;
    }
    if (run.phase == Phase::Trip && g.cues.empty()) sync_shown(g);
}

void draw_wheel(Game& g, int wheel, bool interactive) {
    Run& run = g.run;
    float cx, cy, R;
    wheel_layout(g, wheel, &cx, &cy, &R);
    WheelView& v = g.wheels[wheel];
    const WheelDef& def = run.wheel_def(wheel);
    const auto& wedges = run.deal[wheel];
    int n = static_cast<int>(wedges.size());
    std::vector<float> start, width;
    wedge_arcs(g, wheel, start, width);
    double chance = shown_chance(g, wheel);
    bool can = interactive && run.can_spin();
    float dist = std::hypot(ui::input().mx - cx, ui::input().my - cy);
    bool over = can && !ui::input().blocked && dist < R + 14;

    // The stand.
    quad({cx - 26, cy}, {cx + 26, cy}, {cx + 70, cy + R + 70}, {cx - 70, cy + R + 70}, ui::kInk);
    quad({cx - 16, cy}, {cx + 16, cy}, {cx + 58, cy + R + 64}, {cx - 58, cy + R + 64}, ui::kWoodDark);
    rrect(cx - 96, cy + R + 56, 192, 22, 8, ui::kInk);
    rrect(cx - 90, cy + R + 60, 180, 14, 5, ui::kWood);

    // The rim: its color tells the wheel's kind.
    Color rim = ui::kRed;
    if (def.effect == WheelEffect::Gilded) rim = rgb(0xD9A21E);
    else if (def.effect == WheelEffect::Bump) rim = rgb(0x7C4DCC);
    else if (def.effect == WheelEffect::Express) rim = rgb(0x2E9E8F);
    else if (def.effect == WheelEffect::Padded) rim = rgb(0xD96C9A);
    if (v.flash > 0) circle(cx, cy, R + 30 + v.flash * 16, v.flash_color.with_alpha(0.28f * v.flash));
    if (over) circle(cx, cy, R + 26, ui::kCream.with_alpha(0.22f));
    circle(cx, cy, R + 18, ui::kInk);
    circle(cx, cy, R + 13, rim);
    circle(cx, cy, R + 1, ui::kInk);

    int under = slot_under_pointer(start, width, v.angle);
    for (int i = 0; i < n; ++i) {
        const Wedge& w = wedges[i];
        bool clash = run.clashes(wheel, w, g.shown_load);
        bool safe_anyway = clash && run.covered(wheel, g.shown_load);
        float a0 = v.angle + start[i], a1 = a0 + width[i];
        Color bg = i % 2 == 0 ? ui::kCream : rgb(0xEAD7AC);
        if (w.kind == WedgeKind::Blank) bg = rgb(0x8E8AA0);
        if (clash) bg = safe_anyway ? rgb(0x5B5770) : rgb(0x2A2638);
        if (v.spinning && i == under) bg = clash && !safe_anyway ? mix(bg, ui::kDanger, 0.55f) : mix(bg, ui::kSafe, 0.45f);
        sector(cx, cy, 0, R - 2, a0 + 0.012f, a1 - 0.012f, bg);
        float mid = (a0 + a1) * 0.5f;
        float tr = R * (n <= 3 ? 0.56f : 0.62f);
        float tx = cx + std::sin(mid) * tr, ty = cy - std::cos(mid) * tr;
        float size = std::min(R * (n <= 3 ? 0.5f : 0.4f), width[i] * tr * 0.78f);
        int shown = w.kind == WedgeKind::Cargo ? run.landed_value(wheel, w) : -1;
        ui::wedge_token(tx, ty, size, w, shown, clash && !safe_anyway);
        if (clash && !safe_anyway) ui::cross(tx, ty, size * 1.25f, ui::kDanger, std::max(5.0f, size * 0.13f));
        if (def.effect == WheelEffect::Bump && w.kind == WedgeKind::Cargo && w.value < run.t.run.max_value)
            text(Font::Display, size * 0.34f, tx + size * 0.62f, ty - size * 0.36f, "+1", rgb(0x7C4DCC), Align::Center);
    }
    // Pegs between wedges, and the hub.
    for (int i = 0; i < n; ++i) {
        float a = v.angle + start[i];
        circle(cx + std::sin(a) * (R + 6), cy - std::cos(a) * (R + 6), 6, ui::kInk);
        circle(cx + std::sin(a) * (R + 6), cy - std::cos(a) * (R + 6), 3.5f, ui::kCream);
        line({cx, cy}, {cx + std::sin(a) * R, cy - std::cos(a) * R}, 4, ui::kInk);
    }
    circle(cx, cy, R * 0.17f + 4, ui::kInk);
    circle(cx, cy, R * 0.17f, ui::kGold);
    circle(cx, cy, R * 0.07f, ui::kInk);

    // Bulbs around the rim: they chase while the wheel turns, and show the risk when it's still.
    int bulbs = 14;
    int chase = static_cast<int>(ui::time() * 12);
    for (int i = 0; i < bulbs; ++i) {
        float a = 2 * kPi * (i + 0.5f) / bulbs;
        bool lit = v.spinning ? (i + chase) % 3 == 0 : (over ? true : i % 2 == 0);
        ui::bulb(cx + std::sin(a) * (R + 13), cy - std::cos(a) * (R + 13), 6, lit, v.spinning ? ui::kGold : mix(ui::kGold, ui::risk_color(chance), 0.5f));
    }

    // The flapper.
    push(cx, cy - R - 20, -v.tilt * 0.5f, 1);
    tri({-17, -24}, {17, -24}, {0, 28}, ui::kInk);
    tri({-11, -20}, {11, -20}, {0, 20}, rgb(0x8A5A32));
    circle(0, -20, 9, ui::kInk);
    circle(0, -20, 5, ui::kGold);
    pop();

    // The chance to spoil the load, above the wheel: a number, always.
    Color rc = ui::risk_color(chance);
    char pct[16];
    std::snprintf(pct, sizeof pct, "%d%%", static_cast<int>(chance * 100 + 0.5));
    float ly = cy - R - 50;
    panel(cx - 104, ly - 44, 208, 50, 25, ui::kNight, rc, 5);
    float pw = text_width(Font::Display, 34, pct);
    const char* tail = chance <= 0 ? "safe" : "to spoil";
    float tw = text_width(Font::Text, 20, tail);
    float px = cx - (pw + 10 + tw) * 0.5f;
    text(Font::Display, 34, px, ly - 7, pct, rc);
    text(Font::Text, 20, px + pw + 10, ly - 10, tail, rc);

    // Its name and key below.
    float ny = cy + R + 110;
    bool free = def.effect == WheelEffect::Express && std::find(run.free_spun.begin(), run.free_spun.end(), wheel) == run.free_spun.end();
    std::string label = def.name + (free ? ": free spin" : "");
    float lw = text_width(Font::Display, 22, label);
    float lx = cx - (lw + 46) * 0.5f;
    panel(lx, ny - 27, 36, 34, 8, can ? ui::kCream : rgb(0x4A4F6B), ui::kInk, 3);
    text(Font::Display, 22, lx + 18, ny - 2, std::to_string(wheel + 1), ui::kInk, Align::Center);
    text(Font::Display, 22, lx + 46, ny - 2, label, def.effect == WheelEffect::None ? ui::kMuted : lighten(rim, 0.3f));

    if (over) {
        audio::risk(static_cast<float>(chance));
        // Which wedge is the mouse on?
        std::string body = def.text + ".";
        if (dist < R) {
            float a = wrap(std::atan2(ui::input().mx - cx, -(ui::input().my - cy)) - v.angle);
            for (int i = 0; i < n; ++i)
                if (a >= start[i] && a < start[i] + width[i]) {
                    const Wedge& w = wedges[i];
                    body += "\n" + ui::wedge_name(w);
                    if (run.clashes(wheel, w, g.shown_load))
                        body += run.covered(wheel, g.shown_load) ? ": a repeat, but it would be covered." : ": a repeat. It would spoil the load.";
                    else if (w.kind == WedgeKind::Tarp) body += ": covers one repeat.";
                    else if (w.kind == WedgeKind::Crate) body += ": x1.5 to the load.";
                    else if (w.kind == WedgeKind::Blank) body += ": nothing.";
                    else body += ": safe to land.";
                }
        }
        body += "\n" + std::string(pct) + " to spoil the load. Click, or press " + std::to_string(wheel + 1) + ", to spin.";
        ui::tooltip(def.name, body, rc);
        if (ui::input().pressed) act_spin(g, wheel);
    }
}

void draw_wagon(Game& g) {
    Run& run = g.run;
    const Load& load = g.shown_load;
    int n = static_cast<int>(load.cargo.size());
    float sink = std::min(n, 7) * 2.5f + std::sin(g.wagon_bounce * kPi) * 9;
    long long points = 0;
    for (auto& c : load.cargo) points += c.value + c.bonus;
    float frac = run.quota > 0 ? std::min(1.0f, static_cast<float>(points) / run.quota) : 0;

    // Ground.
    rect(0, 1044, kWidth, 36, rgb(0x070A1A));
    // Wheels behind the body.
    for (float wx : {kWagonX + 190.0f, kWagonX + kWagonW - 190.0f}) {
        circle(wx, 986, 58, ui::kInk);
        circle(wx, 986, 51, ui::kGold);
        circle(wx, 986, 38, ui::kRedDark);
        for (int s = 0; s < 8; ++s) {
            float a = kPi * s / 8;
            line({wx - std::sin(a) * 38, 986 + std::cos(a) * 38}, {wx + std::sin(a) * 38, 986 - std::cos(a) * 38}, 5, ui::kGold);
        }
        circle(wx, 986, 11, ui::kInk);
    }
    push(0, sink);
    // Hitch.
    rrect(kWagonX - 110, kBedY + 46, 130, 18, 6, ui::kInk);
    rrect(kWagonX - 106, kBedY + 50, 122, 10, 4, ui::kWood);
    // Body: a red side panel with cream trim, the quota line painted on it.
    panel(kWagonX, kBedY, kWagonW, 96, 14, ui::kRed, ui::kInk, 5);
    rrect(kWagonX + 12, kBedY + 12, kWagonW - 24, 72, 8, ui::kRedDark);
    // The bed plank.
    panel(kWagonX - 14, kBedY - 16, kWagonW + 28, 24, 8, ui::kWood, ui::kInk, 5);
    // Quota fill line.
    float qx = kWagonX + 150, qw = kWagonW - 300, qy = kBedY + 34, qh = 34;
    bool close = frac >= 0.6f && frac < 1;
    float pulse = close ? 0.5f + 0.5f * std::sin(static_cast<float>(ui::time()) * 9) : 0;
    if (close || frac >= 1) rrect(qx - 12, qy - 12, qw + 24, qh + 24, 20, ui::kGold.with_alpha(frac >= 1 ? 0.35f : 0.12f + 0.2f * pulse));
    panel(qx, qy, qw, qh, 10, ui::kNight, ui::kInk, 4);
    if (frac > 0) rrect(qx, qy, std::max(20.0f, qw * frac), qh, 10, frac >= 1 ? ui::kGold : mix(ui::kCream, ui::kGold, frac));
    std::string q = "QUOTA  " + std::to_string(points) + " / " + std::to_string(run.quota) + (frac >= 1 ? "   PAYS DOUBLE" : "");
    text_outlined(Font::Display, 24, qx + qw * 0.5f, qy + 26, q, ui::kCream, ui::kInk, 2.5f, Align::Center);
    int bulbs = 16;
    ui::bulb_row(qx + 10, qy - 14, qx + qw - 10, qy - 14, bulbs, static_cast<int>(frac * bulbs + 0.001f), 6);
    if (ui::hover(qx, qy - 20, qw, qh + 30))
        ui::tooltip("The quota", "A load whose points reach the quota pays double. Points are the cargo's values, plus bonus points.");

    // Cargo on the bed, in the order it landed.
    for (int i = 0; i < n; ++i) {
        float x, y;
        slot_position(i, &x, &y);
        const Cargo& c = load.cargo[i];
        ui::cargo(x, y, kTokenSize, c.color, c.value, c.bonus);
    }
    int size = run.t.scoring.truck_size;
    for (int i = n; i < size; ++i) {
        float x, y;
        slot_position(i, &x, &y);
        circle(x, y + 8, 10, rgb(0xFFFFFF, 0.1f));
    }
    // Crates stack at the back; tarps hang on the side.
    for (int i = 0; i < load.crates; ++i) ui::crate_icon(1310, kBedY - 50 - i * 70.0f, 72);
    int tarps = load.tarps + (run.inspection() == Inspection::NoTarps ? 0 : run.spare_tarps);
    for (int i = 0; i < tarps; ++i) ui::tarp_icon(1420 - i * 26.0f, kBedY + 46 + i * 4.0f, 64);
    if (tarps > 0 && ui::hover(1340, kBedY + 10, 130, 76)) ui::tooltip("Tarps", std::to_string(tarps) + " ready. A tarp covers one repeat, then it's used up.");
    pop();
}

void draw_gauges(Game& g) {
    Run& run = g.run;
    // Fuel: spins left in this trip's tank, and trips left this round.
    float x = 36, y = 700;
    panel(x, y, 370, 170, 16, ui::kNight2, ui::kInk, 4);
    text(Font::Display, 24, x + 20, y + 38, "TANK", ui::kCream);
    text(Font::Text, 20, x + 350, y + 36, std::to_string(std::max(0, g.shown_spins)) + " spins", ui::kMuted, Align::Right);
    int tank = run.tank;
    float pw = std::min(30.0f, 330.0f / std::max(1, tank));
    for (int i = 0; i < tank; ++i) {
        bool full = i < g.shown_spins;
        rrect(x + 20 + i * pw, y + 52, pw - 6, 30, 6, full ? ui::kGold : rgb(0x2C3358));
    }
    text(Font::Display, 24, x + 20, y + 124, "TRIPS", ui::kCream);
    for (int i = 0; i < run.t.round.trips; ++i) {
        bool left = i < g.shown_trips;
        float tx = x + 130 + i * 56.0f;
        rrect(tx, y + 100, 44, 28, 6, left ? ui::kRed : rgb(0x2C3358));
        circle(tx + 10, y + 132, 7, left ? ui::kGold : rgb(0x2C3358));
        circle(tx + 34, y + 132, 7, left ? ui::kGold : rgb(0x2C3358));
    }
    if (ui::hover(x, y, 370, 170))
        ui::tooltip("Fuel", "Each trip has a tank of spins. When it's empty, the load delivers. The round has " + std::to_string(run.t.round.trips) +
                                " trips to reach the target; each trip left over pays $" + std::to_string(run.t.income.per_unused_trip) + ".");
    // Protections in play.
    float py = y + 186;
    if (run.has(ItemEffect::DoubleClutch)) {
        bool ready = !g.shown_load.clutch_used;
        panel(x, py, 370, 44, 22, ready ? rgb(0x2FA865) : rgb(0x2C3358), ui::kInk, 4);
        text(Font::Display, 22, x + 185, py + 30, ready ? "Double Clutch ready" : "Double Clutch used", ui::kCream, Align::Center);
        py += 56;
    }
    if (run.inspection() == Inspection::NoTarps) {
        panel(x, py, 370, 44, 22, ui::kGold, ui::kInk, 4);
        text(Font::Display, 22, x + 185, py + 30, "High winds: no tarps", ui::kInk, Align::Center);
    }
}

void draw_load_value(Game& g) {
    Run& run = g.run;
    float x = 1524, y = 700, w = 360;
    panel(x, y, w, 170, 16, ui::kNight2, ui::kInk, 4);
    Score s = run.score(g.shown_load);
    bool any = !g.shown_load.cargo.empty();
    text(Font::Display, 24, x + 20, y + 38, "DELIVER NOW FOR", ui::kCream);
    ui::sign(any ? 76 : 60, x + w * 0.5f, y + 116, any ? ui::number(s.total) : "0", s.quota_hit ? ui::kGold : ui::kCream);
    if (any) {
        char buf[96];
        std::string fac = s.factor > 1 ? " x " + std::to_string(s.factor).substr(0, s.factor == std::floor(s.factor) ? 1 : 3) : "";
        std::snprintf(buf, sizeof buf, "%.0f points x %.4g mult%s", s.points, s.mult, fac.c_str());
        text(Font::Text, 20, x + w * 0.5f, y + 152, buf, ui::kMuted, Align::Center);
    } else {
        text(Font::Text, 20, x + w * 0.5f, y + 152, "spin a wheel to load the wagon", ui::kMuted, Align::Center);
    }
    int freight = run.t.income.freight_bonus * std::max(0, static_cast<int>(g.shown_load.cargo.size()) - run.t.income.freight_past);
    if (freight > 0) ui::money(30, x + w - 16, y + 40, freight, Align::Right);
    if (ui::hover(x, y, w, 170))
        ui::tooltip("What the load pays", "Points x mult, then the quota and full-truck bonuses. Each cargo past the third also pays $1 freight. "
                                          "A spoiled load pays nothing.");
    bool can = run.can_deliver() && g.cues.empty();
    if (ui::button(x, y + 190, w, 104, "DELIVER", ui::Style::Gold, can, 52) || (can && ui::key(SDL_SCANCODE_SPACE))) act_deliver(g);
    text(Font::Text, 18, x + w * 0.5f, y + 322, "Space", ui::kMuted, Align::Center);
}

void draw_payout(Game& g) {
    if (!g.payout_open) return;
    const Delivery& d = g.payout;
    float w = 780, h = 600, x = (kWidth - w) * 0.5f, y = 118;
    rect(0, 105, kWidth, 590, rgb(0x050817, 0.72f));
    ui::ticket(x, y, w, h);
    ui::stripes(x + 8, y + 8, w - 16, 14, 40);
    text(Font::Display, 40, x + w * 0.5f, y + 70, "DELIVERY", ui::kInk, Align::Center);
    int shown = std::min(g.payout_step, static_cast<int>(d.steps.size()) - 1);
    int rows = static_cast<int>(d.steps.size());
    float lh = std::min(30.0f, 360.0f / std::max(1, rows));
    for (int i = 0; i <= shown; ++i) {
        const Step& s = d.steps[i];
        if (s.kind == Step::Kind::Total) continue;
        bool cur = i == shown;
        float ly = y + 108 + i * lh;
        if (cur) rrect(x + 30, ly - lh * 0.74f, w - 60, lh, 6, ui::kGold.with_alpha(0.55f));
        Color c = s.kind == Step::Kind::Quota || s.kind == Step::Kind::FullTruck ? rgb(0xA35B00) : s.kind == Step::Kind::Item ? rgb(0x7B2D8E) : ui::kInk;
        text(Font::Text, lh * 0.78f, x + 48, ly, s.label, c);
        char run_buf[64];
        if (s.mult > 0)
            std::snprintf(run_buf, sizeof run_buf, "%.0f \xC3\x97 %.4g", s.points, s.mult * s.factor);
        else
            std::snprintf(run_buf, sizeof run_buf, "%.0f points", s.points);
        text(Font::Text, lh * 0.78f, x + w - 48, ly, run_buf, rgb(0x6B5F70), Align::Right);
    }
    if (shown >= 0) {
        const Step& s = d.steps[shown];
        float by = y + h - 96;
        rect(x + 30, by - 58, w - 60, 3, ui::kInk);
        float pulse = 1 + g.payout_pulse * 0.12f;
        char buf[96];
        if (s.kind == Step::Kind::Total) {
            ui::sign(72 * pulse, x + w * 0.5f, by + 30, ui::number(d.score.total), ui::kGold);
            std::snprintf(buf, sizeof buf, "%.0f points x %.4g mult%s", d.score.points, d.score.mult, d.score.factor > 1 ? " x bonus" : "");
            text(Font::Text, 22, x + w * 0.5f, by + 68, buf, rgb(0x6B5F70), Align::Center);
        } else {
            std::snprintf(buf, sizeof buf, "%.0f", s.points);
            ui::sign(60 * pulse, x + w * 0.5f - 60, by + 30, buf, rgb(0x7FD0FF), Align::Right);
            text(Font::Text, 56, x + w * 0.5f, by + 26, "\xC3\x97", ui::kInk, Align::Center);
            std::snprintf(buf, sizeof buf, "%.4g", std::max(1.0, s.mult) * s.factor);
            ui::sign(60 * pulse, x + w * 0.5f + 60, by + 30, buf, rgb(0xFF8C7A), Align::Left);
            text(Font::Text, 20, x + w * 0.5f - 60, by + 62, "points", rgb(0x6B5F70), Align::Right);
            text(Font::Text, 20, x + w * 0.5f + 60, by + 62, "mult", rgb(0x6B5F70), Align::Left);
        }
    }
    text(Font::Text, 18, x + w * 0.5f, y + h + 30, "click to skip", ui::kMuted, Align::Center);
}

void draw_rescue(Game& g) {
    Run& run = g.run;
    ui::Input& in = ui::input();
    float w = 860, h = 330, x = (kWidth - w) * 0.5f, y = 250;
    rect(0, 105, kWidth, kHeight - 105, rgb(0x050817, 0.6f));
    panel(x, y, w, h, 18, ui::kCream, ui::kInk, 6);
    ui::stripes(x, y, w, 18, 43);
    const Wedge& wedge = run.deal[run.rescue_wheel][run.rescue_index];
    ui::sign(54, x + w * 0.5f, y + 86, "A REPEAT!", ui::kDanger);
    ui::wedge_token(x + 100, y + 150, 84, wedge, wedge.kind == WedgeKind::Cargo ? run.landed_value(run.rescue_wheel, wedge) : -1);
    long long value = run.score(run.load).total;
    text_wrapped(Font::Text, 26, x + 170, y + 140, w - 200,
                 ui::wedge_name(wedge) + " would spoil the load, worth " + ui::number(value) + " now. Use a consumable to save it?", ui::kInk);
    float bw = 190, bx = x + 30, by = y + 220;
    in.blocked = false;
    if (ui::button(bx, by, bw, 76, "< Nudge", ui::Style::Green, run.rescue_allowed(Rescue::NudgeLeft), 30)) act_rescue(g, Rescue::NudgeLeft);
    if (ui::button(bx + bw + 14, by, bw, 76, "Nudge >", ui::Style::Green, run.rescue_allowed(Rescue::NudgeRight), 30)) act_rescue(g, Rescue::NudgeRight);
    if (ui::button(bx + 2 * (bw + 14), by, bw, 76, "Respin", ui::Style::Gold, run.rescue_allowed(Rescue::Respin), 30)) act_rescue(g, Rescue::Respin);
    if (ui::button(bx + 3 * (bw + 14), by, bw, 76, "Let it spoil", ui::Style::Red, true, 26)) act_rescue(g, Rescue::Decline);
    in.blocked = true;
}

void draw_rod(Game& g) {
    Run& run = g.run;
    ui::Input& in = ui::input();
    float w = 900, h = 330, x = (kWidth - w) * 0.5f, y = 250;
    rect(0, 105, kWidth, kHeight - 105, rgb(0x050817, 0.6f));
    panel(x, y, w, h, 18, ui::kCream, ui::kInk, 6);
    ui::stripes(x, y, w, 18, 45);
    text(Font::Display, 40, x + w * 0.5f, y + 78, "THE ROD", ui::kInk, Align::Center);
    text(Font::Text, 24, x + w * 0.5f, y + 116, "Take one delivered wedge out of your bag for good, or keep them all.", ui::kInk, Align::Center);
    in.blocked = false;
    int n = static_cast<int>(run.delivered_uids.size());
    for (int i = 0; i < n; ++i) {
        Wedge* wedge = run.find_wedge(run.delivered_uids[i]);
        if (!wedge) continue;
        float tx = x + w * 0.5f + (i - (n - 1) * 0.5f) * 110, ty = y + 180;
        bool over = ui::hover(tx - 50, ty - 50, 100, 100);
        if (over) circle(tx, ty, 58, ui::kGold.with_alpha(0.6f));
        ui::wedge_token(tx, ty, 80, *wedge);
        if (over) {
            ui::tooltip(ui::wedge_name(*wedge), "Click to remove it from your bag.");
            if (in.pressed) {
                act_rod(g, wedge->uid);
                break;
            }
        }
    }
    if (g.run.phase == Phase::Rod && ui::button(x + w * 0.5f - 150, y + 240, 300, 70, "Keep them all", ui::Style::Red, true, 30)) act_rod(g, 0);
    in.blocked = true;
}

void draw_round_over(Game& g) {
    Run& run = g.run;
    ui::Input& in = ui::input();
    bool won_run = run.phase == Phase::Won;
    float w = 700, h = won_run ? 380 : 560, x = (kWidth - w) * 0.5f, y = 200;
    rect(0, 105, kWidth, kHeight - 105, rgb(0x050817, 0.66f));
    ui::ticket(x, y, w, h);
    ui::stripes(x + 8, y + 8, w - 16, 14, 40);
    ui::bulb_row(x + 30, y - 10, x + w - 30, y - 10, 18, 0, 8, static_cast<int>(ui::time() * 6));
    ui::sign(54, x + w * 0.5f, y + 96, won_run ? "THE LAST ROUND IS CLEARED!" : "ROUND " + std::to_string(run.round + 1) + " CLEARED!", ui::kGold);
    text(Font::Text, 26, x + w * 0.5f, y + 140, ui::number(run.round_score) + " scored against a target of " + ui::number(run.target()),
         ui::kInk, Align::Center);
    in.blocked = false;
    if (won_run) {
        text(Font::Text, 26, x + w * 0.5f, y + 200, "Twelve rounds, and the show never missed a stop.", ui::kInk, Align::Center);
        if (ui::button(x + w * 0.5f - 190, y + h - 120, 380, 84, "TAKE A BOW", ui::Style::Gold, true, 40) || ui::key(SDL_SCANCODE_RETURN)) {
            g.round_over = false;
            end_run(g, true);
        }
    } else {
        const Income& inc = run.last_income;
        struct Row {
            const char* label;
            int v;
        } rows[] = {{"Round cleared", inc.round}, {"Trips to spare", inc.unused}, {"Interest ($1 per $5 held)", inc.interest},
                    {"Piggy Bank", inc.piggy}, {"Spendthrift", inc.spendthrift}};
        float ly = y + 200;
        for (auto& row : rows) {
            if (row.v == 0 && (row.label[0] == 'P' || row.label[0] == 'S')) continue;
            text(Font::Text, 26, x + 70, ly, row.label, ui::kInk);
            text(Font::Display, 28, x + w - 70, ly, "$" + std::to_string(row.v), row.v > 0 ? rgb(0x1E7B45) : rgb(0x8A8090), Align::Right);
            ly += 42;
        }
        rect(x + 60, ly - 22, w - 120, 3, ui::kInk);
        text(Font::Display, 30, x + 70, ly + 22, "Paid", ui::kInk);
        text(Font::Display, 36, x + w - 70, ly + 24, "$" + std::to_string(inc.total), rgb(0x1E7B45), Align::Right);
        if (ui::button(x + w * 0.5f - 220, y + h - 116, 440, 84, "TO THE PRIZE BOOTH", ui::Style::Gold, true, 36) || ui::key(SDL_SCANCODE_RETURN) ||
            ui::key(SDL_SCANCODE_SPACE)) {
            g.round_over = false;
            enter_shop(g);
        }
    }
    in.blocked = true;
}

} // namespace

void wheel_layout(const Game& g, int wheel, float* cx, float* cy, float* r) {
    int n = std::max(1, static_cast<int>(g.run.deal.size()));
    *r = n <= 4 ? kWheelR : n == 5 ? 130.0f : 114.0f;
    *cx = 70 + 1780.0f * (wheel + 0.5f) / n;
    *cy = kWheelY + (kWheelR - *r) * 0.5f;
}

void enter_round(Game& g) {
    g.screen = Screen::Round;
    g.wheels.assign(g.run.deal.size(), WheelView{});
    for (std::size_t i = 0; i < g.wheels.size(); ++i) g.wheels[i].angle = 0.35f + 1.3f * i;
    g.round_over = g.run.phase == Phase::Shop || g.run.phase == Phase::Won;
    g.payout_open = false;
    g.spoiled_banner = false;
    g.band_armed = false;
    g.quota_hits_round = 0;
    g.flyers.clear();
    sync_shown(g);
    for (auto& wheel : g.run.wheels) see(g, wheel.id);
    Inspection insp = g.run.inspection();
    if (insp != Inspection::None)
        if (auto* d = g.run.t.inspection(insp)) see(g, d->id);
}

void act_spin(Game& g, int wheel) {
    Run& run = g.run;
    if (!g.cues.empty() || !run.can_spin() || wheel < 0 || wheel >= static_cast<int>(run.deal.size())) return;
    bool band = g.band_armed && run.count_consumable("band") > 0 && run.spoil_chance(wheel) > 0;
    WheelView& v = g.wheels[wheel];
    if (band) {
        v.band_clash.clear();
        for (auto& w : run.deal[wheel]) v.band_clash.push_back(run.clashes(wheel, w));
    }
    SpinResult r = run.spin(wheel, band);
    if (!r.ok) return;
    if (r.band) {
        v.band = true;
        g.band_armed = false;
        popup(g, "Outer Band!", 960, 240, rgb(0x7FE0D2), 40, 1.0f);
    }
    audio::risk(0);
    if (!r.free) g.shown_spins = run.spins_left();
    Load after = run.phase == Phase::Trip || run.phase == Phase::Rescue ? run.load : Load();
    animate_spin(g, r, after, 3.0f + (g.frame_no % 2), 1.45f);
}

void act_rescue(Game& g, Rescue how) {
    Run& run = g.run;
    if (run.phase != Phase::Rescue || !run.rescue_allowed(how)) return;
    SpinResult r = run.rescue(how);
    if (!r.ok) return;
    Load after = run.phase == Phase::Trip || run.phase == Phase::Rescue ? run.load : Load();
    if (how == Rescue::Decline) {
        resolve_landing(g, r, after);
        return;
    }
    bool saved = r.landing <= Landing::Blank;
    if (saved) {
        g.save_banner = how == Rescue::Respin ? "THE RESPIN SAVES IT!" : "A NUDGE SAVES IT!";
        g.save_t = 1.6f;
        unlock_check(g, "rescue", 1);
    }
    if (how == Rescue::Respin)
        animate_spin(g, r, after, 2.0f, 1.2f);
    else
        animate_spin(g, r, after, 0.0f, 0.45f);
    if (saved) cue(g, 0, [&g] { sfx(g, audio::Sfx::Save, 0.9f); });
}

void act_deliver(Game& g) {
    Run& run = g.run;
    if (run.phase != Phase::Trip || g.payout_open) return;
    int tank = run.tank;
    Delivery d = run.deliver();
    if (!d.ok) return;
    audio::risk(0);
    if (d.empty) {
        popup(g, "The tank ran dry with nothing aboard", 960, 640, ui::kMuted, 40, 1.4f);
        cue(g, 0.6f, nullptr, nullptr, [&g] { finish_action(g); });
        return;
    }
    g.payout = d;
    g.popups.clear();
    g.payout_open = true;
    g.payout_step = -1;
    int count = static_cast<int>(d.steps.size());
    for (int i = 0; i < count; ++i) {
        const Step& s = d.steps[i];
        float dur = s.kind == Step::Kind::Cargo ? 0.2f : s.kind == Step::Kind::Total ? 1.1f : 0.5f;
        dur *= 1.0f - 0.3f * static_cast<float>(i) / count; // faster as the total grows
        cue(g, dur, [&g, i, count] {
            g.payout_step = i;
            g.payout_pulse = 1;
            const Step& s = g.payout.steps[i];
            float pitch = 0.8f + 0.9f * static_cast<float>(i) / count;
            switch (s.kind) {
            case Step::Kind::Quota: sfx(g, audio::Sfx::QuotaBell, 0.9f); break;
            case Step::Kind::FullTruck: sfx(g, audio::Sfx::FullTruck, 0.8f); break;
            case Step::Kind::Total: sfx(g, audio::Sfx::Total, 1.0f); break;
            case Step::Kind::Cargo: sfx(g, audio::Sfx::LandCargo, 0.5f, pitch); break;
            default: sfx(g, audio::Sfx::Step, 0.8f, pitch); break;
            }
        });
    }
    cue(g, 0.35f, [&g, d, tank] {
        Run& run = g.run;
        g.payout_open = false;
        g.shown_load = Load();
        g.shown_score += d.score.total;
        popup(g, "+" + ui::number(d.score.total), 990, 150, ui::kGold, 54, 1.3f);
        if (d.freight + d.tips > 0) {
            popup(g, "+$" + std::to_string(d.freight + d.tips) + (d.tips ? " freight and tips" : " freight"), kWidth - 220, 160, ui::kGold, 36, 1.5f);
            sfx(g, audio::Sfx::Buy, 0.7f);
        }
        g.shown_money = run.money;
        if (d.score.full) unlock_check(g, "full_truck", 1);
        unlock_check(g, "delivery_worth", static_cast<int>(std::min<long long>(d.score.total, 2000000000)));
        unlock_check(g, "color_set", d.score.flush);
        if (d.load.spins >= tank) unlock_check(g, "empty_tank", 1);
        if (d.score.quota_hit) {
            ++g.quota_hits_round;
            unlock_check(g, "quota_in_round", g.quota_hits_round);
            unlock_check(g, "quota_in_run", run.m.quota_hits);
        }
        g.profile.best_delivery = std::max(g.profile.best_delivery, d.score.total);
    });
    cue(g, 0.2f, nullptr, nullptr, [&g] { finish_action(g); });
}

void act_rod(Game& g, int uid) {
    if (g.run.phase != Phase::Rod) return;
    if (!g.run.rod_remove(uid)) return;
    if (uid != 0) {
        sfx(g, audio::Sfx::Sell, 0.8f);
        unlock_check(g, "zap", 1);
    }
    finish_action(g);
}

void round_screen(Game& g) {
    Run& run = g.run;
    ui::Input& in = ui::input();
    draw_backdrop(g, false);
    draw_top_bar(g, false);

    bool idle = g.cues.empty() && !g.round_over && run.phase == Phase::Trip;
    if (idle && g.flyers.empty()) {
        // What the screen shows can't drift from the rules once nothing is moving.
        g.shown_load = run.load;
        g.shown_score = run.round_score;
        g.shown_money = run.money;
    }
    int cargo = static_cast<int>(g.shown_load.cargo.size());
    audio::music(g.payout_open ? std::max(1, cargo) : cargo, false);

    int sell = draw_items_row(g, 36, 116, false);
    (void)sell;
    draw_consumables(g, kWidth - 36 - 2 * 176 - 12, 116, idle);

    bool any_hover = false;
    for (int i = 0; i < static_cast<int>(run.deal.size()); ++i) {
        draw_wheel(g, i, true);
        float cx, cy, R;
        wheel_layout(g, i, &cx, &cy, &R);
        if (std::hypot(in.mx - cx, in.my - cy) < R + 14) any_hover = true;
        if (idle && ui::key(SDL_SCANCODE_1 + i)) act_spin(g, i);
    }
    if (!any_hover || !idle) audio::risk(0);

    draw_wagon(g);
    draw_gauges(g);
    draw_load_value(g);

    if (g.save_t > 0) {
        float a = std::min(1.0f, g.save_t / 0.3f);
        push_alpha(a);
        ui::sign(72, 960, 250, g.save_banner, ui::kGold);
        pop_alpha();
    }
    if (g.spoiled_banner || g.spoiled_t > 0) {
        float a = std::min(1.0f, g.spoiled_t / 0.3f);
        push_alpha(a);
        push(960, 560, -0.05f, 1);
        ui::sign(150, 0, 0, "SPOILED!", ui::kDanger);
        pop();
        if (g.spoiled_value > 0) ui::sign(54, 960, 650, ui::number(g.spoiled_value) + " lost", ui::kCream);
        pop_alpha();
    }

    // A first run gets one line of direction, in the free band above the wagon.
    if (idle && g.profile.wins == 0 && run.round == 0 && g.toasts.empty()) {
        const char* hint = g.shown_load.cargo.empty() ? "Click a wheel to spin it and load the wagon."
                           : g.shown_load.cargo.size() < 3 ? "Spin again for a bigger load, but a crossed-out wedge would spoil it."
                                                           : "Deliver to bank the load, or press your luck for more.";
        text(Font::Display, 28, 960, 752, hint, ui::kCream, Align::Center);
    }

    draw_payout(g);
    if (g.cues.empty()) {
        if (run.phase == Phase::Rescue) draw_rescue(g);
        else if (run.phase == Phase::Rod) draw_rod(g);
        else if (g.round_over) draw_round_over(g);
    }
    if (in.key[SDL_SCANCODE_ESCAPE] && !g.settings_open) g.paused = !g.paused;
}

} // namespace spin
