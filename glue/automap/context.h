// Written by automap from the composition's recipes. Edit the manifest, not this file.
#pragma once

struct SDL_GPUDevice;
struct SDL_Window;
struct ma_engine;

namespace am {

// The context every entry point takes: the fields recipes provide and read.
struct Context {
    struct {
        ::SDL_Window* window = nullptr;
        ::SDL_GPUDevice* gpu_device = nullptr;
    } sdl3;
    struct {
        ::ma_engine* engine = nullptr;
    } miniaudio;
};

} // namespace am
