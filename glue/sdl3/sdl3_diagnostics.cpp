// SDL3's binding to the diagnostics contract.
// SPDX-License-Identifier: MIT-0
#include <SDL3/SDL.h>

#include "automap/context.h"
#include "automap/diag.h"
#include "automap/lifecycle.h"

void SDLCALL am_sdl3_log(void*, int, SDL_LogPriority priority, const char* message) {
    am::diag::Level level = priority >= SDL_LOG_PRIORITY_ERROR ? am::diag::Level::error
                            : priority >= SDL_LOG_PRIORITY_WARN ? am::diag::Level::warn
                            : priority >= SDL_LOG_PRIORITY_INFO ? am::diag::Level::info
                                                                : am::diag::Level::trace;
    am::diag::log(level, "sdl3", message);
}

SDL_AssertState SDLCALL am_sdl3_assert(const SDL_AssertData* data, void*) {
    bool debug_break = am::diag::assert_failed("sdl3", data->condition, nullptr, data->filename, data->linenum);
    return debug_break ? SDL_ASSERTION_BREAK : SDL_ASSERTION_IGNORE;
}

// Routes SDL's log output and assertions through the diagnostics facade.
// Runs in the diagnostics phase.
bool am_bind_sdl3_diagnostics(am::Context&) {
    SDL_SetLogOutputFunction(am_sdl3_log, nullptr);
    SDL_SetAssertionHandler(am_sdl3_assert, nullptr);
    am::lifecycle::init_step("sdl3:SDL_SetLogOutputFunction");
    return true;
}
