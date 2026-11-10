#include "ui.h"

#include <algorithm>
#include <cmath>
#include <vector>

namespace ui {

using namespace gfx;

namespace {
Input g_input;
double g_time = 0;
void (*g_click)(int) = nullptr;
struct Tip {
    std::string title, body;
    Color accent;
    bool set = false;
} g_tip;
} // namespace

Input& input() { return g_input; }
double time() { return g_time; }
void (*&click_sound())(int) { return g_click; }

void begin_frame(float dt) {
    g_time += dt;
    g_tip.set = false;
}

Color cargo_color(int color) {
    switch (color) {
    case 0: return rgb(0xE5484D);
    case 1: return rgb(0x3DBB6E);
    case 2: return rgb(0x3E8BEB);
    case 3: return rgb(0xF2C230);
    default: return rgb(0xE9E2F5);
    }
}

Color rarity_color(spin::Rarity r) {
    return r == spin::Rarity::Rare ? rgb(0xE86BD0) : r == spin::Rarity::Uncommon ? rgb(0x5CC8FF) : rgb(0xBFC6DE);
}
const char* rarity_name(spin::Rarity r) {
    return r == spin::Rarity::Rare ? "Rare" : r == spin::Rarity::Uncommon ? "Uncommon" : "Common";
}

Color risk_color(double p) {
    if (p <= 0.0) return kSafe;
    if (p < 0.34) return mix(kSafe, kWarn, static_cast<float>(p / 0.34));
    return mix(kWarn, kDanger, static_cast<float>(std::min(1.0, (p - 0.34) / 0.33)));
}

bool hover(float x, float y, float w, float h) {
    return g_input.mx >= x && g_input.mx < x + w && g_input.my >= y && g_input.my < y + h;
}
bool key(int scancode) { return !g_input.blocked && scancode >= 0 && scancode < 512 && g_input.key[scancode]; }

void tooltip(std::string_view title, std::string_view body, Color accent) {
    g_tip.title = title;
    g_tip.body = body;
    g_tip.accent = accent;
    g_tip.set = true;
}

void end_frame() {
    if (!g_tip.set) return;
    const float w = 430, pad = 18;
    float body_h = g_tip.body.empty() ? 0 : text_wrapped(Font::Text, 24, 0, 0, w - 2 * pad, g_tip.body, kInk, Align::Left, 1.25f, true);
    float h = pad + 34 + body_h + pad - 6;
    float x = std::clamp(g_input.mx + 24, 10.0f, kWidth - w - 10);
    float y = g_input.my + 30;
    if (y + h > kHeight - 10) y = g_input.my - h - 16;
    rrect(x + 5, y + 7, w, h, 12, rgb(0x000000, 0.35f));
    panel(x, y, w, h, 12, kCream, kInk, 4);
    rect(x, y + 46, w, 3, g_tip.accent.with_alpha(0.9f));
    text(Font::Display, 26, x + pad, y + 34, g_tip.title, kInk);
    if (!g_tip.body.empty()) text_wrapped(Font::Text, 24, x + pad, y + 78, w - 2 * pad, g_tip.body, rgb(0x33283A));
}

bool button(float x, float y, float w, float h, std::string_view label, Style style, bool enabled, float text_size) {
    bool over = enabled && !g_input.blocked && hover(x, y, w, h);
    bool held = over && g_input.down;
    Color face = kRed, lip = kRedDark, ink = kCream;
    switch (style) {
    case Style::Gold: face = kGold; lip = rgb(0xB67F12); ink = kInk; break;
    case Style::Cream: face = kCream; lip = kCreamDark; ink = kInk; break;
    case Style::Dark: face = kNight3; lip = kNight; ink = kCream; break;
    case Style::Green: face = rgb(0x2FA865); lip = rgb(0x1B7243); ink = kCream; break;
    default: break;
    }
    if (!enabled) {
        face = rgb(0x4A4F6B);
        lip = rgb(0x2E3247);
        ink = rgb(0x8B90A8);
    } else if (over) {
        face = lighten(face, 0.14f);
    }
    float drop = held ? 2.0f : 7.0f;
    float r = std::min(h * 0.28f, 18.0f);
    rrect(x - 4, y - 4 + 7, w + 8, h + 8, r + 4, kInk);
    rrect(x, y + 7, w, h, r, lip);
    rrect(x, y + 7 - drop, w, h, r, face);
    float ts = text_size > 0 ? text_size : std::min(h * 0.5f, 44.0f);
    float tw = text_width(Font::Display, ts, label);
    if (tw > w - 24) ts *= (w - 24) / tw;
    text(Font::Display, ts, x + w * 0.5f, y + 7 - drop + h * 0.5f + ts * 0.34f, label, ink, Align::Center);
    bool clicked = over && g_input.pressed;
    if (clicked && g_click) g_click(0);
    if (!enabled && !g_input.blocked && hover(x, y, w, h) && g_input.pressed && g_click) g_click(2);
    return clicked;
}

void cross(float cx, float cy, float size, Color color, float thickness) {
    float h = size * 0.5f;
    line({cx - h, cy - h}, {cx + h, cy + h}, thickness, color);
    line({cx - h, cy + h}, {cx + h, cy - h}, thickness, color);
}

void cargo(float cx, float cy, float size, int color, int value, int bonus, bool dark) {
    Color c = cargo_color(color);
    Color ink = kInk;
    if (dark) {
        c = mix(c, rgb(0x2A2638), 0.72f);
        ink = rgb(0x0E0A14);
    }
    float r = size * 0.5f, o = std::max(2.5f, size * 0.07f);
    switch (color) {
    case 0: // circle
        circle(cx, cy, r + o, ink);
        circle(cx, cy, r, c);
        break;
    case 1: // triangle
        ngon(cx, cy + r * 0.12f, r * 1.22f + o * 1.6f, 3, 0, ink);
        ngon(cx, cy + r * 0.12f, r * 1.22f, 3, 0, c);
        break;
    case 2: // square
        rrect(cx - r * 0.9f - o, cy - r * 0.9f - o, r * 1.8f + 2 * o, r * 1.8f + 2 * o, r * 0.2f + o, ink);
        rrect(cx - r * 0.9f, cy - r * 0.9f, r * 1.8f, r * 1.8f, r * 0.2f, c);
        break;
    case 3: // diamond
        ngon(cx, cy, r * 1.15f + o * 1.3f, 4, 0, ink);
        ngon(cx, cy, r * 1.15f, 4, 0, c);
        break;
    default: // wild: a star
        star(cx, cy, r * 1.25f + o * 1.5f, r * 0.62f + o, 5, 0, ink);
        star(cx, cy, r * 1.25f, r * 0.62f, 5, 0, c);
        break;
    }
    float ts = size * 0.74f;
    float ty = cy + ts * 0.36f + (color == 1 ? r * 0.22f : 0);
    std::string label = std::to_string(value);
    text(Font::Display, ts, cx + ts * 0.035f, ty + ts * 0.045f, label, ink.with_alpha(0.55f), Align::Center);
    text(Font::Display, ts, cx, ty, label, dark ? rgb(0x8D879B) : kWhite, Align::Center);
    if (bonus > 0) { // a stamp: a gold seal at the corner
        float sx = cx + r * 0.85f, sy = cy - r * 0.85f, sr = size * 0.24f;
        star(sx, sy, sr + o * 0.6f, sr * 0.72f + o * 0.5f, 10, 0, ink);
        star(sx, sy, sr, sr * 0.72f, 10, 0, dark ? kGoldDim : kGold);
        text(Font::Text, sr * 1.15f, sx, sy + sr * 0.4f, "+" + std::to_string(bonus), kInk, Align::Center);
    }
}

void tarp_icon(float cx, float cy, float size, bool dark) {
    float r = size * 0.5f, o = std::max(2.5f, size * 0.07f);
    Color c = dark ? mix(kTarp, rgb(0x2A2638), 0.72f) : kTarp;
    // A folded tarpaulin: a rounded sheet with a fold line and eyelets.
    rrect(cx - r - o, cy - r * 0.72f - o, 2 * r + 2 * o, r * 1.44f + 2 * o, r * 0.3f + o, kInk);
    rrect(cx - r, cy - r * 0.72f, 2 * r, r * 1.44f, r * 0.3f, c);
    quad({cx - r, cy - r * 0.1f}, {cx + r, cy - r * 0.45f}, {cx + r, cy - r * 0.25f}, {cx - r, cy + r * 0.1f}, darken(c, 0.22f));
    for (float sx : {-0.7f, 0.7f})
        for (float sy : {-0.45f, 0.45f}) {
            circle(cx + sx * r, cy + sy * r, size * 0.07f, kInk);
            circle(cx + sx * r, cy + sy * r, size * 0.035f, dark ? rgb(0x5A566A) : kCream);
        }
}

void crate_icon(float cx, float cy, float size, bool dark) {
    float r = size * 0.5f, o = std::max(2.5f, size * 0.07f);
    Color c = dark ? mix(kCrate, rgb(0x2A2638), 0.72f) : kCrate;
    Color d = darken(c, 0.28f);
    rrect(cx - r - o, cy - r - o, 2 * r + 2 * o, 2 * r + 2 * o, o * 1.6f, kInk);
    rrect(cx - r, cy - r, 2 * r, 2 * r, o * 0.8f, c);
    float b = size * 0.13f;
    rect(cx - r, cy - r, 2 * r, b, d);
    rect(cx - r, cy + r - b, 2 * r, b, d);
    rect(cx - r, cy - r, b, 2 * r, d);
    rect(cx + r - b, cy - r, b, 2 * r, d);
    line({cx - r + b, cy + r - b}, {cx + r - b, cy - r + b}, b * 0.9f, d);
    text(Font::Display, size * 0.36f, cx, cy + size * 0.13f, "\xC3\x97" "1.5", dark ? rgb(0x8D879B) : kWhite, Align::Center);
}

void wedge_token(float cx, float cy, float size, const spin::Wedge& w, int shown_value, bool dark) {
    switch (w.kind) {
    case spin::WedgeKind::Cargo: cargo(cx, cy, size, w.color, shown_value >= 0 ? shown_value : w.value, w.bonus, dark); break;
    case spin::WedgeKind::Tarp: tarp_icon(cx, cy, size, dark); break;
    case spin::WedgeKind::Crate: crate_icon(cx, cy, size * 0.9f, dark); break;
    default:
        circle(cx, cy, size * 0.2f, rgb(0x000000, 0.25f));
        break;
    }
}

void bulb(float cx, float cy, float r, bool lit, Color color) {
    if (lit) {
        circle(cx, cy, r * 2.3f, color.with_alpha(0.13f));
        circle(cx, cy, r * 1.6f, color.with_alpha(0.22f));
    }
    circle(cx, cy, r + 2, kInk);
    circle(cx, cy, r, lit ? color : mix(color, kInk, 0.72f));
    if (lit) circle(cx - r * 0.25f, cy - r * 0.25f, r * 0.42f, lighten(color, 0.7f));
}

void bulb_row(float x0, float y0, float x1, float y1, int count, int lit, float r, int chase) {
    for (int i = 0; i < count; ++i) {
        float t = count == 1 ? 0.5f : static_cast<float>(i) / (count - 1);
        bool on = chase >= 0 ? (i + chase) % 3 == 0 : i < lit;
        bulb(x0 + (x1 - x0) * t, y0 + (y1 - y0) * t, r, on);
    }
}

void ticket(float x, float y, float w, float h, Color fill, Color edge) {
    rrect(x - 4, y - 4, w + 8, h + 8, 12, edge);
    rrect(x, y, w, h, 8, fill);
    // Punched notches on the short sides.
    float n = std::min(h * 0.13f, 13.0f);
    circle(x - 4, y + h * 0.5f, n, edge);
    circle(x + w + 4, y + h * 0.5f, n, edge);
}

void stripes(float x, float y, float w, float h, float stripe_w, Color a, Color b) {
    int n = static_cast<int>(std::ceil(w / stripe_w));
    for (int i = 0; i < n; ++i) {
        float sx = x + i * stripe_w;
        rect(sx, y, std::min(stripe_w, x + w - sx), h, i % 2 == 0 ? a : b);
    }
}

void sign(float size, float x, float y, std::string_view s, Color color, Align align) {
    text_outlined(Font::Display, size, x, y, s, color, kInk, std::max(2.5f, size * 0.075f), align);
}

std::string number(long long v) {
    std::string digits = std::to_string(v < 0 ? -v : v), out;
    for (std::size_t i = 0; i < digits.size(); ++i) {
        if (i > 0 && (digits.size() - i) % 3 == 0) out += ',';
        out += digits[i];
    }
    return v < 0 ? "-" + out : out;
}

void money(float size, float x, float y, int dollars, Align align) {
    sign(size, x, y, "$" + number(dollars), kGold, align);
}

std::string wedge_name(const spin::Wedge& w) {
    switch (w.kind) {
    case spin::WedgeKind::Tarp: return "Tarp";
    case spin::WedgeKind::Crate: return "Crate";
    case spin::WedgeKind::Blank: return "Empty wedge";
    default: break;
    }
    std::string s = w.color == spin::kWild ? "Wild" : spin::kColorNames[w.color & 3];
    s += " " + std::to_string(w.value);
    if (w.bonus > 0) s += " (+" + std::to_string(w.bonus) + " points)";
    return s;
}

} // namespace ui
