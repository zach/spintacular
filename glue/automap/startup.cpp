// Written by automap from the composition's recipes. Edit the manifest, not this file.
#include "automap/startup.h"

#include <cstdio>

bool am_bind_imgui_allocation(am::Context& ctx);
bool am_bind_miniaudio_allocation(am::Context& ctx);
bool am_bind_sdl3_allocation(am::Context& ctx);
bool am_bind_miniaudio_diagnostics(am::Context& ctx);
bool am_bind_sdl3_diagnostics(am::Context& ctx);
bool am_spdlog_setup_init(am::Context& ctx);
bool am_sdl3_setup_init(am::Context& ctx);
bool am_imgui_setup_init(am::Context& ctx);
bool am_miniaudio_setup_init(am::Context& ctx);
bool am_shadercross_setup_init(am::Context& ctx);
bool am_pair_imgui_sdl3_init(am::Context& ctx);
void am_pair_imgui_sdl3_shutdown(am::Context& ctx);
void am_shadercross_setup_shutdown(am::Context& ctx);
void am_miniaudio_setup_shutdown(am::Context& ctx);
void am_imgui_setup_shutdown(am::Context& ctx);
void am_sdl3_setup_shutdown(am::Context& ctx);
void am_spdlog_setup_shutdown(am::Context& ctx);
void am_unbind_miniaudio_diagnostics(am::Context& ctx);

namespace am {

namespace {
bool init_failed(const char* call) {
    std::fprintf(stderr, "INIT_FAILED(%s)\n", call);
    return false;
}
} // namespace

bool startup(Context& ctx) {
    // memory
    if (!am_bind_imgui_allocation(ctx)) return init_failed("am_bind_imgui_allocation");
    if (!am_bind_miniaudio_allocation(ctx)) return init_failed("am_bind_miniaudio_allocation");
    if (!am_bind_sdl3_allocation(ctx)) return init_failed("am_bind_sdl3_allocation");
    // diagnostics
    if (!am_bind_miniaudio_diagnostics(ctx)) return init_failed("am_bind_miniaudio_diagnostics");
    if (!am_bind_sdl3_diagnostics(ctx)) return init_failed("am_bind_sdl3_diagnostics");
    if (!am_spdlog_setup_init(ctx)) return init_failed("am_spdlog_setup_init");
    // platform
    if (!am_sdl3_setup_init(ctx)) return init_failed("am_sdl3_setup_init");
    // systems
    if (!am_imgui_setup_init(ctx)) return init_failed("am_imgui_setup_init");
    if (!am_miniaudio_setup_init(ctx)) return init_failed("am_miniaudio_setup_init");
    if (!am_shadercross_setup_init(ctx)) return init_failed("am_shadercross_setup_init");
    if (!am_pair_imgui_sdl3_init(ctx)) return init_failed("am_pair_imgui_sdl3_init");
    return true;
}

void shutdown(Context& ctx) {
    // systems
    am_pair_imgui_sdl3_shutdown(ctx);
    am_shadercross_setup_shutdown(ctx);
    am_miniaudio_setup_shutdown(ctx);
    am_imgui_setup_shutdown(ctx);
    // platform
    am_sdl3_setup_shutdown(ctx);
    // diagnostics
    am_spdlog_setup_shutdown(ctx);
    am_unbind_miniaudio_diagnostics(ctx);
}

} // namespace am
