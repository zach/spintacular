// The dear-imgui--sdl3 pairing: Dear ImGui's SDL3 platform backend and
// SDL_GPU renderer backend.
// SPDX-License-Identifier: MIT-0
#pragma once
#include "automap/context.h"

// Initializes both backends for sdl3.window and sdl3.gpu_device. Runs in the
// systems phase, after the ImGui context exists and SDL has claimed the
// window for the device.
bool am_pair_imgui_sdl3_init(am::Context& ctx);

// Waits for the GPU to go idle, then shuts both backends down, before the
// ImGui context and the GPU device are destroyed.
void am_pair_imgui_sdl3_shutdown(am::Context& ctx);

// Starts a frame: polls every SDL event, passes each to ImGui, and begins an
// ImGui frame. Returns false once the window is closed.
bool am_pair_imgui_sdl3_begin_frame(am::Context& ctx);

// Ends the frame: renders ImGui's draw data into the window's swapchain.
void am_pair_imgui_sdl3_end_frame(am::Context& ctx);
