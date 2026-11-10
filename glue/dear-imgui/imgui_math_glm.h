// Dear ImGui's binding to the math-types contract: adapters between ImGui's
// vectors and GLM's, for a composition with GLM in the math slot. ImGui's
// coordinates are screen pixels with the origin at the top left and y
// pointing down; the adapters copy components and leave that convention to
// the caller.
// SPDX-License-Identifier: MIT-0
#pragma once
#if defined(AUTOMAP_LIBRARY_GLM)
#include <imgui.h>

#include <glm/glm.hpp>

namespace am::math {

inline glm::vec2 to_glm(const ImVec2& v) { return glm::vec2(v.x, v.y); }
inline ImVec2 to_imgui(const glm::vec2& v) { return ImVec2(v.x, v.y); }

inline glm::vec4 to_glm(const ImVec4& v) { return glm::vec4(v.x, v.y, v.z, v.w); }
inline ImVec4 to_imgui(const glm::vec4& v) { return ImVec4(v.x, v.y, v.z, v.w); }

} // namespace am::math
#endif
