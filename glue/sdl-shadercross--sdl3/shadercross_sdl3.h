// The sdl-shadercross--sdl3 pairing: shaders compiled from SPIR-V for the
// composition's SDL_GPU device, in the format its backend takes, with the
// resource counts SDL_GPUShaderCreateInfo needs read from the shader by
// reflection.
// SPDX-License-Identifier: MIT-0
#pragma once
#include <SDL3/SDL.h>
#include <SDL3_shadercross/SDL_shadercross.h>

// The shader format glue compiles to for an SDL_GPU driver, as
// SDL_GetGPUDeviceDriver names it: SPIR-V for Vulkan, DXBC for Direct3D 12
// (through the system's d3dcompiler_47.dll, so no DXC is needed at
// runtime), MSL for Metal. SDL_GPU_SHADERFORMAT_INVALID for any other.
SDL_GPUShaderFormat am_shadercross_format_for(const char* driver);

// Compiles SPIR-V for the device and creates the shader. When info isn't
// null it is filled with what the shader was created from. Returns null,
// with SDL's error set, when the shader can't be compiled or created.
SDL_GPUShader* am_shadercross_create_shader(SDL_GPUDevice* device, const Uint8* spirv, size_t size, const char* entrypoint,
                                            SDL_ShaderCross_ShaderStage stage, SDL_GPUShaderCreateInfo* info = nullptr);
