// The game: screens, input, animation and sound on top of the rules core.
#pragma once

#include <cstdint>
#include <string>

#include "automap/context.h"

namespace spin {

struct AppOptions {
    std::string script;      // a scripted demo to play instead of taking input
    std::string shots_dir;   // where a script saves screenshots
    std::string profile_dir; // where saves and the profile live; empty: the user's folder
    std::uint64_t seed = 0;
    int width = 0, height = 0;
    bool mute = false;
    bool headless = false; // fixed time steps; nothing written to the user's folder
};

struct Game;

class App {
public:
    bool init(am::Context& ctx, const AppOptions& options);
    bool frame(); // false when the game should close
    void shutdown();

private:
    Game* g = nullptr;
};

} // namespace spin
