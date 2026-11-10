// SDL_shadercross's setup glue.
// SPDX-License-Identifier: MIT-0
#include "shadercross_setup.h"

#include <SDL3_shadercross/SDL_shadercross.h>

#include "automap/lifecycle.h"

bool am_shadercross_setup_init(am::Context&) {
    if (!SDL_ShaderCross_Init()) return false;
    am::lifecycle::init_step("sdl-shadercross:SDL_ShaderCross_Init");
    return true;
}

void am_shadercross_setup_shutdown(am::Context&) {
    SDL_ShaderCross_Quit();
    am::lifecycle::shutdown_step("sdl-shadercross:SDL_ShaderCross_Quit");
}
