// Dear ImGui's setup glue.
// SPDX-License-Identifier: MIT-0
#pragma once
#include "automap/context.h"

// Creates the ImGui context with docking enabled. In probe builds it writes
// no imgui.ini. Runs in the systems phase, after the allocation hooks.
bool am_imgui_setup_init(am::Context& ctx);

// Destroys the ImGui context, after its backends have shut down.
void am_imgui_setup_shutdown(am::Context& ctx);
