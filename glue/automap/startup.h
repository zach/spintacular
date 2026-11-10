// Written by automap from the composition's recipes. Edit the manifest, not this file.
#pragma once
#include "automap/context.h"

namespace am {

// Calls every init entry point in phase order. When one fails, reports
// INIT_FAILED(<call>) on stderr and returns false.
bool startup(Context& ctx);

// Calls every shutdown entry point in reverse phase order.
void shutdown(Context& ctx);

} // namespace am
