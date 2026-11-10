// SDL_shadercross's binding to the assets contract.
// SPDX-License-Identifier: MIT-0
#include "shadercross_assets.h"

#include <cstring>

#include "automap/assets.h"

bool am_shadercross_cook(const char* path, SDL_GPUShaderFormat format, SDL_ShaderCross_ShaderStage stage, const char* entrypoint,
                         std::vector<std::uint8_t>& cooked) {
    cooked.clear();
    std::vector<std::byte> spirv;
    if (!am::assets::read("sdl-shadercross", path, spirv)) {
        SDL_SetError("can't read %s", path);
        return false;
    }
    if (format == SDL_GPU_SHADERFORMAT_SPIRV) {
        const auto* bytes = reinterpret_cast<const std::uint8_t*>(spirv.data());
        cooked.assign(bytes, bytes + spirv.size());
        return true;
    }
    SDL_ShaderCross_SPIRV_Info source = {reinterpret_cast<const Uint8*>(spirv.data()), spirv.size(), entrypoint, stage, 0};
    void* code = nullptr;
    size_t size = 0;
    if (format == SDL_GPU_SHADERFORMAT_DXBC) {
        code = SDL_ShaderCross_CompileDXBCFromSPIRV(&source, &size);
    } else if (format == SDL_GPU_SHADERFORMAT_MSL) {
        code = SDL_ShaderCross_TranspileMSLFromSPIRV(&source);
        size = code != nullptr ? std::strlen(static_cast<const char*>(code)) : 0;
    } else {
        SDL_SetError("SDL_shadercross can't cook SPIR-V to shader format %u", unsigned(format));
        return false;
    }
    if (code == nullptr) return false;
    cooked.assign(static_cast<const std::uint8_t*>(code), static_cast<const std::uint8_t*>(code) + size);
    SDL_free(code);
    return true;
}
