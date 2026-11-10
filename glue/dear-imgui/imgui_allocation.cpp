// Dear ImGui's binding to the allocation contract.
// SPDX-License-Identifier: MIT-0
#include <imgui.h>

#include "automap/alloc.h"
#include "automap/context.h"
#include "automap/lifecycle.h"

void* am_imgui_alloc(size_t size, void*) { return am::alloc::allocate("dear-imgui", size); }
void am_imgui_free(void* block, void*) { am::alloc::release("dear-imgui", block); }

// Routes Dear ImGui's allocations through the allocation facade. Runs in
// the memory phase, before ImGui::CreateContext().
bool am_bind_imgui_allocation(am::Context&) {
    ImGui::SetAllocatorFunctions(am_imgui_alloc, am_imgui_free, nullptr);
    am::lifecycle::init_step("dear-imgui:SetAllocatorFunctions");
    return true;
}
