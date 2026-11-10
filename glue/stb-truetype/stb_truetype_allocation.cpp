// stb_truetype's binding to the allocation contract, and the one translation
// unit that compiles stb_truetype's implementation. Its allocation hooks are
// macros the implementation reads, so they are defined here, before it is
// included.
#include "automap/alloc.h"

#define STBTT_malloc(size, userdata) ((void)(userdata), am::alloc::allocate("stb-truetype", size))
#define STBTT_free(block, userdata) ((void)(userdata), am::alloc::release("stb-truetype", block))
#define STB_TRUETYPE_IMPLEMENTATION
#include <stb_truetype.h>
