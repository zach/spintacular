// SDL_shadercross's setup glue.
// SPDX-License-Identifier: MIT-0
#pragma once
#include "automap/context.h"

// Initializes SDL_shadercross. Runs in the systems phase.
bool am_shadercross_setup_init(am::Context& ctx);

// Shuts SDL_shadercross down. Runs in the systems phase at shutdown.
void am_shadercross_setup_shutdown(am::Context& ctx);
