// stb_image's binding to the allocation contract, and the one translation
// unit that compiles stb_image's implementation. stb_image's allocation
// hooks are macros its implementation reads, so they are defined here,
// before it is included.
// SPDX-License-Identifier: MIT-0
#include "automap/alloc.h"

#define STBI_MALLOC(size) am::alloc::allocate("stb-image", size)
#define STBI_REALLOC(block, size) am::alloc::reallocate("stb-image", block, size)
#define STBI_FREE(block) am::alloc::release("stb-image", block)
#define STB_IMAGE_IMPLEMENTATION
#include <stb_image.h>
