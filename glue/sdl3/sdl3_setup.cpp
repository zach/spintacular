// SDL3's setup glue.
// SPDX-License-Identifier: MIT-0
#include "sdl3_setup.h"

#include <SDL3/SDL.h>

#include "automap/lifecycle.h"

bool am_sdl3_setup_init(am::Context& ctx) {
    if (!SDL_Init(SDL_INIT_VIDEO)) {
        SDL_Log("SDL_Init failed: %s", SDL_GetError());
        return false;
    }
    am::lifecycle::init_step("sdl3:SDL_Init");
    ctx.sdl3.window = SDL_CreateWindow("Automap testbed", 1280, 720, SDL_WINDOW_RESIZABLE);
    if (ctx.sdl3.window == nullptr) {
        SDL_Log("SDL_CreateWindow failed: %s", SDL_GetError());
        return false;
    }
    am::lifecycle::init_step("sdl3:SDL_CreateWindow");
    ctx.sdl3.gpu_device = SDL_CreateGPUDevice(
        SDL_GPU_SHADERFORMAT_SPIRV | SDL_GPU_SHADERFORMAT_DXIL | SDL_GPU_SHADERFORMAT_MSL | SDL_GPU_SHADERFORMAT_METALLIB,
        false, nullptr);
    if (ctx.sdl3.gpu_device == nullptr) {
        SDL_Log("SDL_CreateGPUDevice failed: %s", SDL_GetError());
        return false;
    }
    am::lifecycle::init_step("sdl3:SDL_CreateGPUDevice");
    if (!SDL_ClaimWindowForGPUDevice(ctx.sdl3.gpu_device, ctx.sdl3.window)) {
        SDL_Log("SDL_ClaimWindowForGPUDevice failed: %s", SDL_GetError());
        return false;
    }
    am::lifecycle::init_step("sdl3:SDL_ClaimWindowForGPUDevice");
    return true;
}

void am_sdl3_setup_shutdown(am::Context& ctx) {
    if (ctx.sdl3.gpu_device != nullptr) {
        SDL_WaitForGPUIdle(ctx.sdl3.gpu_device);
        am::lifecycle::shutdown_step("sdl3:SDL_WaitForGPUIdle");
        SDL_ReleaseWindowFromGPUDevice(ctx.sdl3.gpu_device, ctx.sdl3.window);
        am::lifecycle::shutdown_step("sdl3:SDL_ReleaseWindowFromGPUDevice");
        SDL_DestroyGPUDevice(ctx.sdl3.gpu_device);
        ctx.sdl3.gpu_device = nullptr;
        am::lifecycle::shutdown_step("sdl3:SDL_DestroyGPUDevice");
    }
    SDL_DestroyWindow(ctx.sdl3.window);
    ctx.sdl3.window = nullptr;
    am::lifecycle::shutdown_step("sdl3:SDL_DestroyWindow");
    SDL_Quit();
    am::lifecycle::shutdown_step("sdl3:SDL_Quit");
}
