// Probes for stb_truetype's binding to the allocation contract.
#ifdef AUTOMAP_PROBE
#include <stb_truetype.h>

#include "automap/alloc.h"
#include "automap/probe.h"

namespace {

// allocation.routed-through-facade: a glyph packing context allocates its
// bookkeeping through the facade.
void am_probe_stb_truetype_allocation(am::Context&) {
    auto before = am::alloc::routed_count("stb-truetype");
    unsigned char pixels[16 * 16];
    stbtt_pack_context pack;
    bool began = stbtt_PackBegin(&pack, pixels, 16, 16, 0, 1, nullptr) != 0;
    bool routed = began && am::alloc::routed_count("stb-truetype") > before;
    if (began) stbtt_PackEnd(&pack);
    am::probe::check("allocation.routed-through-facade", "stb-truetype", routed, "ALLOC_NOT_ROUTED(stb-truetype)");
}
const am::probe::Registration registration("stb-truetype--allocation", &am_probe_stb_truetype_allocation);

} // namespace
#endif
