// Probes for the dear-imgui--sdl3 pairing.
// SPDX-License-Identifier: MIT-0
#ifdef AUTOMAP_PROBE
#include <SDL3/SDL.h>
#include <imgui.h>

#include "automap/probe.h"
#include "imgui_sdl3_gpu.h"

namespace {

// dear-imgui--sdl3.events-forwarded: a mouse motion SDL reports reaches
// ImGui within one frame.
void am_probe_imgui_sdl3_events(am::Context& ctx) {
    SDL_Event motion = {};
    motion.type = SDL_EVENT_MOUSE_MOTION;
    motion.motion.windowID = SDL_GetWindowID(ctx.sdl3.window);
    motion.motion.x = 12.0f;
    motion.motion.y = 34.0f;
    SDL_PushEvent(&motion);
    am_pair_imgui_sdl3_begin_frame(ctx);
    ImVec2 seen = ImGui::GetIO().MousePos;
    am_pair_imgui_sdl3_end_frame(ctx);
    bool forwarded = seen.x == 12.0f && seen.y == 34.0f;
    am::probe::check("dear-imgui--sdl3.events-forwarded", "", forwarded, "EVENTS_NOT_FORWARDED(dear-imgui--sdl3)");
}
const am::probe::Registration registration("dear-imgui--sdl3", &am_probe_imgui_sdl3_events);

} // namespace
#endif
