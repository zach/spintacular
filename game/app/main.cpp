// Spintacular's executable. It starts the Automap composition, runs the game
// until the window closes, and shuts down.
//
//   --frames <n>          stop after n frames (automap verify's smoke test)
//   --probe <file>        also run every Automap probe and write the results
//   --script <name>       play a scripted demo instead of taking input
//   --shots <dir>         with --script: save screenshots into dir
//   --seed <n>            the seed for a scripted run
//   --size <w>x<h>        window size
//   --profile-dir <dir>   where saves and the profile live (default: the user's folder)
//   --mute                no sound
#include <cstdlib>
#include <cstring>
#include <string>

#include <SDL3/SDL.h>

#include "app.h"
#include "automap/diag.h"
#include "automap/probe.h"
#include "automap/startup.h"

int main(int argc, char** argv) {
    int frames = -1;
    const char* results = nullptr;
    spin::AppOptions options;
    for (int i = 1; i < argc; ++i) {
        bool more = i + 1 < argc;
        if (!std::strcmp(argv[i], "--frames") && more) frames = std::atoi(argv[++i]);
        else if (!std::strcmp(argv[i], "--probe") && more) results = argv[++i];
        else if (!std::strcmp(argv[i], "--script") && more) options.script = argv[++i];
        else if (!std::strcmp(argv[i], "--shots") && more) options.shots_dir = argv[++i];
        else if (!std::strcmp(argv[i], "--seed") && more) options.seed = std::strtoull(argv[++i], nullptr, 10);
        else if (!std::strcmp(argv[i], "--profile-dir") && more) options.profile_dir = argv[++i];
        else if (!std::strcmp(argv[i], "--size") && more) std::sscanf(argv[++i], "%dx%d", &options.width, &options.height);
        else if (!std::strcmp(argv[i], "--mute")) options.mute = true;
    }
    // A fixed frame count means an unattended run: fixed time steps, nothing saved to the user's folder.
    options.headless = frames >= 0 || !options.script.empty();

    if (results != nullptr) am::probe::begin(results);
    am::Context ctx;
    if (!am::startup(ctx)) {
        am::probe::write_results();
        return 1;
    }
    int status = 0;
    {
        spin::App app;
        if (!app.init(ctx, options)) {
            am::diag::log(am::diag::Level::error, "spintacular", "the game couldn't start");
            status = 1;
        } else {
            for (int frame = 0; frames < 0 || frame < frames; ++frame)
                if (!app.frame()) break;
        }
        if (results != nullptr) am::probe::run(am::probe::When::running, ctx);
        app.shutdown();
    }
    am::shutdown(ctx);
    if (results != nullptr) {
        am::probe::run(am::probe::When::after_shutdown, ctx);
        am::probe::write_results();
    }
    return status;
}
