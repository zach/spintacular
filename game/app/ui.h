// The game's look (the carnival midway palette and the shapes of its pieces)
// and its small immediate-mode widgets. docs/design/art-and-audio.md.
#pragma once

#include <string>
#include <string_view>

#include "gfx.h"
#include "rules/run.h"

namespace ui {

using gfx::Color;

// ---- palette
constexpr Color kNight = gfx::rgb(0x0B1026), kNight2 = gfx::rgb(0x161F47), kNight3 = gfx::rgb(0x222C5E);
constexpr Color kRed = gfx::rgb(0xC8372D), kRedDark = gfx::rgb(0x8E2019), kCream = gfx::rgb(0xF6E8C6), kCreamDark = gfx::rgb(0xD9C394);
constexpr Color kGold = gfx::rgb(0xFFC53D), kGoldDim = gfx::rgb(0x6B5220), kInk = gfx::rgb(0x1A1220);
constexpr Color kSafe = gfx::rgb(0x46D17F), kWarn = gfx::rgb(0xFFB02E), kDanger = gfx::rgb(0xFF5A4F);
constexpr Color kWood = gfx::rgb(0xA9713D), kWoodDark = gfx::rgb(0x6E4522), kTarp = gfx::rgb(0x5FA8A0), kCrate = gfx::rgb(0xD2A163);
constexpr Color kWhite = gfx::rgb(0xFFFFFF), kMuted = gfx::rgb(0x9AA3C7);
Color cargo_color(int color);      // 0..3, or spin::kWild
Color rarity_color(spin::Rarity r);
const char* rarity_name(spin::Rarity r);
Color risk_color(double chance);   // green through gold to red

// ---- input, in canvas coordinates
struct Input {
    float mx = -1000, my = -1000;
    bool down = false, pressed = false, released = false; // left mouse button
    bool right_pressed = false;
    float wheel = 0;
    bool key[512] = {};  // pressed this frame, by SDL scancode
    std::string text;    // typed this frame
    bool blocked = false; // a modal is up, or an animation is playing: widgets don't react
};
Input& input();
void begin_frame(float dt);
// Draws what widgets queued for the top of the frame (tooltips).
void end_frame();
double time(); // seconds since start

bool hover(float x, float y, float w, float h);
bool key(int scancode);
void tooltip(std::string_view title, std::string_view body, Color accent = kGold);

enum class Style { Red, Gold, Cream, Dark, Green };
// A painted button. Returns true when clicked.
bool button(float x, float y, float w, float h, std::string_view label, Style style = Style::Red, bool enabled = true,
            float text_size = 0);
void (*&click_sound())(int kind); // 0 click, 1 hover, 2 refused

// ---- pieces
// A cargo prize: its color's shape, with its value on it.
void cargo(float cx, float cy, float size, int color, int value, int bonus = 0, bool dark = false);
void tarp_icon(float cx, float cy, float size, bool dark = false);
void crate_icon(float cx, float cy, float size, bool dark = false);
void wedge_token(float cx, float cy, float size, const spin::Wedge& w, int shown_value = -1, bool dark = false);
void cross(float cx, float cy, float size, Color color, float thickness);
// A bulb: lit ones glow.
void bulb(float cx, float cy, float r, bool lit, Color color = kGold);
// A row of bulbs along a line; `lit` of them are on, or with chase >= 0 every third one, moving.
void bulb_row(float x0, float y0, float x1, float y1, int count, int lit, float r, int chase = -1);
// A cream ticket with a notched edge.
void ticket(float x, float y, float w, float h, Color fill = kCream, Color edge = kInk);
// Red and cream tent stripes, as a band.
void stripes(float x, float y, float w, float h, float stripe_w, Color a = kRed, Color b = kCream);
// A sign: cream text with an ink outline.
void sign(float size, float x, float y, std::string_view s, Color color = kCream, gfx::Align align = gfx::Align::Center);
void money(float size, float x, float y, int dollars, gfx::Align align = gfx::Align::Left);
std::string number(long long v);  // 12,345
std::string wedge_name(const spin::Wedge& w);

} // namespace ui
