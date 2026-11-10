// stb_image's binding to the assets contract: images decoded from files the
// assets interface reads.
// SPDX-License-Identifier: MIT-0
#pragma once
#include <stb_image.h>

// Decodes the image file at path, read through the assets interface, as
// stbi_load would decode it. Free the pixels with stbi_image_free.
stbi_uc* am_stb_image_load(const char* path, int* width, int* height, int* channels, int desired_channels);
