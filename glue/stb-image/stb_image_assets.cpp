// stb_image's binding to the assets contract.
// SPDX-License-Identifier: MIT-0
#include "stb_image_assets.h"

#include <climits>
#include <vector>

#include "automap/assets.h"

stbi_uc* am_stb_image_load(const char* path, int* width, int* height, int* channels, int desired_channels) {
    std::vector<std::byte> bytes;
    if (!am::assets::read("stb-image", path, bytes) || bytes.size() > INT_MAX) return nullptr;
    stbi_uc* pixels = stbi_load_from_memory(reinterpret_cast<const stbi_uc*>(bytes.data()), static_cast<int>(bytes.size()),
                                            width, height, channels, desired_channels);
    return pixels;
}
