// The sdl-shadercross--sdl3 pairing.
// SPDX-License-Identifier: MIT-0
#include "shadercross_sdl3.h"

#include <cstring>

SDL_GPUShaderFormat am_shadercross_format_for(const char* driver) {
    if (driver == nullptr) return SDL_GPU_SHADERFORMAT_INVALID;
    if (std::strcmp(driver, "vulkan") == 0) return SDL_GPU_SHADERFORMAT_SPIRV;
    if (std::strcmp(driver, "direct3d12") == 0) return SDL_GPU_SHADERFORMAT_DXBC;
    if (std::strcmp(driver, "metal") == 0) return SDL_GPU_SHADERFORMAT_MSL;
    return SDL_GPU_SHADERFORMAT_INVALID;
}

SDL_GPUShader* am_shadercross_create_shader(SDL_GPUDevice* device, const Uint8* spirv, size_t size, const char* entrypoint,
                                            SDL_ShaderCross_ShaderStage stage, SDL_GPUShaderCreateInfo* info) {
    SDL_ShaderCross_GraphicsShaderMetadata* metadata = SDL_ShaderCross_ReflectGraphicsSPIRV(spirv, size, 0);
    if (metadata == nullptr) return nullptr;
    SDL_ShaderCross_SPIRV_Info source = {spirv, size, entrypoint, stage, 0};

    SDL_GPUShaderCreateInfo create = {};
    create.format = am_shadercross_format_for(SDL_GetGPUDeviceDriver(device));
    create.stage = static_cast<SDL_GPUShaderStage>(stage);
    create.entrypoint = entrypoint;
    create.num_samplers = metadata->resource_info.num_samplers;
    create.num_storage_textures = metadata->resource_info.num_storage_textures;
    create.num_storage_buffers = metadata->resource_info.num_storage_buffers;
    create.num_uniform_buffers = metadata->resource_info.num_uniform_buffers;
    SDL_free(metadata);

    void* code = nullptr;
    switch (create.format) {
    case SDL_GPU_SHADERFORMAT_SPIRV:
        code = SDL_malloc(size);
        if (code != nullptr) SDL_memcpy(code, spirv, size);
        create.code_size = size;
        break;
    case SDL_GPU_SHADERFORMAT_DXBC:
        code = SDL_ShaderCross_CompileDXBCFromSPIRV(&source, &create.code_size);
        break;
    case SDL_GPU_SHADERFORMAT_MSL:
        code = SDL_ShaderCross_TranspileMSLFromSPIRV(&source);
        create.code_size = code != nullptr ? SDL_strlen(static_cast<const char*>(code)) : 0;
        // Metal reserves "main": SPIRV-Cross renames it main0.
        if (std::strcmp(entrypoint, "main") == 0) create.entrypoint = "main0";
        break;
    default:
        SDL_SetError("no shader format for SDL_GPU driver %s", SDL_GetGPUDeviceDriver(device));
        return nullptr;
    }
    if (code == nullptr) return nullptr;
    create.code = static_cast<const Uint8*>(code);
    SDL_GPUShader* shader = SDL_CreateGPUShader(device, &create);
    SDL_free(code);
    if (info != nullptr) {
        *info = create;
        info->code = nullptr;
    }
    return shader;
}
