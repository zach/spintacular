// The testbed's executable, written by automap start: it starts the
// composition, runs a frame loop until the window is closed, and shuts down.
// --frames <n> stops after n frames; --probe <results-file> also runs every
// probe and writes the results.
// SPDX-License-Identifier: MIT-0
#include <cstdlib>
#include <cstring>

#include "automap/probe.h"
#include "automap/startup.h"

#if defined(AUTOMAP_LIBRARY_DEAR_IMGUI) && defined(AUTOMAP_LIBRARY_SDL3)
#include <SDL3/SDL.h>
#include <imgui.h>

#include "dear-imgui--sdl3/imgui_sdl3_gpu.h"
#define AUTOMAP_TESTBED_IMGUI 1
#elif defined(AUTOMAP_LIBRARY_SDL3)
#include <SDL3/SDL.h>
#define AUTOMAP_TESTBED_SDL3 1
#endif

#if defined(AUTOMAP_LIBRARY_JOLT)
#include <Jolt/Jolt.h>

#include <Jolt/Physics/PhysicsSystem.h>
#endif

int main(int argc, char** argv) {
    int frames = -1;
    const char* results = nullptr;
    for (int i = 1; i + 1 < argc; ++i) {
        if (std::strcmp(argv[i], "--frames") == 0) frames = std::atoi(argv[i + 1]);
        if (std::strcmp(argv[i], "--probe") == 0) results = argv[i + 1];
    }
#if !defined(AUTOMAP_TESTBED_IMGUI) && !defined(AUTOMAP_TESTBED_SDL3)
    if (frames < 0) frames = 120; // nothing to close: run a fixed number of frames
#endif
    if (results != nullptr) am::probe::begin(results);
    am::Context ctx;
    if (!am::startup(ctx)) {
        am::probe::write_results();
        return 1;
    }
    for (int frame = 0; frames < 0 || frame < frames; ++frame) {
#if defined(AUTOMAP_LIBRARY_JOLT)
        ctx.jolt.physics_system->Update(1.0f / 60.0f, 1, ctx.jolt.temp_allocator, ctx.jolt.job_system);
#endif
#if defined(AUTOMAP_TESTBED_IMGUI)
        if (!am_pair_imgui_sdl3_begin_frame(ctx)) break;
        ImGui::Begin("Automap testbed");
        ImGui::Text("Frame %d", frame);
        ImGui::Text("GPU driver: %s", SDL_GetGPUDeviceDriver(ctx.sdl3.gpu_device));
        ImGui::End();
        am_pair_imgui_sdl3_end_frame(ctx);
#elif defined(AUTOMAP_TESTBED_SDL3)
        bool open = true;
        SDL_Event event;
        while (SDL_PollEvent(&event)) {
            if (event.type == SDL_EVENT_QUIT || event.type == SDL_EVENT_WINDOW_CLOSE_REQUESTED) open = false;
        }
        if (!open) break;
        SDL_Delay(16);
#endif
    }
    if (results != nullptr) am::probe::run(am::probe::When::running, ctx);
    am::shutdown(ctx);
    if (results != nullptr) {
        am::probe::run(am::probe::When::after_shutdown, ctx);
        am::probe::write_results();
    }
    return 0;
}
