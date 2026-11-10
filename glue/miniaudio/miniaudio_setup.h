// miniaudio's setup glue: the composition's one audio engine, the context
// it plays through, and the log and allocation callbacks its bindings fill
// in before the systems phase starts it.
// SPDX-License-Identifier: MIT-0
#pragma once
#include <miniaudio.h>

#include "automap/context.h"

// The allocation callbacks the log, context and engine copy when they are
// made. The allocation binding fills them in the memory phase; until then
// they are empty, and miniaudio uses the C runtime's allocator.
ma_allocation_callbacks& am_miniaudio_allocation();

// The log the context and engine write to. The diagnostics binding makes it
// in the diagnostics phase.
ma_log& am_miniaudio_log();

// The composition's audio engine, once the systems phase has started it.
ma_engine& am_miniaudio_engine();

// Starts the context and the engine on it. With AUTOMAP_MINIAUDIO_BACKEND
// set to null, as verify sets it for headless runs, the context uses only
// miniaudio's null backend, which needs no audio device.
bool am_miniaudio_setup_init(am::Context& ctx);

// Stops the engine, then the context.
void am_miniaudio_setup_shutdown(am::Context& ctx);
