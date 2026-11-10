// Dear ImGui's setup glue.
// SPDX-License-Identifier: MIT-0
#include "imgui_setup.h"

#include <imgui.h>

#include "automap/lifecycle.h"

bool am_imgui_setup_init(am::Context&) {
    if (ImGui::CreateContext() == nullptr) return false;
    am::lifecycle::init_step("dear-imgui:CreateContext");
    ImGuiIO& io = ImGui::GetIO();
    io.ConfigFlags |= ImGuiConfigFlags_NavEnableKeyboard | ImGuiConfigFlags_DockingEnable;
#ifdef AUTOMAP_PROBE
    io.IniFilename = nullptr; // probe runs must not depend on each other
#endif
    return true;
}

void am_imgui_setup_shutdown(am::Context&) {
    ImGui::DestroyContext();
    am::lifecycle::shutdown_step("dear-imgui:DestroyContext");
}
