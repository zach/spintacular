// SDL3's binding to the allocation contract.
// SPDX-License-Identifier: MIT-0
#pragma once
#include "automap/context.h"

// Routes SDL's allocations through the allocation facade. Runs in the
// memory phase, before any other call into SDL.
bool am_bind_sdl3_allocation(am::Context& ctx);
