// The game's 2D renderer on SDL_GPU: flat shapes and text, batched into one
// draw call per frame. Everything is drawn in a virtual 1920x1080 canvas that
// is scaled to fit the window.
#pragma once

#include <cstdint>
#include <string>
#include <string_view>

#include <glm/vec2.hpp>

struct SDL_GPUDevice;
struct SDL_Window;
struct SDL_GPUCommandBuffer;
struct SDL_GPUTexture;

namespace gfx {

constexpr float kWidth = 1920.0f, kHeight = 1080.0f;
constexpr float kPi = 3.14159265358979f;

struct Color {
    float r = 1, g = 1, b = 1, a = 1;
    Color with_alpha(float alpha) const { return {r, g, b, a * alpha}; }
};
constexpr Color rgb(std::uint32_t hex, float a = 1.0f) {
    return {((hex >> 16) & 0xff) / 255.0f, ((hex >> 8) & 0xff) / 255.0f, (hex & 0xff) / 255.0f, a};
}
Color mix(Color a, Color b, float t);
Color darken(Color c, float amount);  // 0..1
Color lighten(Color c, float amount); // 0..1

using Vec2 = glm::vec2;

enum class Font { Display, Text };
enum class Align { Left, Center, Right };

bool init(SDL_GPUDevice* device, SDL_Window* window, const std::string& asset_dir);
void shutdown();

// A frame: begin, draw, then end. end() renders the batch into the scene
// texture; present() puts it on the window (and Dear ImGui's draw data over it).
void begin_frame();
void end_frame(SDL_GPUCommandBuffer* commands);
// The scene texture end_frame() rendered into, and its size in pixels.
SDL_GPUTexture* scene_texture();
void scene_size(int* w, int* h);
// Saves the last frame's scene as a PNG. Returns false if it couldn't.
bool save_screenshot(const std::string& path);

// Window pixels -> canvas coordinates.
Vec2 to_canvas(float window_x, float window_y);

// ---- transforms: translate, then rotate (radians) and scale about that point
void push(float tx, float ty, float rotation = 0, float scale = 1);
void pop();
// Multiplies the alpha of everything drawn until the matching pop_alpha().
void push_alpha(float alpha);
void pop_alpha();

// ---- shapes
void tri(Vec2 a, Vec2 b, Vec2 c, Color color);
void quad(Vec2 a, Vec2 b, Vec2 c, Vec2 d, Color color);
void rect(float x, float y, float w, float h, Color color);
void rrect(float x, float y, float w, float h, float radius, Color color);
// A rounded rectangle with an outline drawn outside it.
void panel(float x, float y, float w, float h, float radius, Color fill, Color outline, float outline_width);
void circle(float cx, float cy, float r, Color color);
void ring(float cx, float cy, float r_inner, float r_outer, Color color);
// A slice of a ring between two angles (radians, clockwise from 12 o'clock).
void sector(float cx, float cy, float r_inner, float r_outer, float a0, float a1, Color color);
void line(Vec2 a, Vec2 b, float thickness, Color color);
void ngon(float cx, float cy, float r, int sides, float rotation, Color color);
void star(float cx, float cy, float r_outer, float r_inner, int points, float rotation, Color color);
void gradient_rect(float x, float y, float w, float h, Color top, Color bottom);

// ---- text. Size is the cap-to-descender line height in canvas pixels.
float text_width(Font font, float size, std::string_view s);
// (x, y) is the left/center/right end of the baseline.
void text(Font font, float size, float x, float y, std::string_view s, Color color, Align align = Align::Left);
// Text with a dark outline behind it, for signs.
void text_outlined(Font font, float size, float x, float y, std::string_view s, Color color, Color outline, float width,
                   Align align = Align::Left);
// Wraps at max_width; returns the height used. line_height in multiples of size.
float text_wrapped(Font font, float size, float x, float y, float max_width, std::string_view s, Color color,
                   Align align = Align::Left, float line_height = 1.25f, bool measure_only = false);

} // namespace gfx
