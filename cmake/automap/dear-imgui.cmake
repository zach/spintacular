# Dear ImGui's build fragment. The imgui port builds the SDL3 platform and
# SDL_GPU renderer backends from the same commit, as part of imgui::imgui.
# SPDX-License-Identifier: MIT-0
find_package(imgui CONFIG REQUIRED)

target_link_libraries(automap_glue PUBLIC imgui::imgui)
