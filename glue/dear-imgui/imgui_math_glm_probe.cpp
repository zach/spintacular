// Probes for Dear ImGui's binding to the math-types contract.
// SPDX-License-Identifier: MIT-0
#if defined(AUTOMAP_PROBE) && defined(AUTOMAP_LIBRARY_GLM)
#include <cfloat>
#include <cstring>

#include "automap/probe.h"
#include "imgui_math_glm.h"

namespace {

// Sample values that a conversion through another type or precision would
// change: thirds, tiny and huge magnitudes, the smallest normal float,
// negative zero.
const float samples[] = {1.0f / 3.0f, -2.5e-7f, 3.0e38f, FLT_MIN, -0.0f, 12345.678f, 0.1f, 1.0e7f + 1.0f};

template <typename T> bool same(const T& a, const T& b) { return std::memcmp(&a, &b, sizeof(T)) == 0; }

// math-types.adapters-round-trip: ImVec2 and ImVec4 sample values convert
// to GLM's types and back bit for bit.
void am_probe_imgui_math(am::Context&) {
    bool ok = true;
    for (int i = 0; i + 4 <= 8; i += 2) {
        const float* s = samples + i;
        ImVec2 v2(s[0], s[1]);
        ImVec4 v4(s[0], s[1], s[2], s[3]);
        ok = ok && same(am::math::to_imgui(am::math::to_glm(v2)), v2) && same(am::math::to_imgui(am::math::to_glm(v4)), v4);
    }
    am::probe::check("math-types.adapters-round-trip", "dear-imgui", ok, "ADAPTER_LOSSY(dear-imgui)");
}
const am::probe::Registration registration("dear-imgui--math-types", &am_probe_imgui_math);

} // namespace
#endif
