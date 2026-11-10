// The prize booth between rounds: five offers on the back wall, the bag of
// wedges in a tray on the counter. docs/design/economy.md.
#include <algorithm>
#include <cmath>

#include <SDL3/SDL.h>

#include "game.h"

namespace spin {

using namespace gfx;

namespace {

const char* kind_label(const Offer& o) {
    switch (o.kind) {
    case OfferKind::Item: return "ITEM";
    case OfferKind::Wheel: return "WHEEL";
    case OfferKind::Upgrade: return "WEDGE UPGRADE";
    case OfferKind::Wedge: return "WEDGE";
    case OfferKind::Consumable: return "CONSUMABLE";
    default: return "ZAP";
    }
}

std::string offer_text(const Game& g, const Offer& o) {
    const Tables& t = g.run.t;
    switch (o.kind) {
    case OfferKind::Item:
        if (auto* d = t.item(o.id)) return d->text;
        break;
    case OfferKind::Wheel:
        if (auto* d = t.wheel(o.id)) return d->text + ". Adds a wheel.";
        break;
    case OfferKind::Upgrade:
        if (auto* d = t.upgrade(o.id)) return d->text;
        break;
    case OfferKind::Consumable:
        if (auto* d = t.consumable(o.id)) return d->text;
        break;
    case OfferKind::Wedge:
        if (o.wedge == WedgeKind::Tarp) return "Goes into your bag. In the truck, covers one repeat.";
        if (o.wedge == WedgeKind::Crate) return "Goes into your bag. In the truck, multiplies the load by 1.5.";
        return "Goes into your bag. Points equal to its value; one more cargo.";
    case OfferKind::Zap: return "Remove a wedge from your bag for good.";
    }
    return "";
}

void offer_icon(const Game& g, const Offer& o, float cx, float cy) {
    switch (o.kind) {
    case OfferKind::Wedge: {
        Wedge w;
        w.kind = o.wedge;
        w.color = o.color;
        w.value = o.value;
        ui::wedge_token(cx, cy, 86, w);
        break;
    }
    case OfferKind::Wheel: {
        const WheelDef* d = g.run.t.wheel(o.id);
        Color rim = ui::kRed;
        if (d) {
            if (d->effect == WheelEffect::Gilded) rim = rgb(0xD9A21E);
            if (d->effect == WheelEffect::Bump) rim = rgb(0x7C4DCC);
            if (d->effect == WheelEffect::Express) rim = rgb(0x2E9E8F);
            if (d->effect == WheelEffect::Padded) rim = rgb(0xD96C9A);
        }
        int n = d ? d->size : 3;
        circle(cx, cy, 52, ui::kInk);
        circle(cx, cy, 47, rim);
        circle(cx, cy, 38, ui::kInk);
        for (int i = 0; i < n; ++i)
            sector(cx, cy, 0, 36, 2 * kPi * i / n + 0.04f, 2 * kPi * (i + 1) / n - 0.04f, i % 2 ? rgb(0xEAD7AC) : ui::kCream);
        circle(cx, cy, 9, ui::kGold);
        break;
    }
    case OfferKind::Item: {
        const ItemDef* d = g.run.t.item(o.id);
        Color accent = ui::rarity_color(o.rarity);
        push(cx, cy, -0.12f, 1);
        ui::ticket(-58, -34, 116, 68, ui::kCream);
        rect(-50, -26, 10, 52, darken(accent, 0.25f));
        std::string glyph = "+";
        if (d) {
            if (!is_additive(d->effect) && d->effect <= ItemEffect::FullBonus) glyph = "\xC3\x97";
            if (d->effect >= ItemEffect::Tip) glyph = d->effect == ItemEffect::Tip || d->effect == ItemEffect::Piggy || d->effect == ItemEffect::Spendthrift ? "$" : "!";
        }
        text(Font::Display, 54, 8, 20, glyph, ui::kInk, Align::Center);
        pop();
        break;
    }
    case OfferKind::Upgrade: {
        // A wedge with a brush across it.
        Wedge w;
        w.kind = WedgeKind::Cargo;
        w.color = o.id == "wild" ? kWild : 2;
        w.value = o.id == "premium" ? 7 : 4;
        w.bonus = o.id == "stamp" ? 3 : 0;
        ui::wedge_token(cx - 8, cy + 4, 74, w);
        line({cx + 10, cy + 30}, {cx + 56, cy - 34}, 12, ui::kInk);
        line({cx + 12, cy + 27}, {cx + 54, cy - 31}, 6, ui::kWood);
        circle(cx + 10, cy + 30, 10, ui::kInk);
        circle(cx + 10, cy + 30, 6, ui::kGold);
        break;
    }
    case OfferKind::Consumable:
        panel(cx - 56, cy - 28, 112, 56, 28, rgb(0x2F8F86), ui::kInk, 4);
        text(Font::Display, 30, cx, cy + 11, o.id == "band" ? "BAND" : o.id == "nudge" ? "< >" : "AGAIN", ui::kCream, Align::Center);
        break;
    case OfferKind::Zap: {
        Vec2 p[6] = {{cx + 8, cy - 50}, {cx - 26, cy + 6}, {cx - 4, cy + 6}, {cx - 12, cy + 50}, {cx + 28, cy - 10}, {cx + 4, cy - 10}};
        for (int pass = 0; pass < 2; ++pass) {
            Color c = pass == 0 ? ui::kInk : ui::kGold;
            float s = pass == 0 ? 1.18f : 1.0f;
            auto sc = [&](Vec2 v) { return Vec2{cx + (v.x - cx) * s, cy + (v.y - cy) * s}; };
            tri(sc(p[0]), sc(p[1]), sc(p[5]), c);
            tri(sc(p[1]), sc(p[2]), sc(p[5]), c);
            tri(sc(p[2]), sc(p[3]), sc(p[4]), c);
            tri(sc(p[2]), sc(p[4]), sc(p[5]), c);
        }
        break;
    }
    }
}

bool needs_wedge(const Offer& o) { return o.kind == OfferKind::Upgrade || o.kind == OfferKind::Zap; }

void draw_offer(Game& g, int i, float x, float y, float w, float h) {
    Run& run = g.run;
    Offer& o = run.offers[i];
    ui::Input& in = ui::input();
    bool over = !in.blocked && ui::hover(x, y, w, h);
    bool targeting = g.target_offer == i;
    Color accent = ui::rarity_color(o.rarity);
    // Hung on the back wall by a string.
    line({x + w * 0.5f, y - 46}, {x + w * 0.5f, y + 4}, 4, ui::kCreamDark);
    circle(x + w * 0.5f, y - 46, 7, ui::kGold);
    if (o.sold) {
        rrect(x, y, w, h, 14, rgb(0x000000, 0.25f));
        push(x + w * 0.5f, y + h * 0.5f, -0.2f, 1);
        ui::sign(54, 0, 16, "SOLD", ui::kMuted);
        pop();
        return;
    }
    float lift = over || targeting ? -6.0f : 0.0f;
    y += lift;
    if (targeting) rrect(x - 12, y - 12, w + 24, h + 24, 22, ui::kGold.with_alpha(0.7f));
    panel(x, y, w, h, 14, ui::kCream, ui::kInk, 5);
    rrect(x, y, w, 44, 14, darken(accent, 0.3f));
    rect(x, y + 30, w, 14, darken(accent, 0.3f));
    text(Font::Display, 20, x + 16, y + 30, kind_label(o), ui::kCream);
    text(Font::Text, 18, x + w - 16, y + 29, ui::rarity_name(o.rarity), ui::kCream, Align::Right);
    offer_icon(g, o, x + w * 0.5f, y + 108);
    float ns = 28;
    float nw = text_width(Font::Display, ns, o.name);
    if (nw > w - 28) ns *= (w - 28) / nw;
    text(Font::Display, ns, x + w * 0.5f, y + 196, o.name, ui::kInk, Align::Center);
    text_wrapped(Font::Text, 20, x + w * 0.5f, y + 228, w - 36, offer_text(g, o), rgb(0x4A3E50), Align::Center, 1.2f);

    bool afford = o.price <= run.money, room = run.can_take(o);
    std::string label = "$" + std::to_string(o.price);
    if (!room) label = o.kind == OfferKind::Item ? "No slot" : o.kind == OfferKind::Wheel ? "6 wheels" : o.kind == OfferKind::Consumable ? "Full" : label;
    if (targeting) label = "Cancel";
    bool clicked = ui::button(x + 30, y + h - 78, w - 60, 60, label, targeting ? ui::Style::Dark : afford && room ? ui::Style::Gold : ui::Style::Dark,
                              targeting || (afford && room), 32);
    if (over && !clicked) {
        std::string body = offer_text(g, o);
        if (!afford) body += "\nYou need $" + std::to_string(o.price - run.money) + " more.";
        if (!room && o.kind == OfferKind::Item) body += "\nYour item slots are full: sell one first (right-click it).";
        ui::tooltip(o.name, body, accent);
    }
    if (clicked) {
        if (targeting) {
            g.target_offer = -1;
            g.target_uid = 0;
        } else if (needs_wedge(o)) {
            g.target_offer = i;
            g.target_uid = 0;
        } else {
            shop_buy(g, i);
        }
    }
}

std::vector<int> sorted_bag(const Run& run) {
    std::vector<int> order(run.bag.size());
    for (std::size_t i = 0; i < order.size(); ++i) order[i] = static_cast<int>(i);
    std::stable_sort(order.begin(), order.end(), [&](int a, int b) {
        const Wedge &x = run.bag[a], &y = run.bag[b];
        if (x.kind != y.kind) return static_cast<int>(x.kind) < static_cast<int>(y.kind);
        if (x.value != y.value) return x.value < y.value;
        return x.color < y.color;
    });
    return order;
}

void draw_bag(Game& g, float x, float y, float w) {
    Run& run = g.run;
    ui::Input& in = ui::input();
    const Offer* target = g.target_offer >= 0 ? &run.offers[g.target_offer] : nullptr;
    int n = static_cast<int>(run.bag.size());
    int slots = 0;
    for (std::size_t i = 0; i < run.wheels.size(); ++i) slots += run.wheel_def(static_cast<int>(i)).size;
    // The tray.
    panel(x, y, w, 178, 16, ui::kWoodDark, ui::kInk, 5);
    rrect(x + 10, y + 10, w - 20, 158, 10, rgb(0x4A2E16));
    std::string title = "YOUR BAG: " + std::to_string(n) + " wedges for " + std::to_string(slots) + " wheel slots";
    if (n > slots) title += " (" + std::to_string(n - slots) + " sit each round out)";
    if (n < slots) title += " (" + std::to_string(slots - n) + " slots stay empty)";
    text(Font::Display, 22, x + 24, y + 40, title, ui::kCream);
    if (target) text(Font::Display, 22, x + w - 24, y + 40, std::string("Choose a wedge for ") + target->name, ui::kGold, Align::Right);
    auto order = sorted_bag(run);
    int per_row = std::max(1, (n + 1) / 2);
    if (n <= 20) per_row = n;
    float size = std::min(58.0f, (w - 60) / std::max(per_row, 1) - 8);
    if (n > 20) size = std::min(size, 50.0f);
    float step = std::min(size + 12, (w - 60) / std::max(per_row, 1));
    for (int k = 0; k < n; ++k) {
        const Wedge& wedge = run.bag[order[k]];
        int row = k / per_row, col = k % per_row;
        float tx = x + 30 + step * 0.5f + col * step, ty = y + (n > 20 ? 78 : 106) + row * (size + 10);
        bool fits = target && run.upgrade_fits(*target, wedge);
        bool over = !in.blocked && ui::hover(tx - step * 0.5f, ty - size * 0.5f - 4, step, size + 8);
        if (target && fits) circle(tx, ty, size * 0.72f, ui::kGold.with_alpha(over ? 0.8f : 0.3f));
        if (g.target_uid == wedge.uid) circle(tx, ty, size * 0.8f, ui::kSafe.with_alpha(0.8f));
        ui::wedge_token(tx, ty, size, wedge, -1, target && !fits);
        if (over) {
            std::string body = wedge.kind == WedgeKind::Cargo ? "Cargo: points equal to its value." : wedge.kind == WedgeKind::Tarp ? "Covers one repeat." : "x1.5 to the load.";
            if (target) body = fits ? std::string("Click to use ") + target->name + " on it." : std::string(target->name) + " can't be used on this wedge.";
            ui::tooltip(ui::wedge_name(wedge), body);
            if (target && fits && in.pressed) {
                if (target->id == "recut" || target->id == "paint") {
                    g.target_uid = wedge.uid;
                    audio::play(audio::Sfx::Click, 0.6f);
                } else {
                    shop_buy(g, g.target_offer, wedge.uid, 0);
                }
            }
        }
    }
}

// Recut's value or Paint's color, once a wedge is picked.
void draw_choice(Game& g) {
    Run& run = g.run;
    if (g.target_offer < 0 || g.target_uid == 0) return;
    const Offer& o = run.offers[g.target_offer];
    Wedge* wedge = run.find_wedge(g.target_uid);
    if (!wedge) {
        g.target_uid = 0;
        return;
    }
    bool recut = o.id == "recut";
    int n = recut ? 6 : 4;
    float w = 120.0f * n + 80, h = 250, x = (kWidth - w) * 0.5f, y = 330;
    rect(0, 105, kWidth, kHeight - 105, rgb(0x050817, 0.6f));
    panel(x, y, w, h, 18, ui::kCream, ui::kInk, 6);
    text(Font::Display, 34, x + w * 0.5f, y + 58, recut ? "Recut it to which value?" : "Paint it which color?", ui::kInk, Align::Center);
    ui::Input& in = ui::input();
    for (int i = 0; i < n; ++i) {
        float tx = x + 100 + i * 120.0f, ty = y + 140;
        bool over = ui::hover(tx - 54, ty - 54, 108, 108);
        if (over) circle(tx, ty, 62, ui::kGold.with_alpha(0.7f));
        Wedge preview = *wedge;
        if (recut) preview.value = i + 1;
        else preview.color = i;
        ui::wedge_token(tx, ty, 84, preview);
        if (over && in.pressed) {
            shop_buy(g, g.target_offer, wedge->uid, recut ? i + 1 : i);
            return;
        }
    }
    text(Font::Text, 20, x + w * 0.5f, y + h - 18, "Esc or right-click to cancel", rgb(0x6B5F70), Align::Center);
}

} // namespace

bool shop_buy(Game& g, int offer, int uid, int choice) {
    Run& run = g.run;
    if (offer < 0 || offer >= static_cast<int>(run.offers.size())) return false;
    OfferKind kind = run.offers[offer].kind;
    if (!run.buy(offer, uid, choice)) {
        audio::play(audio::Sfx::Refuse, 0.5f);
        return false;
    }
    audio::play(kind == OfferKind::Zap ? audio::Sfx::Sell : audio::Sfx::Buy, 0.9f);
    if (kind == OfferKind::Zap) unlock_check(g, "zap", 1);
    g.target_offer = -1;
    g.target_uid = 0;
    g.shown_money = run.money;
    save_run(g);
    return true;
}

void shop_leave(Game& g) {
    if (g.run.phase != Phase::Shop) return;
    g.target_offer = -1;
    g.target_uid = 0;
    g.run.leave_shop();
    enter_round(g);
    save_run(g);
    audio::play(audio::Sfx::Reroll, 0.6f, 0.8f);
}

void enter_shop(Game& g) {
    g.screen = Screen::Shop;
    g.target_offer = -1;
    g.target_uid = 0;
    g.shown_money = g.run.money;
    g.shown_score = g.run.round_score;
    for (auto& o : g.run.offers) see(g, o.id);
    unlock_check(g, "hold_money", g.run.money);
    save_run(g);
    save_profile(g);
}

void shop_screen(Game& g) {
    Run& run = g.run;
    ui::Input& in = ui::input();
    draw_backdrop(g, true);
    draw_top_bar(g, true);
    audio::music(0, true);
    for (auto& o : run.offers) see(g, o.id);

    ui::sign(40, 36, 162, "THE PRIZE BOOTH", ui::kGold, Align::Left);
    text(Font::Text, 22, 470, 158, "Shop for the round ahead. Nothing here is a must: buy what tilts your game.", ui::kMuted);

    // Offers on the back wall.
    int n = static_cast<int>(run.offers.size());
    float cw = 300, ch = 392, gap = 16, total = n * cw + (n - 1) * gap;
    float ox = 36;
    for (int i = 0; i < n; ++i) draw_offer(g, i, ox + i * (cw + gap), 222, cw, ch);

    // Reroll, beside the offers.
    float rx = ox + total + 20, rw = kWidth - 36 - rx;
    panel(rx, 222, rw, ch, 14, ui::kNight2, ui::kInk, 5);
    text(Font::Display, 26, rx + rw * 0.5f, 268, "NEW OFFERS", ui::kCream, Align::Center);
    text_wrapped(Font::Text, 20, rx + rw * 0.5f, 300, rw - 30, "Hang five new offers. $1 more each time this visit.", ui::kMuted,
                 Align::Center);
    if (ui::button(rx + 18, 222 + ch - 180, rw - 36, 70, "Reroll $" + std::to_string(run.reroll_price), ui::Style::Red, run.money >= run.reroll_price, 30)) {
        if (run.reroll()) {
            audio::play(audio::Sfx::Reroll, 0.9f);
            g.target_offer = -1;
            g.target_uid = 0;
            g.shown_money = run.money;
            for (auto& o : run.offers) see(g, o.id);
            save_run(g);
        }
    }
    if (ui::button(rx + 18, 222 + ch - 92, rw - 36, 74, "HIT THE ROAD", ui::Style::Gold, true, 30) || ui::key(SDL_SCANCODE_RETURN)) {
        shop_leave(g);
        return;
    }

    draw_bag(g, 36, 640, kWidth - 72);

    // Items, wheels and consumables along the bottom.
    text(Font::Display, 22, 36, 866, "ITEMS, in the order they apply", ui::kCream);
    int sell = draw_items_row(g, 36, 884, true);
    if (sell >= 0 && run.sell_item(sell)) {
        audio::play(audio::Sfx::Sell, 0.9f);
        g.shown_money = run.money;
        save_run(g);
    }
    text(Font::Display, 22, 1430, 866, "CONSUMABLES", ui::kCream);
    draw_consumables(g, 1430, 884, false);

    // Wheels owned, and the round ahead.
    float wy = 1000;
    text(Font::Display, 22, 36, wy + 30, "WHEELS", ui::kCream);
    for (int i = 0; i < static_cast<int>(run.wheels.size()); ++i) {
        const WheelDef* d = run.t.wheel(run.wheels[i].id);
        if (!d) continue;
        float bx = 150 + i * 196.0f;
        bool over = !in.blocked && ui::hover(bx, wy, 184, 44);
        panel(bx, wy, 184, 44, 22, over ? ui::kNight3 : ui::kNight2, ui::kInk, 3);
        text(Font::Text, 20, bx + 92, wy + 29, d->name, d->effect == WheelEffect::None && d->size == 3 ? ui::kMuted : ui::kGold, Align::Center);
        if (over) ui::tooltip(d->name, d->text + ". Holds " + std::to_string(d->size) + " wedges.");
    }
    int next = run.round + 1;
    Inspection insp = run.inspection_of(next);
    std::string ahead = "Next: round " + std::to_string(next + 1) + ", target " + ui::number(run.target_of(next)) + ", quota " + std::to_string(run.quota_of(next));
    if (insp != Inspection::None)
        if (auto* d = run.t.inspection(insp)) ahead += ". INSPECTION: " + d->name;
    text(Font::Display, 22, kWidth - 36, wy + 30, ahead, insp != Inspection::None ? ui::kGold : ui::kCream, Align::Right);

    if (g.target_offer >= 0 && (in.key[SDL_SCANCODE_ESCAPE] || in.right_pressed)) {
        g.target_offer = -1;
        g.target_uid = 0;
    } else if (in.key[SDL_SCANCODE_ESCAPE] && !g.settings_open) {
        g.paused = !g.paused;
    }
    if (g.target_uid != 0) {
        in.blocked = false;
        draw_choice(g);
    }
}

} // namespace spin
