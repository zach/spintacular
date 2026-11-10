// SDL3's setup glue.
// SPDX-License-Identifier: MIT-0
#pragma once
#include "automap/context.h"

// Initializes SDL's video subsystem, creates one window and a GPU device for
// the platform's backend, and claims the window for the device. Provides
// sdl3.window and sdl3.gpu_device. Runs in the platform phase.
bool am_sdl3_setup_init(am::Context& ctx);

// Waits for the GPU to go idle, releases the window from the device, and
// destroys both before SDL_Quit.
void am_sdl3_setup_shutdown(am::Context& ctx);
