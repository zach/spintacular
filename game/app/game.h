// The game's state, shared by its screens. Internal to game/app.
#pragma once

#include <deque>
#include <functional>
#include <string>
#include <vector>

#include "app.h"
#include "audio.h"
#include "data/profile.h"
#include "gfx.h"
#include "rules/run.h"
#include "ui.h"

namespace spin {

using gfx::Color;

enum class Screen { Title, Round, Shop, RunEnd, Collection };

// One step of the animation timeline. Cues play one after another; while any
// is queued the player can't act, only skip.
struct Cue {
    float dur = 0;
    std::function<void()> start;
    std::function<void(float)> tick; // 0..1
    std::function<void()> done;
    bool started = false;
    float t = 0;
    bool unskippable = false;
};

struct WheelView {
    float angle = 0;      // rotation, radians
    float tilt = 0;       // the flapper's kick, decaying
    float flash = 0;      // a glow after landing, decaying
    Color flash_color;
    int last_slot = -1;   // for ticks
    bool spinning = false;
    bool band = false;    // drawn with spoiling wedges half as wide
    std::vector<bool> band_clash;
};

// A prize flying from a wheel to the wagon, or off it.
struct Flyer {
    Wedge wedge;
    int value = 0;
    float x0, y0, x1, y1, t = 0, dur = 0.3f, size0 = 60, size1 = 90;
    bool falling = false;
    float vx = 0, vy = 0, x = 0, y = 0, spin = 0;
};

struct Popup {
    std::string text;
    float x, y, t = 0, dur = 1.2f, size = 44;
    Color color;
};

struct Toast {
    std::string title, body;
    float t = 0;
};

struct Game {
    am::Context* ctx = nullptr;
    AppOptions opt;
    std::string asset_dir, save_dir;
    Tables tables;
    std::string tables_text;
    Profile profile;
    Run run;
    bool has_run = false;
    Screen screen = Screen::Title;
    bool quit = false;
    float dt = 0;
    float speed = 1; // animation time multiplier; 0 is instant
    long frame_no = 0;

    // ---- animation
    std::deque<Cue> cues;
    bool skip = false;
    std::vector<Flyer> flyers;
    std::vector<Popup> popups;
    std::deque<Toast> toasts;
    float shake = 0;

    // ---- what the round screen shows; it trails the rules while cues play
    Load shown_load;
    long long shown_score = 0;
    int shown_money = 0;
    int shown_trips = 0, shown_spins = 0;
    std::vector<WheelView> wheels;
    bool band_armed = false;
    float wagon_bounce = 0;
    float quota_glow = 0;
    // The payout being played
    bool payout_open = false;
    Delivery payout;
    int payout_step = -1;
    float payout_pulse = 0;
    // Modals on the round screen
    bool round_over = false;   // the round-cleared ticket is up
    bool spoiled_banner = false;
    long long spoiled_value = 0;
    float spoiled_t = 0;
    std::string save_banner; // "A TARP SAVES IT!"
    float save_t = 0;
    int quota_hits_round = 0;

    // ---- the shop
    int target_offer = -1;  // an upgrade or Zap waiting for its wedge
    int target_uid = 0;     // the wedge picked, waiting for a value or color
    int drag_item = -1;
    float drag_dx = 0;
    bool dragging = false;

    // ---- menus
    bool paused = false, settings_open = false, help_open = false, debug_open = false;
    int rig_index = 0, stake = 1;
    std::string seed_text;
    bool seed_focus = false;
    std::vector<std::string> newly_unlocked; // this run
    bool run_won = false;
    bool run_counted = false;
    float collection_scroll = 0;

    // ---- the script, if one is playing
    int script_step = 0;
    float script_wait = 0;
    int script_shots = 0;
    std::string pending_shot;
    int script_rounds = 0;
};

// app.cpp
void cue(Game& g, float dur, std::function<void()> start, std::function<void(float)> tick = nullptr,
         std::function<void()> done = nullptr);
void wait(Game& g, float dur);
void sync_shown(Game& g);
void save_run(Game& g);
void save_profile(Game& g);
void start_run(Game& g, std::uint64_t seed, bool daily);
bool resume_run(Game& g);
void end_run(Game& g, bool won);
void unlock_check(Game& g, const char* when, int value);
void see(Game& g, const std::string& id);
void toast(Game& g, const std::string& title, const std::string& body);
void popup(Game& g, const std::string& text, float x, float y, Color color, float size = 44, float dur = 1.2f);
std::uint64_t daily_seed();
std::string daily_date();
void sfx(Game& g, audio::Sfx s, float volume = 1, float pitch = 1);
void apply_settings(Game& g);
void draw_top_bar(Game& g, bool shop);
// Draws the items as tickets. Returns the index of an item the player asked to sell, or -1.
int draw_items_row(Game& g, float x, float y, bool can_sell);
void draw_consumables(Game& g, float x, float y, bool can_arm);
std::string item_state_text(const Game& g, const OwnedItem& it);

// view_round.cpp
void enter_round(Game& g);
void round_screen(Game& g);
void act_spin(Game& g, int wheel);
void act_deliver(Game& g);
void act_rescue(Game& g, Rescue r);
void act_rod(Game& g, int uid);
void wheel_layout(const Game& g, int wheel, float* cx, float* cy, float* r);

// view_shop.cpp
void enter_shop(Game& g);
void shop_screen(Game& g);
bool shop_buy(Game& g, int offer, int uid = 0, int choice = 0);
void shop_leave(Game& g);

// menus.cpp
void title_screen(Game& g);
void run_end_screen(Game& g);
void collection_screen(Game& g);
void pause_menu(Game& g);
void settings_panel(Game& g);
void help_panel(Game& g);
void draw_backdrop(Game& g, bool booth);

// debug_panel.cpp
void debug_panel(Game& g);

// script.cpp
void script_update(Game& g);

} // namespace spin
