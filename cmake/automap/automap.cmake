# Written by automap from automap.manifest.json. Edit the manifest, not this file.
# The developer's CMakeLists.txt includes this file once.

set(AUTOMAP_ROOT ${CMAKE_CURRENT_LIST_DIR}/../..)
set(AUTOMAP_CXX_STANDARD 20)

add_library(automap_glue OBJECT
    ${AUTOMAP_ROOT}/glue/automap/alloc.cpp
    ${AUTOMAP_ROOT}/glue/automap/assets.cpp
    ${AUTOMAP_ROOT}/glue/automap/diag.cpp
    ${AUTOMAP_ROOT}/glue/automap/lifecycle.cpp
    ${AUTOMAP_ROOT}/glue/automap/probe.cpp
    ${AUTOMAP_ROOT}/glue/automap/startup.cpp
    ${AUTOMAP_ROOT}/glue/dear-imgui--sdl3/imgui_sdl3_gpu.cpp
    ${AUTOMAP_ROOT}/glue/dear-imgui--sdl3/imgui_sdl3_gpu_probe.cpp
    ${AUTOMAP_ROOT}/glue/dear-imgui/imgui_allocation.cpp
    ${AUTOMAP_ROOT}/glue/dear-imgui/imgui_allocation_probe.cpp
    ${AUTOMAP_ROOT}/glue/dear-imgui/imgui_build_flags_probe.cpp
    ${AUTOMAP_ROOT}/glue/dear-imgui/imgui_math_glm_probe.cpp
    ${AUTOMAP_ROOT}/glue/dear-imgui/imgui_setup.cpp
    ${AUTOMAP_ROOT}/glue/miniaudio/miniaudio_allocation.cpp
    ${AUTOMAP_ROOT}/glue/miniaudio/miniaudio_allocation_probe.cpp
    ${AUTOMAP_ROOT}/glue/miniaudio/miniaudio_diagnostics.cpp
    ${AUTOMAP_ROOT}/glue/miniaudio/miniaudio_diagnostics_probe.cpp
    ${AUTOMAP_ROOT}/glue/miniaudio/miniaudio_setup.cpp
    ${AUTOMAP_ROOT}/glue/sdl-shadercross--sdl3/shadercross_sdl3.cpp
    ${AUTOMAP_ROOT}/glue/sdl-shadercross--sdl3/shadercross_sdl3_probe.cpp
    ${AUTOMAP_ROOT}/glue/sdl-shadercross/shadercross_assets.cpp
    ${AUTOMAP_ROOT}/glue/sdl-shadercross/shadercross_assets_probe.cpp
    ${AUTOMAP_ROOT}/glue/sdl-shadercross/shadercross_setup.cpp
    ${AUTOMAP_ROOT}/glue/sdl3/sdl3_allocation.cpp
    ${AUTOMAP_ROOT}/glue/sdl3/sdl3_allocation_probe.cpp
    ${AUTOMAP_ROOT}/glue/sdl3/sdl3_diagnostics.cpp
    ${AUTOMAP_ROOT}/glue/sdl3/sdl3_diagnostics_probe.cpp
    ${AUTOMAP_ROOT}/glue/sdl3/sdl3_setup.cpp
    ${AUTOMAP_ROOT}/glue/spdlog/spdlog_diagnostics.cpp
    ${AUTOMAP_ROOT}/glue/spdlog/spdlog_diagnostics_probe.cpp
    ${AUTOMAP_ROOT}/glue/stb-image/stb_image_allocation.cpp
    ${AUTOMAP_ROOT}/glue/stb-image/stb_image_allocation_probe.cpp
    ${AUTOMAP_ROOT}/glue/stb-image/stb_image_assets.cpp
    ${AUTOMAP_ROOT}/glue/stb-image/stb_image_assets_probe.cpp
    ${AUTOMAP_ROOT}/glue/tracy/tracy_diagnostics.cpp
    ${AUTOMAP_ROOT}/glue/tracy/tracy_diagnostics_probe.cpp
)
target_include_directories(automap_glue PUBLIC ${AUTOMAP_ROOT}/glue)
target_compile_features(automap_glue PUBLIC cxx_std_${AUTOMAP_CXX_STANDARD})
if(AUTOMAP_PROBE)
    target_compile_definitions(automap_glue PUBLIC AUTOMAP_PROBE)
endif()
target_compile_definitions(automap_glue PUBLIC AUTOMAP_LIBRARY_DEAR_IMGUI=1 AUTOMAP_LIBRARY_GLM=1 AUTOMAP_LIBRARY_MINIAUDIO=1 AUTOMAP_LIBRARY_SDL_SHADERCROSS=1 AUTOMAP_LIBRARY_SDL3=1 AUTOMAP_LIBRARY_SPDLOG=1 AUTOMAP_LIBRARY_STB_IMAGE=1 AUTOMAP_LIBRARY_TRACY=1)
target_compile_definitions(automap_glue PUBLIC AUTOMAP_VARIANT_DEAR_IMGUI="docking" AUTOMAP_VARIANT_GLM="default" AUTOMAP_VARIANT_MINIAUDIO="default" AUTOMAP_VARIANT_SDL_SHADERCROSS="default" AUTOMAP_VARIANT_SDL3="default" AUTOMAP_VARIANT_SPDLOG="default" AUTOMAP_VARIANT_STB_IMAGE="default" AUTOMAP_VARIANT_TRACY="default")

include(${CMAKE_CURRENT_LIST_DIR}/dear-imgui.cmake)
include(${CMAKE_CURRENT_LIST_DIR}/glm.cmake)
include(${CMAKE_CURRENT_LIST_DIR}/miniaudio.cmake)
include(${CMAKE_CURRENT_LIST_DIR}/sdl-shadercross.cmake)
include(${CMAKE_CURRENT_LIST_DIR}/sdl3.cmake)
include(${CMAKE_CURRENT_LIST_DIR}/spdlog.cmake)
include(${CMAKE_CURRENT_LIST_DIR}/stb-image.cmake)
include(${CMAKE_CURRENT_LIST_DIR}/tracy.cmake)
