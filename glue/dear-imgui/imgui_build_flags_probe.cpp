// Dear ImGui's probe for the build-flags contract.
// SPDX-License-Identifier: MIT-0
#ifdef AUTOMAP_PROBE
#include <imgui.h>

#include "automap/probe.h"

namespace {

// build-flags.abi-defines-match: the glue's view of ImGui's version and
// data layout (ImDrawIdx, ImWchar and the structs they change) matches the
// library's. ImGui asserts on a mismatch, so the failure is recorded first.
void am_probe_imgui_abi(am::Context&) {
    am::probe::expect_failure("build-flags.abi-defines-match", "", "ABI_DEFINE_MISMATCH(dear-imgui)");
    bool match = ImGui::DebugCheckVersionAndDataLayout(IMGUI_VERSION, sizeof(ImGuiIO), sizeof(ImGuiStyle), sizeof(ImVec2),
                                                       sizeof(ImVec4), sizeof(ImDrawVert), sizeof(ImDrawIdx));
    am::probe::check("build-flags.abi-defines-match", "", match, "ABI_DEFINE_MISMATCH(dear-imgui)");
}
const am::probe::Registration registration("dear-imgui", &am_probe_imgui_abi);

} // namespace
#endif
