#include "app.h"

#include <algorithm>
#include <cmath>
#include <ctime>
#include <filesystem>

#include <SDL3/SDL.h>
#include <imgui.h>
#include <imgui_impl_sdl3.h>
#include <imgui_impl_sdlgpu3.h>
#include <miniaudio.h>

#include "automap/diag.h"
#include "data/save.h"
#include "data/tables_json.h"
#include "game.h"
#include "stb-image/stb_image_assets.h"

namespace spin {

using namespace gfx;

namespace {

void log(am::diag::Level level, const std::string& message) { am::diag::log(level, "spintacular", message.c_str()); }

void on_click(int kind) {
    audio::play(kind == 2 ? audio::Sfx::Refuse : audio::Sfx::Click, kind == 2 ? 0.5f : 0.6f);
}

std::string find_assets() {
    const char* base = SDL_GetBasePath();
    std::string beside = std::string(base ? base : "") + "assets";
    std::error_code ec;
    if (std::filesystem::exists(beside + "/data/tables.json", ec)) return beside;
    if (std::filesystem::exists("assets/data/tables.json", ec)) return "assets";
    return beside;
}

// Puts the scene on the window, and Dear ImGui's panels over it.
void present(Game& g) {
    am::diag::Zone zone("present");
    ImGui::Render();
    ImDrawData* draw_data = ImGui::GetDrawData();
    SDL_GPUDevice* device = g.ctx->sdl3.gpu_device;
    SDL_GPUCommandBuffer* cmd = SDL_AcquireGPUCommandBuffer(device);
    if (!cmd) return;
    gfx::end_frame(cmd);
    SDL_GPUTexture* swapchain = nullptr;
    Uint32 sw = 0, sh = 0;
    SDL_WaitAndAcquireGPUSwapchainTexture(cmd, g.ctx->sdl3.window, &swapchain, &sw, &sh);
    if (swapchain != nullptr) {
        int w = 0, h = 0;
        gfx::scene_size(&w, &h);
        SDL_GPUBlitInfo blit = {};
        blit.source.texture = gfx::scene_texture();
        blit.source.w = static_cast<Uint32>(w);
        blit.source.h = static_cast<Uint32>(h);
        blit.destination.texture = swapchain;
        blit.destination.w = sw;
        blit.destination.h = sh;
        blit.load_op = SDL_GPU_LOADOP_DONT_CARE;
        blit.filter = SDL_GPU_FILTER_LINEAR;
        SDL_BlitGPUTexture(cmd, &blit);
        bool minimized = draw_data->DisplaySize.x <= 0.0f || draw_data->DisplaySize.y <= 0.0f;
        if (!minimized && draw_data->CmdListsCount > 0) {
            ImGui_ImplSDLGPU3_PrepareDrawData(draw_data, cmd);
            SDL_GPUColorTargetInfo target = {};
            target.texture = swapchain;
            target.load_op = SDL_GPU_LOADOP_LOAD;
            target.store_op = SDL_GPU_STOREOP_STORE;
            SDL_GPURenderPass* pass = SDL_BeginGPURenderPass(cmd, &target, 1, nullptr);
            ImGui_ImplSDLGPU3_RenderDrawData(draw_data, cmd, pass);
            SDL_EndGPURenderPass(pass);
        }
    }
    SDL_SubmitGPUCommandBuffer(cmd);
}

void run_cues(Game& g) {
    float budget = g.dt;
    int guard = 0;
    while (!g.cues.empty() && guard++ < 200) {
        Cue& c = g.cues.front();
        if (!c.started) {
            c.started = true;
            if (c.start) c.start();
        }
        float dur = c.dur * g.speed;
        if (g.skip && !c.unskippable) dur = 0;
        c.t += budget;
        if (c.t < dur) {
            if (c.tick) c.tick(dur > 0 ? c.t / dur : 1.0f);
            return;
        }
        budget = c.t - dur;
        if (c.tick) c.tick(1.0f);
        auto done = std::move(c.done);
        g.cues.pop_front();
        if (done) done();
    }
    if (g.cues.empty()) g.skip = false;
}

void update_effects(Game& g) {
    for (auto& f : g.flyers) {
        f.t += g.dt;
        if (f.falling) {
            f.vy += 2600 * g.dt;
            f.x += f.vx * g.dt;
            f.y += f.vy * g.dt;
            f.spin += 4 * g.dt;
        }
    }
    g.flyers.erase(std::remove_if(g.flyers.begin(), g.flyers.end(), [](const Flyer& f) { return f.falling ? f.y > kHeight + 200 : f.t >= f.dur; }),
                   g.flyers.end());
    for (auto& p : g.popups) p.t += g.dt;
    g.popups.erase(std::remove_if(g.popups.begin(), g.popups.end(), [](const Popup& p) { return p.t >= p.dur; }), g.popups.end());
    if (!g.toasts.empty()) {
        g.toasts.front().t += g.dt;
        if (g.toasts.front().t > 4.0f) g.toasts.pop_front();
    }
    g.shake = std::max(0.0f, g.shake - g.dt * 3.0f);
    g.wagon_bounce = std::max(0.0f, g.wagon_bounce - g.dt * 3.5f);
    for (auto& w : g.wheels) {
        w.tilt = std::max(0.0f, w.tilt - g.dt * 9.0f);
        w.flash = std::max(0.0f, w.flash - g.dt * 1.6f);
    }
    g.payout_pulse = std::max(0.0f, g.payout_pulse - g.dt * 4.0f);
    g.save_t = std::max(0.0f, g.save_t - g.dt);
    g.spoiled_t = std::max(0.0f, g.spoiled_t - g.dt);
}

void draw_effects(Game& g) {
    for (auto& f : g.flyers) {
        if (f.falling) {
            push(f.x, f.y, f.spin, 1);
            ui::wedge_token(0, 0, f.size1, f.wedge, f.value);
            pop();
        } else {
            float t = std::min(1.0f, f.t / f.dur);
            float e = 1 - (1 - t) * (1 - t);
            float x = f.x0 + (f.x1 - f.x0) * e, y = f.y0 + (f.y1 - f.y0) * e - std::sin(t * kPi) * 120;
            ui::wedge_token(x, y, f.size0 + (f.size1 - f.size0) * e, f.wedge, f.value);
        }
    }
    for (auto& p : g.popups) {
        float t = p.t / p.dur;
        float a = t < 0.75f ? 1.0f : (1 - t) / 0.25f;
        float pop_scale = t < 0.12f ? 0.6f + t / 0.12f * 0.5f : 1.1f - std::min(0.1f, (t - 0.12f) * 0.5f);
        push_alpha(a);
        ui::sign(p.size * pop_scale, p.x, p.y - t * 60, p.text, p.color);
        pop_alpha();
    }
    if (!g.toasts.empty()) {
        const Toast& t = g.toasts.front();
        float in = std::min(1.0f, t.t / 0.25f), out = std::min(1.0f, (4.0f - t.t) / 0.4f);
        if (g.screen == Screen::Round) {
            // In a round, the free band between the wheels and the wagon.
            float w = 860, x = (kWidth - w) * 0.5f, y = 712 - (1 - in * out) * 20;
            push_alpha(in * out);
            ui::ticket(x, y, w, 60, ui::kGold);
            float tw = text_width(Font::Display, 26, t.title);
            text(Font::Display, 26, x + 28, y + 40, t.title, ui::kInk);
            text(Font::Text, 22, x + 44 + tw, y + 39, t.body, ui::kInk);
            pop_alpha();
        } else {
            float x = kWidth - 520 + (1 - in * out) * 560, y = 118;
            ui::ticket(x, y, 490, 104, ui::kGold);
            text(Font::Display, 30, x + 24, y + 42, t.title, ui::kInk);
            text(Font::Text, 24, x + 24, y + 80, t.body, ui::kInk);
        }
    }
}

} // namespace

// ---------------------------------------------------------------- shared helpers

void cue(Game& g, float dur, std::function<void()> start, std::function<void(float)> tick, std::function<void()> done) {
    Cue c;
    c.dur = dur;
    c.start = std::move(start);
    c.tick = std::move(tick);
    c.done = std::move(done);
    g.cues.push_back(std::move(c));
}

void wait(Game& g, float dur) { cue(g, dur, nullptr); }

void sfx(Game&, audio::Sfx s, float volume, float pitch) { audio::play(s, volume, pitch); }

void popup(Game& g, const std::string& text, float x, float y, Color color, float size, float dur) {
    g.popups.push_back({text, x, y, 0, dur, size, color});
}

void toast(Game& g, const std::string& title, const std::string& body) { g.toasts.push_back({title, body, 0}); }

void sync_shown(Game& g) {
    g.shown_load = g.run.load;
    g.shown_score = g.run.round_score;
    g.shown_money = g.run.money;
    g.shown_trips = g.run.trips_left;
    g.shown_spins = g.run.spins_left();
}

void apply_settings(Game& g) {
    const Settings& s = g.profile.settings;
    static const float speeds[4] = {0.0f, 0.5f, 1.0f, 1.6f};
    g.speed = speeds[std::clamp(s.speed, 0, 3)];
    audio::volumes(g.opt.mute ? 0.0f : s.master, s.music, s.sfx);
    if (!g.opt.headless) SDL_SetWindowFullscreen(g.ctx->sdl3.window, s.fullscreen);
}

void save_profile(Game& g) {
    if (g.save_dir.empty()) return;
    write_file(g.save_dir + "profile.json", profile_to_json(g.profile));
}

void save_run(Game& g) {
    if (g.save_dir.empty()) return;
    std::error_code ec;
    if (g.has_run && g.run.phase != Phase::Lost)
        write_file(g.save_dir + "run.json", run_to_json(g.run));
    else
        std::filesystem::remove(g.save_dir + "run.json", ec);
}

void see(Game& g, const std::string& id) {
    if (!g.profile.has_seen(id)) g.profile.see(id);
}

std::string daily_date() {
    std::time_t now = std::time(nullptr);
    std::tm tm = *std::gmtime(&now);
    char buf[16];
    std::strftime(buf, sizeof buf, "%Y%m%d", &tm);
    return buf;
}

std::uint64_t daily_seed() { return std::strtoull(daily_date().c_str(), nullptr, 10); }

void unlock_check(Game& g, const char* when, int value) {
    for (auto& u : g.tables.unlocks) {
        if (u.when != when || value < u.n) continue;
        if (!g.profile.unlock(u.id)) continue;
        g.newly_unlocked.push_back(u.id);
        std::string name = u.id;
        if (auto* d = g.tables.item(u.id)) name = d->name;
        else if (auto* w = g.tables.wheel(u.id)) name = w->name;
        else if (auto* up = g.tables.upgrade(u.id)) name = up->name;
        else if (auto* c = g.tables.consumable(u.id)) name = c->name;
        toast(g, "Unlocked: " + name, u.text);
        audio::play(audio::Sfx::Unlock, 0.8f);
        save_profile(g);
    }
}

void start_run(Game& g, std::uint64_t seed, bool daily) {
    RunConfig cfg;
    cfg.seed = seed;
    cfg.daily = daily;
    cfg.rig = daily ? "standard" : g.tables.rigs[std::clamp(g.rig_index, 0, static_cast<int>(g.tables.rigs.size()) - 1)].id;
    cfg.stake = daily ? 1 : g.stake;
    cfg.locked = g.profile.locked_now(g.tables);
    g.run = Run(g.tables, cfg);
    g.has_run = true;
    g.run_won = false;
    g.run_counted = false;
    g.newly_unlocked.clear();
    g.profile.runs += 1;
    if (daily) {
        g.profile.daily_date = daily_date();
        g.profile.daily_result = "In progress";
    }
    save_profile(g);
    enter_round(g);
    save_run(g);
}

bool resume_run(Game& g) {
    if (g.save_dir.empty()) return false;
    std::string text = read_file(g.save_dir + "run.json");
    if (text.empty() || !run_from_json(g.tables, text, g.run)) return false;
    g.has_run = true;
    g.run_won = g.run.endless;
    g.run_counted = g.run.endless;
    g.newly_unlocked.clear();
    if (g.run.phase == Phase::Shop)
        enter_shop(g);
    else if (g.run.phase == Phase::Won) {
        g.run_won = true;
        g.screen = Screen::RunEnd;
    } else
        enter_round(g);
    return true;
}

void end_run(Game& g, bool won) {
    Profile& p = g.profile;
    p.best_round = std::max(p.best_round, g.run.round + (won ? 1 : 0));
    p.best_delivery = std::max(p.best_delivery, g.run.m.best_delivery);
    if (won && !g.run_counted) {
        g.run_counted = true;
        p.wins += 1;
        int& best = p.best_stake[g.run.config.rig];
        best = std::max(best, g.run.config.stake);
        unlock_check(g, "win_run", 1);
    }
    if (g.run.config.daily && p.daily_date == daily_date())
        p.daily_result = won ? "Won the daily haul" : "Reached round " + std::to_string(g.run.round + 1);
    g.run_won = won;
    save_profile(g);
    save_run(g);
    g.screen = Screen::RunEnd;
    g.paused = false;
    audio::play(won ? audio::Sfx::RunWon : audio::Sfx::RunLost, 0.9f);
}

std::string item_state_text(const Game& g, const OwnedItem& it) {
    const ItemDef* d = g.run.t.item(it.id);
    if (!d) return "";
    char buf[48] = "";
    if (d->effect == ItemEffect::ScalingMultSize) std::snprintf(buf, sizeof buf, "+%.0f mult so far", it.state);
    if (d->effect == ItemEffect::ScalingXmultQuota) std::snprintf(buf, sizeof buf, "x%.1f so far", 1.0 + it.state);
    return buf;
}

void draw_top_bar(Game& g, bool shop) {
    Run& r = g.run;
    ui::stripes(0, 0, kWidth, 18, 60);
    rect(0, 18, kWidth, 5, ui::kInk);
    gradient_rect(0, 23, kWidth, 79, ui::kNight3, ui::kNight2);
    rect(0, 100, kWidth, 5, ui::kInk);

    int rounds = r.t.run.rounds;
    std::string round_label = r.round < rounds ? "ROUND " + std::to_string(r.round + 1) + " / " + std::to_string(rounds)
                                               : "ENDLESS " + std::to_string(r.round + 1);
    ui::sign(46, 34, 80, round_label, ui::kCream, Align::Left);
    int route = r.round / 3 + 1, stop = r.round % 3 + 1;
    const Mods* rig = g.tables.rig(r.config.rig);
    text(Font::Text, 20, 392, 58,
         "Route " + std::to_string(route) + ", stop " + std::to_string(stop) + "  \xC2\xB7  " + (rig ? rig->name : r.config.rig) + ", stake " +
             std::to_string(r.config.stake),
         ui::kMuted);
    text(Font::Text, 20, 392, 86, (r.config.daily ? "Daily haul " : "Seed ") + std::to_string(r.config.seed), ui::kMuted);
    if (!ui::input().blocked && ui::hover(392, 66, 300, 28)) {
        ui::tooltip("Seed", "Every run comes from one seed. Type it on the start screen to replay this run. Click to copy it.");
        if (ui::input().pressed) {
            SDL_SetClipboardText(std::to_string(r.config.seed).c_str());
            toast(g, "Seed copied", std::to_string(r.config.seed));
        }
    }

    // Score against the target.
    float bx = 800, bw = 560, by = 40, bh = 44;
    long long target = r.target();
    float frac = target > 0 ? std::min(1.0f, static_cast<float>(g.shown_score) / static_cast<float>(target)) : 1;
    panel(bx, by, bw, bh, 12, ui::kNight, ui::kInk, 4);
    if (frac > 0) rrect(bx, by, std::max(24.0f, bw * frac), bh, 12, frac >= 1 ? ui::kSafe : ui::kGold);
    std::string score = shop ? "Cleared: " + ui::number(g.shown_score) + " / " + ui::number(target)
                             : ui::number(g.shown_score) + " / " + ui::number(target);
    text_outlined(Font::Display, 30, bx + bw * 0.5f, by + 33, score, ui::kCream, ui::kInk, 3, Align::Center);
    text(Font::Text, 20, bx - 12, by + 30, "SCORE", ui::kMuted, Align::Right);

    ui::money(46, kWidth - 34, 80, g.shown_money, Align::Right);

    // The inspection: this round's, pasted on the header, or the next one ahead.
    Inspection now = r.inspection();
    if (!shop && now != Inspection::None) {
        const InspectionDef* d = r.t.inspection(now);
        float w = 560, x = bx + bw * 0.5f - w * 0.5f, y = 86;
        push(x + w * 0.5f, y + 22, -0.012f, 1);
        panel(-w * 0.5f, -22, w, 46, 6, ui::kGold, ui::kInk, 4);
        text(Font::Display, 24, 0, 9, std::string("INSPECTION: ") + (d ? d->name + ". " + d->text : ""), ui::kInk, Align::Center);
        pop();
    } else {
        int next = -1;
        for (int k = r.round + (shop ? 1 : 0); k < static_cast<int>(r.route.size()); ++k)
            if (r.route[k] != Inspection::None) {
                next = k;
                break;
            }
        if (next >= 0) {
            const InspectionDef* d = r.t.inspection(r.route[next]);
            text(Font::Text, 20, 1384, 58, "Inspection at round " + std::to_string(next + 1) + ":", ui::kGold);
            if (d) text(Font::Text, 20, 1384, 84, d->name + ". " + d->text, ui::kGold);
            if (d) see(g, d->id);
        }
    }
}

int draw_items_row(Game& g, float x, float y, bool can_sell) {
    Run& r = g.run;
    const float w = 262, h = 80, gap = 14;
    int sell = -1;
    int slots = r.t.run.max_items;
    ui::Input& in = ui::input();
    int drop_slot = -1;
    if (g.dragging && g.drag_item >= 0) drop_slot = std::clamp(static_cast<int>((in.mx - x + gap * 0.5f) / (w + gap)), 0, static_cast<int>(r.items.size()) - 1);
    for (int i = 0; i < slots; ++i) {
        float sx = x + i * (w + gap);
        rrect(sx, y, w, h, 10, rgb(0x000000, 0.28f));
        if (i >= static_cast<int>(r.items.size())) {
            text(Font::Text, 20, sx + w * 0.5f, y + h * 0.5f + 7, i == static_cast<int>(r.items.size()) ? "item slot" : "", rgb(0xFFFFFF, 0.25f),
                 Align::Center);
        }
    }
    for (int i = 0; i < static_cast<int>(r.items.size()); ++i) {
        const OwnedItem& it = r.items[i];
        const ItemDef* d = r.t.item(it.id);
        if (!d) continue;
        int slot = i;
        if (drop_slot >= 0 && i != g.drag_item) { // make room where the dragged ticket would land
            if (g.drag_item < i && drop_slot >= i) slot = i - 1;
            if (g.drag_item > i && drop_slot <= i) slot = i + 1;
        }
        float sx = x + slot * (w + gap), sy = y;
        bool is_drag = g.dragging && i == g.drag_item;
        if (is_drag) {
            sx = in.mx - g.drag_dx;
            sy = y - 10;
        }
        bool over = !in.blocked && !g.dragging && ui::hover(sx, sy, w, h);
        Color accent = ui::rarity_color(d->rarity);
        ui::ticket(sx, sy - (over ? 4 : 0), w, h, is_drag ? lighten(ui::kCream, 0.4f) : ui::kCream);
        float ty = sy - (over ? 4 : 0);
        rect(sx + 8, ty + 8, 10, h - 16, darken(accent, 0.25f));
        circle(sx + 40, ty + 26, 15, ui::kInk);
        text(Font::Display, 20, sx + 40, ty + 33, std::to_string(i + 1), ui::kCream, Align::Center);
        float ns = 24;
        float nw = text_width(Font::Display, ns, d->name);
        if (nw > w - 78) ns *= (w - 78) / nw;
        text(Font::Display, ns, sx + 64, ty + 34, d->name, ui::kInk);
        std::string state = item_state_text(g, it);
        std::string line = state.empty() ? d->text : state;
        float ls = 18;
        float lw = text_width(Font::Text, ls, line);
        if (lw > w - 40) {
            // Two lines for longer text.
            text_wrapped(Font::Text, 15, sx + 26, ty + 54, w - 40, line, rgb(0x4A3E50), Align::Left, 1.1f);
        } else {
            text(Font::Text, ls, sx + 26, ty + 62, line, rgb(0x4A3E50));
        }
        if (over) {
            std::string body = d->text;
            if (!state.empty()) body += "\n" + state;
            body += std::string("\n") + ui::rarity_name(d->rarity) + ". Applies " + (i == 0 ? "first" : "in position " + std::to_string(i + 1)) +
                    "; drag to reorder.";
            if (can_sell) body += "\nRight-click to sell for $" + std::to_string(r.sell_value(it)) + ".";
            ui::tooltip(d->name, body, accent);
            if (in.pressed) {
                g.drag_item = i;
                g.drag_dx = in.mx - sx;
                g.dragging = false;
            }
            if (can_sell && in.right_pressed) sell = i;
        }
    }
    // Dragging: starts once the mouse moves a little; drops on release.
    if (g.drag_item >= 0) {
        if (g.drag_item >= static_cast<int>(r.items.size())) {
            g.drag_item = -1;
            g.dragging = false;
        } else if (!in.down) {
            if (g.dragging && drop_slot >= 0 && drop_slot != g.drag_item) {
                r.move_item(g.drag_item, drop_slot);
                audio::play(audio::Sfx::Click, 0.5f, 1.3f);
                save_run(g);
            }
            g.drag_item = -1;
            g.dragging = false;
        } else if (!g.dragging) {
            float home = x + g.drag_item * (w + gap) + g.drag_dx;
            if (std::fabs(in.mx - home) > 14) g.dragging = true;
        }
    }
    return sell;
}

void draw_consumables(Game& g, float x, float y, bool can_arm) {
    Run& r = g.run;
    const float w = 176, h = 80, gap = 12;
    for (int i = 0; i < r.t.run.max_consumables; ++i) {
        float sx = x + i * (w + gap);
        rrect(sx, y, w, h, 10, rgb(0x000000, 0.28f));
        if (i >= static_cast<int>(r.consumables.size())) continue;
        const SimpleDef* d = r.t.consumable(r.consumables[i]);
        if (!d) continue;
        bool band = d->id == "band";
        bool armed = band && g.band_armed;
        bool over = !ui::input().blocked && ui::hover(sx, y, w, h);
        panel(sx, y - (over ? 4 : 0), w, h, 40, armed ? ui::kGold : rgb(0x2F8F86), ui::kInk, 4);
        text(Font::Display, 24, sx + w * 0.5f, y + 36 - (over ? 4 : 0), d->name, armed ? ui::kInk : ui::kCream, Align::Center);
        text(Font::Text, 17, sx + w * 0.5f, y + 60 - (over ? 4 : 0), band ? (armed ? "armed: next spin" : (can_arm ? "click to arm" : "one use")) : "saves a load",
             armed ? ui::kInk : rgb(0xD5F0EC), Align::Center);
        if (over) {
            ui::tooltip(d->name, d->text + (band ? ". Click to arm it for your next spin." : ". You're asked when a spin would spoil the load."),
                        rgb(0x2F8F86));
            if (band && can_arm && ui::input().pressed) {
                g.band_armed = !g.band_armed;
                audio::play(audio::Sfx::Click, 0.6f, g.band_armed ? 1.3f : 0.8f);
            }
        }
    }
    if (g.band_armed && r.count_consumable("band") == 0) g.band_armed = false;
}

// ---------------------------------------------------------------- the app

bool App::init(am::Context& ctx, const AppOptions& options) {
    g = new Game();
    g->ctx = &ctx;
    g->opt = options;
    g->asset_dir = find_assets();
    if (options.width > 0 && options.height > 0) SDL_SetWindowSize(ctx.sdl3.window, options.width, options.height);
    try {
        g->tables_text = read_file(g->asset_dir + "/data/tables.json");
        g->tables = tables_from_json(g->tables_text);
    } catch (const std::exception& e) {
        log(am::diag::Level::error, e.what());
        return false;
    }
    if (!gfx::init(ctx.sdl3.gpu_device, ctx.sdl3.window, g->asset_dir)) {
        log(am::diag::Level::error, "the renderer couldn't start");
        return false;
    }
    {
        // The window's icon, decoded by stb_image through the assets interface.
        int w = 0, h = 0, channels = 0;
        std::string icon = g->asset_dir + "/icon.png";
        if (stbi_uc* pixels = am_stb_image_load(icon.c_str(), &w, &h, &channels, 4)) {
            if (SDL_Surface* surface = SDL_CreateSurfaceFrom(w, h, SDL_PIXELFORMAT_RGBA32, pixels, w * 4)) {
                SDL_SetWindowIcon(ctx.sdl3.window, surface);
                SDL_DestroySurface(surface);
            }
            stbi_image_free(pixels);
        }
    }
    if (!audio::init(ctx.miniaudio.engine)) log(am::diag::Level::warn, "no sound: the audio layer couldn't start");
    ui::click_sound() = &on_click;

    if (!options.profile_dir.empty()) {
        g->save_dir = options.profile_dir;
        if (g->save_dir.back() != '/' && g->save_dir.back() != '\\') g->save_dir += '/';
        std::error_code ec;
        std::filesystem::create_directories(g->save_dir, ec);
    } else if (!options.headless) {
        char* pref = SDL_GetPrefPath("Spintacular", "Spintacular");
        if (pref) {
            g->save_dir = pref;
            SDL_free(pref);
        }
    }
    if (!g->save_dir.empty()) g->profile = profile_from_json(read_file(g->save_dir + "profile.json"));
    if (options.headless) g->profile.settings.fullscreen = false;
    apply_settings(*g);
    ImGui::GetIO().IniFilename = nullptr;
    SDL_StartTextInput(ctx.sdl3.window);
    audio::music(2, true);
    log(am::diag::Level::info, "Spintacular started; assets at " + g->asset_dir);
    return true;
}

bool App::frame() {
    am::diag::Zone zone("frame");
    Game& game = *g;
    static Uint64 last = SDL_GetTicksNS();
    Uint64 now = SDL_GetTicksNS();
    game.dt = game.opt.headless ? 1.0f / 60.0f : std::min(0.05f, static_cast<float>(now - last) / 1e9f);
    last = now;
    ++game.frame_no;

    ui::Input& in = ui::input();
    in.pressed = in.released = in.right_pressed = false;
    in.wheel = 0;
    in.text.clear();
    std::fill(std::begin(in.key), std::end(in.key), false);
    SDL_Event event;
    bool imgui_mouse = ImGui::GetIO().WantCaptureMouse, imgui_keys = ImGui::GetIO().WantCaptureKeyboard;
    while (SDL_PollEvent(&event)) {
        ImGui_ImplSDL3_ProcessEvent(&event);
        switch (event.type) {
        case SDL_EVENT_QUIT:
        case SDL_EVENT_WINDOW_CLOSE_REQUESTED: game.quit = true; break;
        case SDL_EVENT_MOUSE_MOTION: {
            Vec2 p = to_canvas(event.motion.x, event.motion.y);
            in.mx = p.x;
            in.my = p.y;
            break;
        }
        case SDL_EVENT_MOUSE_BUTTON_DOWN:
        case SDL_EVENT_MOUSE_BUTTON_UP: {
            if (imgui_mouse) break;
            Vec2 p = to_canvas(event.button.x, event.button.y);
            in.mx = p.x;
            in.my = p.y;
            bool down = event.type == SDL_EVENT_MOUSE_BUTTON_DOWN;
            if (event.button.button == SDL_BUTTON_LEFT) {
                in.down = down;
                (down ? in.pressed : in.released) = true;
            } else if (event.button.button == SDL_BUTTON_RIGHT && down) {
                in.right_pressed = true;
            }
            break;
        }
        case SDL_EVENT_MOUSE_WHEEL:
            if (!imgui_mouse) in.wheel += event.wheel.y;
            break;
        case SDL_EVENT_KEY_DOWN:
            if (!imgui_keys && event.key.scancode < 512 && !event.key.repeat) in.key[event.key.scancode] = true;
            if (!imgui_keys && event.key.scancode == SDL_SCANCODE_BACKSPACE) in.key[SDL_SCANCODE_BACKSPACE] = true;
            break;
        case SDL_EVENT_TEXT_INPUT:
            if (!imgui_keys) in.text += event.text.text;
            break;
        default: break;
        }
    }
    if (in.key[SDL_SCANCODE_F1]) game.debug_open = !game.debug_open;
    if (in.key[SDL_SCANCODE_F11]) {
        game.profile.settings.fullscreen = !game.profile.settings.fullscreen;
        apply_settings(game);
        save_profile(game);
    }

    ImGui_ImplSDLGPU3_NewFrame();
    ImGui_ImplSDL3_NewFrame();
    ImGui::NewFrame();
    if (game.debug_open) debug_panel(game);

    if (!game.opt.script.empty()) script_update(game);

    // A click while a payout or a spin plays skips to its end.
    if (!game.cues.empty() && (in.pressed || in.key[SDL_SCANCODE_SPACE] || in.key[SDL_SCANCODE_RETURN])) game.skip = true;
    run_cues(game);
    update_effects(game);

    bool modal = game.paused || game.settings_open || game.help_open;
    in.blocked = !game.cues.empty() || modal;

    gfx::begin_frame();
    ui::begin_frame(game.dt);
    if (game.shake > 0) {
        float a = game.shake * game.shake * 14;
        push(std::sin(game.frame_no * 1.7f) * a, std::cos(game.frame_no * 2.3f) * a);
    } else
        push(0, 0);
    switch (game.screen) {
    case Screen::Title: title_screen(game); break;
    case Screen::Round: round_screen(game); break;
    case Screen::Shop: shop_screen(game); break;
    case Screen::RunEnd: run_end_screen(game); break;
    case Screen::Collection: collection_screen(game); break;
    }
    draw_effects(game);
    pop();
    if (game.paused && !game.settings_open && !game.help_open) {
        in.blocked = false;
        pause_menu(game);
    }
    if (game.settings_open) {
        in.blocked = false;
        settings_panel(game);
    }
    if (game.help_open) {
        in.blocked = false;
        help_panel(game);
    }
    ui::end_frame();
    present(game);

    if (!game.pending_shot.empty()) {
        gfx::save_screenshot(game.pending_shot);
        game.pending_shot.clear();
    }
    return !game.quit;
}

void App::shutdown() {
    if (!g) return;
    save_run(*g);
    save_profile(*g);
    SDL_WaitForGPUIdle(g->ctx->sdl3.gpu_device);
    audio::shutdown();
    gfx::shutdown();
    delete g;
    g = nullptr;
}

} // namespace spin
