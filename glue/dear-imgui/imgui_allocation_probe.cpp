// Probes for Dear ImGui's binding to the allocation contract.
// SPDX-License-Identifier: MIT-0
#ifdef AUTOMAP_PROBE
#include <imgui.h>

#include "automap/alloc.h"
#include "automap/probe.h"

void* am_imgui_alloc(size_t size, void*);
void am_imgui_free(void* block, void*);

namespace {

// allocation.routed-through-facade: ImGui's installed functions are the
// facade's, and the facade served ImGui during the run.
void am_probe_imgui_allocation(am::Context&) {
    ImGuiMemAllocFunc alloc = nullptr;
    ImGuiMemFreeFunc release = nullptr;
    void* user = nullptr;
    ImGui::GetAllocatorFunctions(&alloc, &release, &user);
    bool routed = alloc == am_imgui_alloc && release == am_imgui_free && am::alloc::routed_count("dear-imgui") > 0;
    am::probe::check("allocation.routed-through-facade", "dear-imgui", routed, "ALLOC_NOT_ROUTED(dear-imgui)");
}
const am::probe::Registration routed("dear-imgui--allocation", &am_probe_imgui_allocation);

// allocation.hooks-before-first-allocation: after ImGui::DestroyContext(),
// ImGui has released nothing the facade didn't allocate.
void am_probe_imgui_hooks_early(am::Context&) {
    bool early = !am::alloc::released_foreign_block("dear-imgui");
    am::probe::check("allocation.hooks-before-first-allocation", "dear-imgui", early, "ALLOC_HOOK_LATE(dear-imgui)");
}
const am::probe::Registration early("dear-imgui--allocation", &am_probe_imgui_hooks_early, am::probe::When::after_shutdown);

} // namespace
#endif
