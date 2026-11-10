// SDL_shadercross's binding to the assets contract: shaders cooked from
// SPIR-V files the assets interface reads, for a build step that prepares
// each backend's shaders ahead of time.
// SPDX-License-Identifier: MIT-0
#pragma once
#include <cstdint>
#include <vector>

#include <SDL3/SDL.h>
#include <SDL3_shadercross/SDL_shadercross.h>

// Reads the SPIR-V file at path through the assets interface and compiles
// it to format: SPIR-V is copied, DXBC is compiled (on Windows, through
// d3dcompiler_47.dll), and MSL is transpiled. Returns false, with SDL's
// error set, when the file can't be read or compiled.
bool am_shadercross_cook(const char* path, SDL_GPUShaderFormat format, SDL_ShaderCross_ShaderStage stage, const char* entrypoint,
                         std::vector<std::uint8_t>& cooked);
