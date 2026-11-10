// The start screen, the end of a run, the collection, and the pause and
// settings panels. docs/design/progression.md.
#include <algorithm>
#include <cmath>
#include <filesystem>

#include <SDL3/SDL.h>

#include "data/tables_json.h"
#include "game.h"

namespace spin {

using namespace gfx;

namespace {

std::string content_name(const Tables& t, const std::string& id) {
    if (auto* d = t.item(id)) return d->name;
    if (auto* d = t.wheel(id)) return d->name;
    if (auto* d = t.upgrade(id)) return d->name;
    if (auto* d = t.consumable(id)) return d->name;
    return id;
}

std::string stake_text(const Tables& t, int stake) {
    if (stake <= 1) return "Stake 1: the base game.";
    std::string s = "Stake " + std::to_string(stake) + " adds: " + t.stakes[stake - 2].name + ".";
    if (stake > 2) s += " It keeps every rule of the stakes below it.";
    return s;
}

bool has_saved_run(const Game& g) {
    std::error_code ec;
    return !g.save_dir.empty() && std::filesystem::exists(fs_path(g.save_dir + "run.json"), ec);
}

std::uint64_t random_seed() {
    std::uint64_t x = SDL_GetPerformanceCounter() ^ (static_cast<std::uint64_t>(SDL_GetTicksNS()) << 7);
    x ^= x >> 33;
    x *= 0xff51afd7ed558ccdULL;
    x ^= x >> 33;
    return 100000 + x % 900000000ULL;
}

void slider_row(float x, float y, float w, const char* label, float& value) {
    text(Font::Display, 26, x, y + 34, label, ui::kInk);
    float bx = x + 230, bw = w - 230 - 150;
    if (ui::button(bx - 70, y, 56, 48, "-", ui::Style::Dark, value > 0.001f, 30)) value = std::max(0.0f, value - 0.1f);
    panel(bx, y + 8, bw, 32, 10, ui::kNight, ui::kInk, 3);
    if (value > 0.01f) rrect(bx, y + 8, bw * value, 32, 10, ui::kGold);
    if (ui::button(bx + bw + 14, y, 56, 48, "+", ui::Style::Dark, value < 0.999f, 30)) value = std::min(1.0f, value + 0.1f);
    text(Font::Text, 22, x + w, y + 32, std::to_string(static_cast<int>(value * 100 + 0.5f)) + "%", ui::kInk, Align::Right);
}

} // namespace

void draw_backdrop(Game& g, bool booth) {
    (void)g;
    gradient_rect(0, 0, kWidth, kHeight, rgb(0x0B1026), rgb(0x1B1F4A));
    // Stars, and a string of lights across the midway.
    for (int i = 0; i < 70; ++i) {
        float x = std::fmod(i * 397.31f, kWidth), y = 110 + std::fmod(i * 211.7f, 420.0f);
        float tw = 0.35f + 0.3f * std::sin(static_cast<float>(ui::time()) * (0.6f + i % 5 * 0.2f) + i);
        circle(x, y, 1.6f + (i % 3) * 0.7f, rgb(0xFFF4D6, tw));
    }
    if (booth) {
        // The booth's back wall and counter.
        panel(18, 196, kWidth - 36, 436, 18, rgb(0x5A1F1B), ui::kInk, 5);
        for (int i = 0; i < 16; ++i) rect(18 + i * 118.0f + 59, 196, 4, 436, rgb(0x000000, 0.14f));
        panel(8, 626, kWidth - 16, 206, 14, ui::kWood, ui::kInk, 5);
        rect(8, 626, kWidth - 16, 10, lighten(ui::kWood, 0.2f));
    } else {
        // Distant tents behind the wheels.
        for (int i = 0; i < 5; ++i) {
            float x = 120 + i * 420.0f, base = 700;
            tri({x - 190, base}, {x + 190, base}, {x, base - 170 - (i % 2) * 40.0f}, rgb(0x141A40));
            line({x, base - 170 - (i % 2) * 40.0f}, {x, base - 210 - (i % 2) * 40.0f}, 4, rgb(0x141A40));
        }
        rect(0, 690, kWidth, 390, rgb(0x10143A));
        int chase = static_cast<int>(ui::time() * 2);
        for (int i = 0; i < 34; ++i) {
            float t = i / 33.0f;
            float y = 232 + std::sin(t * kPi * 3) * 14;
            ui::bulb(20 + t * (kWidth - 40), y, 4.5f, (i + chase) % 2 == 0, i % 3 == 0 ? rgb(0xFF7A6E) : i % 3 == 1 ? ui::kGold : rgb(0x7FD0FF));
        }
    }
}

void title_screen(Game& g) {
    ui::Input& in = ui::input();
    const Tables& t = g.tables;
    Profile& p = g.profile;
    draw_backdrop(g, false);
    audio::music(2, true);
    audio::risk(0);

    // The marquee.
    ui::stripes(0, 0, kWidth, 26, 60);
    rect(0, 26, kWidth, 5, ui::kInk);
    float mx = 310, mw = 1300, my = 70, mh = 200;
    panel(mx, my, mw, mh, 30, ui::kRed, ui::kInk, 7);
    rrect(mx + 16, my + 16, mw - 32, mh - 32, 20, ui::kRedDark);
    int chase = static_cast<int>(ui::time() * 7);
    ui::bulb_row(mx + 30, my + 8, mx + mw - 30, my + 8, 34, 0, 8, chase);
    ui::bulb_row(mx + 30, my + mh - 8, mx + mw - 30, my + mh - 8, 34, 0, 8, chase + 1);
    ui::sign(138, kWidth * 0.5f, my + 150, "SPINTACULAR", ui::kGold);
    text(Font::Display, 28, kWidth * 0.5f, my + mh + 48, "Load the wagon one spin at a time. One bad spin spoils the lot.", ui::kCream, Align::Center);

    // ---- rigs
    float lx = 80, ly = 380;
    text(Font::Display, 28, lx, ly - 14, "RIG", ui::kGold);
    int rigs = static_cast<int>(t.rigs.size());
    g.rig_index = std::clamp(g.rig_index, 0, rigs - 1);
    for (int i = 0; i < rigs; ++i) {
        const Mods& rig = t.rigs[i];
        float x = lx + (i % 3) * 330.0f, y = ly + (i / 3) * 128.0f, w = 314, h = 112;
        bool open = p.rig_unlocked(t, rig.id);
        bool sel = g.rig_index == i;
        bool over = !in.blocked && ui::hover(x, y, w, h);
        panel(x, y, w, h, 14, sel ? ui::kCream : open ? ui::kNight3 : rgb(0x1A1E3A), sel ? ui::kGold : ui::kInk, sel ? 6 : 4);
        Color ink = sel ? ui::kInk : open ? ui::kCream : rgb(0x6C7194);
        text(Font::Display, 30, x + 18, y + 42, open ? rig.name : "Locked", ink);
        auto best = p.best_stake.find(rig.id);
        if (open && best != p.best_stake.end())
            text(Font::Text, 18, x + w - 16, y + 38, "won stake " + std::to_string(best->second), sel ? rgb(0x1E7B45) : ui::kSafe, Align::Right);
        text_wrapped(Font::Text, 18, x + 18, y + 70, w - 34, open ? rig.text : "Win a run to open the next rig.", sel ? rgb(0x4A3E50) : rgb(0x9AA3C7),
                     Align::Left, 1.15f);
        if (over) {
            ui::tooltip(open ? rig.name + " rig" : "Locked rig", open ? rig.text : "Each run you win opens the next rig. " + rig.name + " needs " +
                                                                                        std::to_string(i) + (i == 1 ? " win." : " wins."));
            if (in.pressed && open) {
                g.rig_index = i;
                g.stake = std::min(g.stake, p.max_stake(t, rig.id));
                audio::play(audio::Sfx::Click, 0.6f);
            }
        }
    }

    // ---- stake
    float sy = ly + 290;
    const std::string& rig_id = t.rigs[g.rig_index].id;
    int max_stake = p.max_stake(t, rig_id), stakes = static_cast<int>(t.stakes.size()) + 1;
    g.stake = std::clamp(g.stake, 1, max_stake);
    text(Font::Display, 28, lx, sy - 14, "STAKE", ui::kGold);
    for (int s = 1; s <= stakes; ++s) {
        float x = lx + (s - 1) * 82.0f;
        bool open = s <= max_stake, sel = s == g.stake;
        bool over = !in.blocked && ui::hover(x, sy, 70, 70);
        panel(x, sy, 70, 70, 35, sel ? ui::kGold : open ? ui::kNight3 : rgb(0x1A1E3A), ui::kInk, 4);
        text(Font::Display, 34, x + 35, sy + 47, std::to_string(s), sel ? ui::kInk : open ? ui::kCream : rgb(0x565B7E), Align::Center);
        if (over) {
            ui::tooltip("Stake " + std::to_string(s), stake_text(t, s) + (open ? "" : "\nWin stake " + std::to_string(s - 1) + " with this rig to open it."));
            if (in.pressed && open) {
                g.stake = s;
                audio::play(audio::Sfx::Click, 0.6f);
            }
        }
    }
    text(Font::Text, 22, lx + stakes * 82.0f + 14, sy + 44, stake_text(t, g.stake).substr(0, 60), ui::kMuted);

    // ---- seed
    float ey = sy + 130;
    text(Font::Display, 28, lx, ey - 14, "SEED", ui::kGold);
    bool over_seed = ui::hover(lx, ey, 420, 64);
    if (in.pressed && !in.blocked) g.seed_focus = over_seed;
    panel(lx, ey, 420, 64, 12, ui::kNight, g.seed_focus ? ui::kGold : ui::kInk, 4);
    if (g.seed_focus) {
        for (char c : in.text)
            if (c >= '0' && c <= '9' && g.seed_text.size() < 18) g.seed_text += c;
        if (in.key[SDL_SCANCODE_BACKSPACE] && !g.seed_text.empty()) g.seed_text.pop_back();
    }
    bool caret = g.seed_focus && std::fmod(ui::time(), 1.0) < 0.5;
    text(Font::Display, 30, lx + 20, ey + 43, g.seed_text.empty() && !g.seed_focus ? "random" : g.seed_text + (caret ? "|" : ""),
         g.seed_text.empty() ? ui::kMuted : ui::kCream);
    text(Font::Text, 20, lx + 440, ey + 40, "Type a seed to replay a run, or leave it for a new one.", ui::kMuted);
    if (over_seed) ui::tooltip("Seed", "The same seed, rig and stake always deal the same wheels, spins and shops.");

    // ---- the buttons
    float bx = 1180, bw = 660, by = 366;
    bool saved = has_saved_run(g);
    if (saved) {
        if (ui::button(bx, by, bw, 92, "CONTINUE THE RUN", ui::Style::Green, true, 40)) {
            if (!resume_run(g)) toast(g, "That run couldn't be loaded", "It was saved by a different version.");
        }
        by += 112;
    }
    if (ui::button(bx, by, bw, 92, "START A RUN", ui::Style::Gold, true, 44) || (!saved && !g.seed_focus && ui::key(SDL_SCANCODE_RETURN))) {
        std::uint64_t seed = g.seed_text.empty() ? random_seed() : std::strtoull(g.seed_text.c_str(), nullptr, 10);
        start_run(g, seed, false);
        return;
    }
    by += 112;
    bool daily_done = p.daily_date == daily_date() && p.daily_result != "In progress";
    if (ui::button(bx, by, bw, 78, daily_done ? "DAILY HAUL: PLAYED" : "PLAY THE DAILY HAUL", ui::Style::Red, true, 34)) {
        start_run(g, daily_seed(), true);
        return;
    }
    if (ui::hover(bx, by, bw, 78))
        ui::tooltip("The daily haul", "One seed a day, the same for everyone: the Standard rig on stake 1." +
                                          (p.daily_date == daily_date() ? "\nToday: " + p.daily_result + "." : std::string()));
    by += 98;
    float tw3 = (bw - 28) / 3;
    if (ui::button(bx, by, tw3, 72, "HOW TO PLAY", ui::Style::Cream, true, 24)) g.help_open = true;
    if (ui::button(bx + tw3 + 14, by, tw3, 72, "COLLECTION", ui::Style::Cream, true, 24)) {
        g.screen = Screen::Collection;
        g.collection_scroll = 0;
    }
    if (ui::button(bx + 2 * (tw3 + 14), by, tw3, 72, "SETTINGS", ui::Style::Cream, true, 24)) g.settings_open = true;
    by += 92;
    if (ui::button(bx, by, bw, 64, "QUIT", ui::Style::Dark, true, 28)) g.quit = true;

    // ---- the record
    std::string record = std::to_string(p.runs) + " runs, " + std::to_string(p.wins) + " won";
    if (p.best_round > 0) record += ". Furthest: round " + std::to_string(p.best_round);
    if (p.best_delivery > 0) record += ". Best delivery: " + ui::number(p.best_delivery);
    int locked = static_cast<int>(p.locked_now(t).size());
    record += ". " + std::to_string(static_cast<int>(t.locked.size()) - locked) + " of " + std::to_string(t.locked.size()) + " unlocks found.";
    text(Font::Text, 22, kWidth * 0.5f, kHeight - 30, record, ui::kMuted, Align::Center);
    text(Font::Text, 18, kWidth - 30, kHeight - 30, "F1: tuning panel   F11: full screen", rgb(0x5C6390), Align::Right);
}

void run_end_screen(Game& g) {
    Run& run = g.run;
    draw_backdrop(g, false);
    audio::music(g.run_won ? 6 : 0, !g.run_won);
    audio::risk(0);
    ui::stripes(0, 0, kWidth, 26, 60);
    rect(0, 26, kWidth, 5, ui::kInk);
    bool won = g.run_won;
    ui::sign(110, kWidth * 0.5f, 190, won ? "WHAT A SHOW!" : "THE SHOW'S OVER", won ? ui::kGold : ui::kCream);
    std::string sub = won ? (run.endless ? "The endless road ended at round " + std::to_string(run.round + 1) + "."
                                         : "All " + std::to_string(run.t.run.rounds) + " rounds cleared.")
                          : "Round " + std::to_string(run.round + 1) + " came up short: " + ui::number(run.round_score) + " of " + ui::number(run.target()) + ".";
    if (run.endless && !won) sub = "The endless road ended at round " + std::to_string(run.round + 1) + ".";
    text(Font::Display, 32, kWidth * 0.5f, 250, sub, ui::kCream, Align::Center);
    if (won) ui::bulb_row(300, 290, kWidth - 300, 290, 40, 0, 8, static_cast<int>(ui::time() * 8));

    float x = 360, y = 330, w = 1200, h = 440;
    ui::ticket(x, y, w, h);
    ui::stripes(x + 8, y + 8, w - 16, 14, 40);
    const Mods* rig = g.tables.rig(run.config.rig);
    const Metrics& m = run.m;
    struct Row {
        std::string label, value;
    } rows[] = {
        {"Rig and stake", (rig ? rig->name : run.config.rig) + ", stake " + std::to_string(run.config.stake)},
        {run.config.daily ? "Daily haul" : "Seed", std::to_string(run.config.seed)},
        {"Deliveries", std::to_string(m.deliveries) + " (" + std::to_string(m.delivered_cargo) + " cargo)"},
        {"Best delivery", ui::number(m.best_delivery)},
        {"Loads spoiled", std::to_string(m.spoiled) + " (" + std::to_string(m.spoiled_cargo) + " cargo lost)"},
        {"Deliveries that reached the quota", std::to_string(m.quota_hits)},
        {"Full trucks", std::to_string(m.full_trucks)},
        {"Loads saved", std::to_string(m.saves)},
    };
    float ry = y + 76;
    int i = 0;
    for (auto& row : rows) {
        float cx = x + 60 + (i % 2) * (w * 0.5f - 20), cy = ry + (i / 2) * 58.0f;
        text(Font::Text, 22, cx, cy, row.label, rgb(0x6B5F70));
        text(Font::Display, 28, cx, cy + 32, row.value, ui::kInk);
        ++i;
    }
    float uy = y + 330;
    rect(x + 50, uy - 26, w - 100, 3, ui::kInk);
    if (g.newly_unlocked.empty()) {
        text(Font::Text, 24, x + 60, uy + 14, "Nothing new unlocked this run. The collection shows what each locked piece asks for.", rgb(0x6B5F70));
    } else {
        std::string names;
        for (auto& id : g.newly_unlocked) names += (names.empty() ? "" : ", ") + content_name(g.tables, id);
        text(Font::Display, 26, x + 60, uy + 14, "Unlocked this run:", rgb(0x1E7B45));
        text_wrapped(Font::Text, 24, x + 60, uy + 50, w - 120, names, ui::kInk);
    }

    float by = 810;
    if (won && !run.endless && run.phase == Phase::Won) {
        if (ui::button(480, by, 460, 88, "KEEP GOING: ENDLESS", ui::Style::Green, true, 34)) {
            if (run.go_endless()) {
                enter_shop(g);
                return;
            }
        }
        if (ui::button(980, by, 460, 88, "NEW RUN", ui::Style::Gold, true, 38)) {
            g.has_run = false;
            save_run(g);
            g.screen = Screen::Title;
        }
    } else {
        if (ui::button(730, by, 460, 88, "BACK TO THE MIDWAY", ui::Style::Gold, true, 34) || ui::key(SDL_SCANCODE_RETURN)) {
            g.has_run = false;
            save_run(g);
            g.screen = Screen::Title;
        }
    }
    // A result to share: the daily haul's is the same seed for everyone.
    if (ui::button(kWidth * 0.5f - 170, 920, 340, 60, run.config.daily ? "COPY DAILY RESULT" : "COPY RESULT", ui::Style::Dark, true, 24)) {
        std::string result = std::string("Spintacular ") + (run.config.daily ? "daily haul " : "seed ") + std::to_string(run.config.seed) + ": " +
                             (won ? "won" : "reached round " + std::to_string(run.round + 1)) + " on the " + (rig ? rig->name : run.config.rig) +
                             " rig, stake " + std::to_string(run.config.stake) + ". Best delivery " + ui::number(m.best_delivery) + ", " +
                             std::to_string(m.spoiled) + " loads spoiled.";
        SDL_SetClipboardText(result.c_str());
        toast(g, "Copied", "Paste it anywhere to share.");
    }
    if (!won) text(Font::Text, 22, kWidth * 0.5f, 1020, "Type the seed on the start screen to try this run again.", ui::kMuted, Align::Center);
}

void collection_screen(Game& g) {
    ui::Input& in = ui::input();
    const Tables& t = g.tables;
    Profile& p = g.profile;
    draw_backdrop(g, false);
    rect(0, 0, kWidth, kHeight, rgb(0x0B1026, 0.6f));

    struct Entry {
        std::string id, name, text, kind;
        Rarity rarity;
        bool content;
    };
    std::vector<std::pair<std::string, std::vector<Entry>>> groups;
    {
        std::vector<Entry> e;
        for (auto& d : t.items) e.push_back({d.id, d.name, d.text, "Item", d.rarity, true});
        groups.push_back({"ITEMS", e});
        e.clear();
        for (auto& d : t.wheels) e.push_back({d.id, d.name, d.text, "Wheel", d.rarity, true});
        groups.push_back({"WHEELS", e});
        e.clear();
        for (auto& d : t.upgrades) e.push_back({d.id, d.name, d.text, "Upgrade", d.rarity, true});
        for (auto& d : t.consumables) e.push_back({d.id, d.name, d.text, "Consumable", d.rarity, true});
        e.push_back({"tarp_wedge", "Tarp wedge", "Covers one repeat", "Wedge", Rarity::Common, true});
        e.push_back({"crate_wedge", "Crate wedge", "x1.5 to the load", "Wedge", Rarity::Common, true});
        e.push_back({"cargo_wedge", "Cargo wedge", "Points equal to its value; one more cargo", "Wedge", Rarity::Common, true});
        e.push_back({"zap", "Zap", "Remove a wedge from the bag", "Zap", Rarity::Common, true});
        groups.push_back({"UPGRADES, CONSUMABLES AND WEDGES", e});
        e.clear();
        for (auto& d : t.inspections) e.push_back({d.id, d.name, d.text, "Inspection", Rarity::Common, true});
        groups.push_back({"INSPECTIONS", e});
    }

    float top = 150, y = top - g.collection_scroll;
    const float cw = 440, ch = 104, gap = 16, x0 = 60;
    const int cols = 4;
    int seen_count = 0, total = 0;
    for (auto& [title, entries] : groups) {
        if (y > 110 && y < kHeight) text(Font::Display, 28, x0, y + 8, title, ui::kGold);
        y += 28;
        for (std::size_t i = 0; i < entries.size(); ++i) {
            const Entry& e = entries[i];
            float x = x0 + (i % cols) * (cw + gap), cy = y + (i / cols) * (ch + gap);
            bool locked = std::find(t.locked.begin(), t.locked.end(), e.id) != t.locked.end() && !p.is_unlocked(e.id);
            bool seen = p.has_seen(e.id);
            ++total;
            seen_count += seen;
            if (cy + ch < 110 || cy > kHeight) continue;
            Color accent = ui::rarity_color(e.rarity);
            panel(x, cy, cw, ch, 12, seen ? ui::kCream : locked ? rgb(0x1A1E3A) : ui::kNight3, ui::kInk, 4);
            if (seen) {
                rect(x + 8, cy + 8, 10, ch - 16, darken(accent, 0.25f));
                text(Font::Display, 26, x + 30, cy + 38, e.name, ui::kInk);
                text(Font::Text, 16, x + cw - 14, cy + 30, e.kind, rgb(0x6B5F70), Align::Right);
                text_wrapped(Font::Text, 18, x + 30, cy + 64, cw - 46, e.text, rgb(0x4A3E50), Align::Left, 1.15f);
            } else if (locked) {
                std::string how;
                for (auto& u : t.unlocks)
                    if (u.id == e.id) how = u.text;
                text(Font::Display, 26, x + 22, cy + 38, "Locked " + e.kind, rgb(0x8C92B8));
                text_wrapped(Font::Text, 18, x + 22, cy + 64, cw - 40, "To unlock: " + how, ui::kGold, Align::Left, 1.15f);
            } else {
                text(Font::Display, 26, x + 22, cy + 38, "? ? ?", rgb(0x8C92B8));
                text(Font::Text, 18, x + 22, cy + 68, std::string(e.kind) + " you haven't met yet", rgb(0x8C92B8));
            }
        }
        y += ((entries.size() + cols - 1) / cols) * (ch + gap) + 30;
    }
    // Rigs, with the best stake won.
    if (y > 80 && y < kHeight) text(Font::Display, 28, x0, y + 8, "RIGS AND THE BEST STAKE WON", ui::kGold);
    y += 28;
    for (std::size_t i = 0; i < t.rigs.size(); ++i) {
        float x = x0 + (i % cols) * (cw + gap), cy = y + (i / cols) * (ch + gap);
        bool open = p.rig_unlocked(t, t.rigs[i].id);
        auto best = p.best_stake.find(t.rigs[i].id);
        panel(x, cy, cw, ch, 12, open ? ui::kCream : rgb(0x1A1E3A), ui::kInk, 4);
        text(Font::Display, 26, x + 22, cy + 38, open ? t.rigs[i].name : "Locked rig", open ? ui::kInk : rgb(0x8C92B8));
        text(Font::Text, 18, x + 22, cy + 68,
             open ? (best != p.best_stake.end() ? "Best stake won: " + std::to_string(best->second) : "Not won yet") : "Win a run to open the next rig",
             open ? rgb(0x4A3E50) : ui::kGold);
    }
    y += ((t.rigs.size() + cols - 1) / cols) * (ch + gap) + 40;
    float content_h = y + g.collection_scroll - top;
    float max_scroll = std::max(0.0f, content_h - (kHeight - top - 20));
    g.collection_scroll = std::clamp(g.collection_scroll - in.wheel * 90, 0.0f, max_scroll);
    if (in.key[SDL_SCANCODE_DOWN]) g.collection_scroll = std::min(max_scroll, g.collection_scroll + 240);
    if (in.key[SDL_SCANCODE_UP]) g.collection_scroll = std::max(0.0f, g.collection_scroll - 240);

    // Header over the list.
    rect(0, 0, kWidth, 112, ui::kNight);
    ui::stripes(0, 0, kWidth, 18, 60);
    rect(0, 108, kWidth, 5, ui::kInk);
    ui::sign(56, 60, 88, "THE COLLECTION", ui::kGold, Align::Left);
    text(Font::Text, 24, 700, 78, std::to_string(seen_count) + " of " + std::to_string(total) + " met. Scroll for more.", ui::kMuted);
    if (ui::button(kWidth - 300, 34, 250, 62, "BACK", ui::Style::Red, true, 30) || in.key[SDL_SCANCODE_ESCAPE]) g.screen = Screen::Title;
}

void pause_menu(Game& g) {
    ui::Input& in = ui::input();
    rect(0, 0, kWidth, kHeight, rgb(0x050817, 0.72f));
    float w = 620, h = 520, x = (kWidth - w) * 0.5f, y = 260;
    panel(x, y, w, h, 20, ui::kCream, ui::kInk, 6);
    ui::stripes(x, y, w, 18, 44);
    text(Font::Display, 48, x + w * 0.5f, y + 96, "INTERMISSION", ui::kInk, Align::Center);
    float by = y + 140;
    if (ui::button(x + 60, by, w - 120, 76, "BACK TO THE SHOW", ui::Style::Gold, true, 32) || in.key[SDL_SCANCODE_ESCAPE]) {
        g.paused = false;
        std::fill(std::begin(in.key), std::end(in.key), false);
    }
    if (ui::button(x + 60, by + 92, (w - 134) * 0.5f, 76, "SETTINGS", ui::Style::Cream, true, 26)) g.settings_open = true;
    if (ui::button(x + 74 + (w - 134) * 0.5f, by + 92, (w - 134) * 0.5f, 76, "HOW TO PLAY", ui::Style::Cream, true, 26)) g.help_open = true;
    if (ui::button(x + 60, by + 184, w - 120, 76, "SAVE AND LEAVE", ui::Style::Dark, true, 32)) {
        save_run(g);
        g.paused = false;
        g.cues.clear();
        g.payout_open = false;
        g.screen = Screen::Title;
    }
    if (ui::button(x + 60, by + 276, w - 120, 64, "ABANDON THE RUN", ui::Style::Red, true, 26)) {
        g.paused = false;
        g.cues.clear();
        g.payout_open = false;
        g.has_run = false;
        save_run(g);
        g.screen = Screen::Title;
    }
}

void settings_panel(Game& g) {
    ui::Input& in = ui::input();
    Settings& s = g.profile.settings;
    Settings before = s;
    rect(0, 0, kWidth, kHeight, rgb(0x050817, 0.78f));
    float w = 820, h = 600, x = (kWidth - w) * 0.5f, y = 220;
    panel(x, y, w, h, 20, ui::kCream, ui::kInk, 6);
    ui::stripes(x, y, w, 18, 41);
    text(Font::Display, 48, x + w * 0.5f, y + 92, "SETTINGS", ui::kInk, Align::Center);
    slider_row(x + 60, y + 130, w - 120, "Volume", s.master);
    slider_row(x + 60, y + 200, w - 120, "Music", s.music);
    slider_row(x + 60, y + 270, w - 120, "Sound", s.sfx);
    text(Font::Display, 26, x + 60, y + 374, "Animation", ui::kInk);
    const char* speeds[4] = {"Instant", "Fast", "Normal", "Slow"};
    for (int i = 0; i < 4; ++i)
        if (ui::button(x + 250 + i * 132.0f, y + 340, 120, 52, speeds[i], s.speed == i ? ui::Style::Gold : ui::Style::Dark, true, 22)) s.speed = i;
    text(Font::Display, 26, x + 60, y + 448, "Display", ui::kInk);
    if (ui::button(x + 250, y + 414, 252, 52, s.fullscreen ? "Full screen" : "Window", ui::Style::Dark, true, 22)) s.fullscreen = !s.fullscreen;
    text(Font::Text, 20, x + 520, y + 446, "F11 switches too", rgb(0x6B5F70));
    if (ui::button(x + w * 0.5f - 140, y + h - 100, 280, 70, "DONE", ui::Style::Gold, true, 32) || in.key[SDL_SCANCODE_ESCAPE]) {
        g.settings_open = false;
        std::fill(std::begin(in.key), std::end(in.key), false);
        save_profile(g);
    }
    if (before.master != s.master || before.music != s.music || before.sfx != s.sfx || before.speed != s.speed || before.fullscreen != s.fullscreen) {
        apply_settings(g);
        if (before.sfx != s.sfx || before.master != s.master) audio::play(audio::Sfx::LandCargo, 0.8f);
    }
}

void help_panel(Game& g) {
    ui::Input& in = ui::input();
    rect(0, 0, kWidth, kHeight, rgb(0x050817, 0.8f));
    float w = 1320, h = 860, x = (kWidth - w) * 0.5f, y = 110;
    panel(x, y, w, h, 20, ui::kCream, ui::kInk, 6);
    ui::stripes(x, y, w, 18, 44);
    text(Font::Display, 48, x + w * 0.5f, y + 92, "HOW TO PLAY", ui::kInk, Align::Center);
    struct Line {
        const char* head;
        const char* body;
    } lines[] = {
        {"A run", "is 12 rounds. Miss a round's score target and the run is over."},
        {"A round", "is 4 trips. Each trip, you spin wheels to load cargo onto the wagon, then deliver it for points."},
        {"The catch", "no repeats. Land a cargo value you already hold, or the same wedge twice, and the load spoils: it pays nothing."},
        {"The pull", "a load twice as big pays about four times as much. A load whose points reach the quota pays double, "
                     "and every cargo past the third pays a dollar."},
        {"The odds", "are always on show. Every wedge is face up, wedges that would spoil the load are crossed out, "
                     "and each wheel shows its chance to spoil the load."},
        {"The payout", "is points x mult. Points are the cargo's values; mult is 1 per cargo, plus sets: cargo of one color, "
                       "or a run of consecutive values. Then your items apply, in order."},
        {"The shop", "comes between rounds. Reshape your bag of wedges, add wheels, and collect items that multiply what a delivery pays."},
        {"Saves", "tarps cover one repeat. Nudge and Respin can rescue a load when a spin goes wrong. Nothing ever picks a result for you."},
    };
    float ly = y + 150;
    for (auto& l : lines) {
        text(Font::Display, 26, x + 60, ly, l.head, rgb(0xA3271F));
        float used = text_wrapped(Font::Text, 24, x + 250, ly, w - 310, l.body, ui::kInk, Align::Left, 1.25f);
        ly += std::max(46.0f, used + 16);
    }
    text(Font::Text, 22, x + w * 0.5f, y + h - 124, "Mouse: click a wheel to spin, click DELIVER to deliver. Keys: 1 to 6 spin, Space delivers, Esc pauses.",
         rgb(0x6B5F70), Align::Center);
    if (ui::button(x + w * 0.5f - 140, y + h - 96, 280, 70, "GOT IT", ui::Style::Gold, true, 32) || in.key[SDL_SCANCODE_ESCAPE] ||
        in.key[SDL_SCANCODE_RETURN]) {
        g.help_open = false;
        std::fill(std::begin(in.key), std::end(in.key), false);
    }
}

} // namespace spin
