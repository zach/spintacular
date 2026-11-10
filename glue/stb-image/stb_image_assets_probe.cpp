// Probes for stb_image's binding to the assets contract.
// SPDX-License-Identifier: MIT-0
#ifdef AUTOMAP_PROBE
#include <chrono>
#include <cstdio>
#include <cstring>
#include <filesystem>
#include <string>

#include "automap/assets.h"
#include "automap/probe.h"
#include "stb_image_assets.h"

namespace {

// A 3x2 RGBA PNG.
const unsigned char sample_png[] = {
    0x89, 0x50, 0x4e, 0x47, 0x0d, 0x0a, 0x1a, 0x0a, 0x00, 0x00, 0x00, 0x0d, 0x49, 0x48, 0x44, 0x52, 0x00, 0x00,
    0x00, 0x03, 0x00, 0x00, 0x00, 0x02, 0x08, 0x06, 0x00, 0x00, 0x00, 0x9d, 0x74, 0x66, 0x1a, 0x00, 0x00, 0x00,
    0x1e, 0x49, 0x44, 0x41, 0x54, 0x78, 0xda, 0x63, 0xf8, 0xcf, 0xc0, 0xf0, 0x1f, 0x08, 0x1b, 0x40, 0x14, 0x83,
    0xa0, 0x92, 0xb1, 0xcb, 0x89, 0x14, 0x23, 0x49, 0x46, 0x26, 0x66, 0x16, 0x00, 0x6c, 0x25, 0x06, 0xa8, 0x4a,
    0xec, 0xda, 0xe9, 0x00, 0x00, 0x00, 0x00, 0x49, 0x45, 0x4e, 0x44, 0xae, 0x42, 0x60, 0x82};

// Writes the sample image to a file of its own in the temp directory.
std::filesystem::path write_sample() {
    auto stamp = std::chrono::steady_clock::now().time_since_epoch().count();
    std::filesystem::path path = std::filesystem::temp_directory_path() / ("automap-probe-" + std::to_string(stamp) + ".png");
    std::FILE* f = std::fopen(path.string().c_str(), "wb");
    if (f != nullptr) {
        std::fwrite(sample_png, 1, sizeof sample_png, f);
        std::fclose(f);
    }
    return path;
}

// assets.reads-through-interface: decoding the sample file reads it
// through the assets interface. assets.output-identical: decoding it twice
// gives the same pixels.
void am_probe_stb_image_assets(am::Context&) {
    std::filesystem::path path = write_sample();
    auto before = am::assets::read_count("stb-image");
    int w1 = 0, h1 = 0, c1 = 0, w2 = 0, h2 = 0, c2 = 0;
    stbi_uc* first = am_stb_image_load(path.string().c_str(), &w1, &h1, &c1, 4);
    bool through = first != nullptr && am::assets::read_count("stb-image") == before + 1;
    stbi_uc* second = am_stb_image_load(path.string().c_str(), &w2, &h2, &c2, 4);
    bool identical = first != nullptr && second != nullptr && w1 == 3 && h1 == 2 && w1 == w2 && h1 == h2 && c1 == c2 &&
                     std::memcmp(first, second, static_cast<std::size_t>(w1) * h1 * 4) == 0;
    stbi_image_free(first);
    stbi_image_free(second);
    std::error_code ignored;
    std::filesystem::remove(path, ignored);
    am::probe::check("assets.reads-through-interface", "stb-image", through, "ASSET_READ_BYPASSED(stb-image)");
    am::probe::check("assets.output-identical", "stb-image", identical, "ASSET_OUTPUT_DIFFERS(stb-image)");
}
const am::probe::Registration registration("stb-image--assets", &am_probe_stb_image_assets);

} // namespace
#endif
