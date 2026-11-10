#include "gfx.h"

#include <algorithm>
#include <cmath>
#include <cstring>
#include <vector>

#include <SDL3/SDL.h>
#include <stb_truetype.h>

#include "automap/assets.h"
#include "automap/diag.h"
#include "sdl-shadercross--sdl3/shadercross_sdl3.h"
#include "shaders/shaders_spv.h"

namespace gfx {

namespace {

struct Vertex {
    float x, y, u, v;
    std::uint8_t r, g, b, a;
};

struct Transform {
    float a = 1, b = 0, c = 0, d = 1, tx = 0, ty = 0; // x' = a x + c y + tx; y' = b x + d y + ty
};

// Fonts are baked at these canvas sizes (times the window scale); text is
// drawn from the smallest one that is big enough.
const float kBakeSizes[] = {18, 26, 36, 50, 72, 104, 150};
constexpr int kBakeCount = static_cast<int>(sizeof kBakeSizes / sizeof kBakeSizes[0]);
const int kCodepoints[] = {
    32,  33,  34,  35,  36,  37,  38,  39,  40,  41,  42,  43,  44,  45,  46,  47,  48,  49,  50,  51,  52,  53,  54,  55,
    56,  57,  58,  59,  60,  61,  62,  63,  64,  65,  66,  67,  68,  69,  70,  71,  72,  73,  74,  75,  76,  77,  78,  79,
    80,  81,  82,  83,  84,  85,  86,  87,  88,  89,  90,  91,  92,  93,  94,  95,  96,  97,  98,  99,  100, 101, 102, 103,
    104, 105, 106, 107, 108, 109, 110, 111, 112, 113, 114, 115, 116, 117, 118, 119, 120, 121, 122, 123, 124, 125, 126, 0xD7, 0xB7};
constexpr int kGlyphs = static_cast<int>(sizeof kCodepoints / sizeof kCodepoints[0]);

struct FontData {
    std::vector<std::byte> ttf;
    stbtt_packedchar glyphs[kBakeCount][kGlyphs];
    bool ok = false;
};

struct State {
    SDL_GPUDevice* device = nullptr;
    SDL_Window* window = nullptr;
    SDL_GPUGraphicsPipeline* pipeline = nullptr;
    SDL_GPUSampler* sampler = nullptr;
    SDL_GPUTexture* atlas = nullptr;
    SDL_GPUTexture* msaa = nullptr;
    SDL_GPUTexture* scene = nullptr;
    SDL_GPUBuffer* vbuf = nullptr;
    SDL_GPUBuffer* ibuf = nullptr;
    SDL_GPUTransferBuffer* transfer = nullptr;
    Uint32 vcap = 0, icap = 0, tcap = 0;
    SDL_GPUSampleCount samples = SDL_GPU_SAMPLECOUNT_1;
    int width = 0, height = 0; // scene size in pixels
    float scale = 1, off_x = 0, off_y = 0;
    float baked_scale = 0;
    int atlas_size = 2048;
    float white_u = 0, white_v = 0;
    std::vector<Vertex> vertices;
    std::vector<Uint32> indices;
    std::vector<Transform> transforms;
    std::vector<float> alphas;
    float alpha = 1;
    FontData fonts[2];
} s;

std::uint8_t to_byte(float v) { return static_cast<std::uint8_t>(std::clamp(v, 0.0f, 1.0f) * 255.0f + 0.5f); }

Uint32 add_vertex(float x, float y, float u, float v, Color c) {
    const Transform& t = s.transforms.back();
    float px = (t.a * x + t.c * y + t.tx) * s.scale + s.off_x;
    float py = (t.b * x + t.d * y + t.ty) * s.scale + s.off_y;
    s.vertices.push_back({px, py, u, v, to_byte(c.r), to_byte(c.g), to_byte(c.b), to_byte(c.a * s.alpha)});
    return static_cast<Uint32>(s.vertices.size() - 1);
}

Uint32 flat(float x, float y, Color c) { return add_vertex(x, y, s.white_u, s.white_v, c); }

void release_targets() {
    if (s.msaa) SDL_ReleaseGPUTexture(s.device, s.msaa);
    if (s.scene) SDL_ReleaseGPUTexture(s.device, s.scene);
    s.msaa = s.scene = nullptr;
}

bool make_targets(int w, int h) {
    release_targets();
    SDL_GPUTextureCreateInfo info = {};
    info.type = SDL_GPU_TEXTURETYPE_2D;
    info.format = SDL_GPU_TEXTUREFORMAT_R8G8B8A8_UNORM;
    info.width = static_cast<Uint32>(w);
    info.height = static_cast<Uint32>(h);
    info.layer_count_or_depth = 1;
    info.num_levels = 1;
    info.sample_count = SDL_GPU_SAMPLECOUNT_1;
    info.usage = SDL_GPU_TEXTUREUSAGE_COLOR_TARGET | SDL_GPU_TEXTUREUSAGE_SAMPLER;
    s.scene = SDL_CreateGPUTexture(s.device, &info);
    if (s.samples != SDL_GPU_SAMPLECOUNT_1) {
        info.sample_count = s.samples;
        info.usage = SDL_GPU_TEXTUREUSAGE_COLOR_TARGET;
        s.msaa = SDL_CreateGPUTexture(s.device, &info);
    }
    s.width = w;
    s.height = h;
    return s.scene != nullptr && (s.samples == SDL_GPU_SAMPLECOUNT_1 || s.msaa != nullptr);
}

// Bakes both fonts into the atlas at the current window scale.
bool bake_atlas() {
    am::diag::Zone zone("gfx: bake font atlas");
    const int size = s.scale > 1.3f ? 4096 : 2048;
    std::vector<unsigned char> alpha(static_cast<std::size_t>(size) * size, 0);
    stbtt_pack_context pack;
    // The last rows are kept for the white block flat shapes sample.
    if (!stbtt_PackBegin(&pack, alpha.data(), size, size - 8, size, 2, nullptr)) return false;
    for (int f = 0; f < 2; ++f) {
        if (!s.fonts[f].ok) continue;
        for (int b = 0; b < kBakeCount; ++b) {
            float px = kBakeSizes[b] * s.scale;
            int over = px < 40 ? 2 : 1;
            stbtt_PackSetOversampling(&pack, over, over);
            stbtt_pack_range range = {};
            range.font_size = px;
            range.array_of_unicode_codepoints = const_cast<int*>(kCodepoints);
            range.num_chars = kGlyphs;
            range.chardata_for_range = s.fonts[f].glyphs[b];
            stbtt_PackFontRanges(&pack, reinterpret_cast<const unsigned char*>(s.fonts[f].ttf.data()), 0, &range, 1);
        }
    }
    stbtt_PackEnd(&pack);
    for (int y = size - 6; y < size - 1; ++y)
        for (int x = 1; x < 6; ++x) alpha[static_cast<std::size_t>(y) * size + x] = 255;
    s.white_u = 3.5f / size;
    s.white_v = (size - 3.5f) / size;

    std::vector<unsigned char> rgba(alpha.size() * 4);
    for (std::size_t i = 0; i < alpha.size(); ++i) {
        rgba[i * 4] = rgba[i * 4 + 1] = rgba[i * 4 + 2] = 255;
        rgba[i * 4 + 3] = alpha[i];
    }
    if (s.atlas && s.atlas_size != size) {
        SDL_ReleaseGPUTexture(s.device, s.atlas);
        s.atlas = nullptr;
    }
    s.atlas_size = size;
    if (!s.atlas) {
        SDL_GPUTextureCreateInfo info = {};
        info.type = SDL_GPU_TEXTURETYPE_2D;
        info.format = SDL_GPU_TEXTUREFORMAT_R8G8B8A8_UNORM;
        info.width = info.height = static_cast<Uint32>(size);
        info.layer_count_or_depth = 1;
        info.num_levels = 1;
        info.sample_count = SDL_GPU_SAMPLECOUNT_1;
        info.usage = SDL_GPU_TEXTUREUSAGE_SAMPLER;
        s.atlas = SDL_CreateGPUTexture(s.device, &info);
        if (!s.atlas) return false;
    }
    SDL_GPUTransferBufferCreateInfo tinfo = {};
    tinfo.usage = SDL_GPU_TRANSFERBUFFERUSAGE_UPLOAD;
    tinfo.size = static_cast<Uint32>(rgba.size());
    SDL_GPUTransferBuffer* tb = SDL_CreateGPUTransferBuffer(s.device, &tinfo);
    if (!tb) return false;
    std::memcpy(SDL_MapGPUTransferBuffer(s.device, tb, false), rgba.data(), rgba.size());
    SDL_UnmapGPUTransferBuffer(s.device, tb);
    SDL_GPUCommandBuffer* cmd = SDL_AcquireGPUCommandBuffer(s.device);
    SDL_GPUCopyPass* copy = SDL_BeginGPUCopyPass(cmd);
    SDL_GPUTextureTransferInfo src = {};
    src.transfer_buffer = tb;
    src.pixels_per_row = static_cast<Uint32>(size);
    src.rows_per_layer = static_cast<Uint32>(size);
    SDL_GPUTextureRegion dst = {};
    dst.texture = s.atlas;
    dst.w = dst.h = static_cast<Uint32>(size);
    dst.d = 1;
    SDL_UploadToGPUTexture(copy, &src, &dst, false);
    SDL_EndGPUCopyPass(copy);
    SDL_SubmitGPUCommandBuffer(cmd);
    SDL_ReleaseGPUTransferBuffer(s.device, tb);
    s.baked_scale = s.scale;
    return true;
}

void update_layout() {
    int w = 0, h = 0;
    SDL_GetWindowSizeInPixels(s.window, &w, &h);
    w = std::max(w, 16);
    h = std::max(h, 16);
    if (w != s.width || h != s.height || !s.scene) make_targets(w, h);
    s.scale = std::min(w / kWidth, h / kHeight);
    s.off_x = (w - kWidth * s.scale) * 0.5f;
    s.off_y = (h - kHeight * s.scale) * 0.5f;
    if (std::fabs(s.scale - s.baked_scale) > 0.02f * s.scale) bake_atlas();
}

int glyph_index(int codepoint) {
    if (codepoint >= 32 && codepoint <= 126) return codepoint - 32;
    for (int i = 95; i < kGlyphs; ++i)
        if (kCodepoints[i] == codepoint) return i;
    return '?' - 32;
}

// Decodes one UTF-8 codepoint (ASCII and two-byte sequences are all the game uses).
int next_codepoint(std::string_view str, std::size_t& i) {
    unsigned char c = static_cast<unsigned char>(str[i++]);
    if (c < 0x80) return c;
    if ((c & 0xE0) == 0xC0 && i < str.size()) return ((c & 0x1F) << 6) | (static_cast<unsigned char>(str[i++]) & 0x3F);
    while (i < str.size() && (static_cast<unsigned char>(str[i]) & 0xC0) == 0x80) ++i;
    return '?';
}

int bake_for(float size) {
    for (int b = 0; b < kBakeCount; ++b)
        if (kBakeSizes[b] >= size - 0.5f) return b;
    return kBakeCount - 1;
}

int arc_segments(float r, float sweep) {
    float px = std::max(1.0f, r * s.scale);
    int n = static_cast<int>(std::ceil(std::fabs(sweep) * std::sqrt(px) * 1.6f));
    return std::clamp(n, 2, 160);
}

} // namespace

Color mix(Color a, Color b, float t) {
    return {a.r + (b.r - a.r) * t, a.g + (b.g - a.g) * t, a.b + (b.b - a.b) * t, a.a + (b.a - a.a) * t};
}
Color darken(Color c, float amount) { return {c.r * (1 - amount), c.g * (1 - amount), c.b * (1 - amount), c.a}; }
Color lighten(Color c, float amount) { return mix(c, Color{1, 1, 1, c.a}, amount); }

bool init(SDL_GPUDevice* device, SDL_Window* window, const std::string& asset_dir) {
    s.device = device;
    s.window = window;
    s.transforms.assign(1, Transform{});

    for (SDL_GPUSampleCount count : {SDL_GPU_SAMPLECOUNT_4, SDL_GPU_SAMPLECOUNT_2})
        if (SDL_GPUTextureSupportsSampleCount(device, SDL_GPU_TEXTUREFORMAT_R8G8B8A8_UNORM, count)) {
            s.samples = count;
            break;
        }

    SDL_GPUShader* vs = am_shadercross_create_shader(device, shape_vert_spv, sizeof shape_vert_spv, "main",
                                                     SDL_SHADERCROSS_SHADERSTAGE_VERTEX);
    SDL_GPUShader* fs = am_shadercross_create_shader(device, shape_frag_spv, sizeof shape_frag_spv, "main",
                                                     SDL_SHADERCROSS_SHADERSTAGE_FRAGMENT);
    if (!vs || !fs) {
        am::diag::log(am::diag::Level::error, "spintacular", SDL_GetError());
        return false;
    }
    SDL_GPUVertexBufferDescription vb = {};
    vb.slot = 0;
    vb.pitch = sizeof(Vertex);
    vb.input_rate = SDL_GPU_VERTEXINPUTRATE_VERTEX;
    SDL_GPUVertexAttribute attrs[3] = {};
    attrs[0] = {0, 0, SDL_GPU_VERTEXELEMENTFORMAT_FLOAT2, 0};
    attrs[1] = {1, 0, SDL_GPU_VERTEXELEMENTFORMAT_FLOAT2, 8};
    attrs[2] = {2, 0, SDL_GPU_VERTEXELEMENTFORMAT_UBYTE4_NORM, 16};
    SDL_GPUColorTargetDescription target = {};
    target.format = SDL_GPU_TEXTUREFORMAT_R8G8B8A8_UNORM;
    target.blend_state.enable_blend = true;
    target.blend_state.src_color_blendfactor = SDL_GPU_BLENDFACTOR_SRC_ALPHA;
    target.blend_state.dst_color_blendfactor = SDL_GPU_BLENDFACTOR_ONE_MINUS_SRC_ALPHA;
    target.blend_state.color_blend_op = SDL_GPU_BLENDOP_ADD;
    target.blend_state.src_alpha_blendfactor = SDL_GPU_BLENDFACTOR_ONE;
    target.blend_state.dst_alpha_blendfactor = SDL_GPU_BLENDFACTOR_ONE_MINUS_SRC_ALPHA;
    target.blend_state.alpha_blend_op = SDL_GPU_BLENDOP_ADD;
    SDL_GPUGraphicsPipelineCreateInfo pinfo = {};
    pinfo.vertex_shader = vs;
    pinfo.fragment_shader = fs;
    pinfo.vertex_input_state.vertex_buffer_descriptions = &vb;
    pinfo.vertex_input_state.num_vertex_buffers = 1;
    pinfo.vertex_input_state.vertex_attributes = attrs;
    pinfo.vertex_input_state.num_vertex_attributes = 3;
    pinfo.primitive_type = SDL_GPU_PRIMITIVETYPE_TRIANGLELIST;
    pinfo.rasterizer_state.fill_mode = SDL_GPU_FILLMODE_FILL;
    pinfo.rasterizer_state.cull_mode = SDL_GPU_CULLMODE_NONE;
    pinfo.multisample_state.sample_count = s.samples;
    pinfo.target_info.color_target_descriptions = &target;
    pinfo.target_info.num_color_targets = 1;
    s.pipeline = SDL_CreateGPUGraphicsPipeline(device, &pinfo);
    SDL_ReleaseGPUShader(device, vs);
    SDL_ReleaseGPUShader(device, fs);
    if (!s.pipeline) {
        am::diag::log(am::diag::Level::error, "spintacular", SDL_GetError());
        return false;
    }
    SDL_GPUSamplerCreateInfo sinfo = {};
    sinfo.min_filter = sinfo.mag_filter = SDL_GPU_FILTER_LINEAR;
    sinfo.mipmap_mode = SDL_GPU_SAMPLERMIPMAPMODE_NEAREST;
    sinfo.address_mode_u = sinfo.address_mode_v = sinfo.address_mode_w = SDL_GPU_SAMPLERADDRESSMODE_CLAMP_TO_EDGE;
    s.sampler = SDL_CreateGPUSampler(device, &sinfo);

    const char* files[2] = {"/fonts/AlfaSlabOne-Regular.ttf", "/fonts/Lato-Bold.ttf"};
    for (int f = 0; f < 2; ++f) {
        std::string path = asset_dir + files[f];
        s.fonts[f].ok = am::assets::read("stb-truetype", path.c_str(), s.fonts[f].ttf) && !s.fonts[f].ttf.empty();
        if (!s.fonts[f].ok) am::diag::log(am::diag::Level::error, "spintacular", ("can't read font " + path).c_str());
    }
    update_layout();
    return s.sampler != nullptr && s.atlas != nullptr && s.fonts[0].ok && s.fonts[1].ok;
}

void shutdown() {
    if (!s.device) return;
    release_targets();
    if (s.atlas) SDL_ReleaseGPUTexture(s.device, s.atlas);
    if (s.vbuf) SDL_ReleaseGPUBuffer(s.device, s.vbuf);
    if (s.ibuf) SDL_ReleaseGPUBuffer(s.device, s.ibuf);
    if (s.transfer) SDL_ReleaseGPUTransferBuffer(s.device, s.transfer);
    if (s.sampler) SDL_ReleaseGPUSampler(s.device, s.sampler);
    if (s.pipeline) SDL_ReleaseGPUGraphicsPipeline(s.device, s.pipeline);
    s = State{};
}

void begin_frame() {
    update_layout();
    s.vertices.clear();
    s.indices.clear();
    s.transforms.assign(1, Transform{});
    s.alphas.clear();
    s.alpha = 1;
}

void end_frame(SDL_GPUCommandBuffer* cmd) {
    am::diag::Zone zone("gfx: end frame");
    Uint32 vbytes = static_cast<Uint32>(s.vertices.size() * sizeof(Vertex));
    Uint32 ibytes = static_cast<Uint32>(s.indices.size() * sizeof(Uint32));
    if (vbytes > 0 && ibytes > 0) {
        if (vbytes > s.vcap) {
            if (s.vbuf) SDL_ReleaseGPUBuffer(s.device, s.vbuf);
            s.vcap = std::max(vbytes * 2, 1u << 20);
            SDL_GPUBufferCreateInfo b = {SDL_GPU_BUFFERUSAGE_VERTEX, s.vcap, 0};
            s.vbuf = SDL_CreateGPUBuffer(s.device, &b);
        }
        if (ibytes > s.icap) {
            if (s.ibuf) SDL_ReleaseGPUBuffer(s.device, s.ibuf);
            s.icap = std::max(ibytes * 2, 1u << 19);
            SDL_GPUBufferCreateInfo b = {SDL_GPU_BUFFERUSAGE_INDEX, s.icap, 0};
            s.ibuf = SDL_CreateGPUBuffer(s.device, &b);
        }
        if (vbytes + ibytes > s.tcap) {
            if (s.transfer) SDL_ReleaseGPUTransferBuffer(s.device, s.transfer);
            s.tcap = std::max((vbytes + ibytes) * 2, 1u << 21);
            SDL_GPUTransferBufferCreateInfo t = {SDL_GPU_TRANSFERBUFFERUSAGE_UPLOAD, s.tcap, 0};
            s.transfer = SDL_CreateGPUTransferBuffer(s.device, &t);
        }
        auto* mapped = static_cast<std::uint8_t*>(SDL_MapGPUTransferBuffer(s.device, s.transfer, true));
        std::memcpy(mapped, s.vertices.data(), vbytes);
        std::memcpy(mapped + vbytes, s.indices.data(), ibytes);
        SDL_UnmapGPUTransferBuffer(s.device, s.transfer);
        SDL_GPUCopyPass* copy = SDL_BeginGPUCopyPass(cmd);
        SDL_GPUTransferBufferLocation src = {s.transfer, 0};
        SDL_GPUBufferRegion dst = {s.vbuf, 0, vbytes};
        SDL_UploadToGPUBuffer(copy, &src, &dst, true);
        src.offset = vbytes;
        dst = {s.ibuf, 0, ibytes};
        SDL_UploadToGPUBuffer(copy, &src, &dst, true);
        SDL_EndGPUCopyPass(copy);
    }

    SDL_GPUColorTargetInfo target = {};
    target.clear_color = SDL_FColor{0.043f, 0.063f, 0.149f, 1.0f};
    target.load_op = SDL_GPU_LOADOP_CLEAR;
    if (s.msaa) {
        target.texture = s.msaa;
        target.store_op = SDL_GPU_STOREOP_RESOLVE;
        target.resolve_texture = s.scene;
        target.cycle = true;
        target.cycle_resolve_texture = true;
    } else {
        target.texture = s.scene;
        target.store_op = SDL_GPU_STOREOP_STORE;
        target.cycle = true;
    }
    SDL_GPURenderPass* pass = SDL_BeginGPURenderPass(cmd, &target, 1, nullptr);
    if (vbytes > 0 && ibytes > 0) {
        SDL_BindGPUGraphicsPipeline(pass, s.pipeline);
        float frame[4] = {2.0f / s.width, -2.0f / s.height, -1.0f, 1.0f};
        SDL_PushGPUVertexUniformData(cmd, 0, frame, sizeof frame);
        SDL_GPUBufferBinding vb = {s.vbuf, 0};
        SDL_BindGPUVertexBuffers(pass, 0, &vb, 1);
        SDL_GPUBufferBinding ib = {s.ibuf, 0};
        SDL_BindGPUIndexBuffer(pass, &ib, SDL_GPU_INDEXELEMENTSIZE_32BIT);
        SDL_GPUTextureSamplerBinding tex = {s.atlas, s.sampler};
        SDL_BindGPUFragmentSamplers(pass, 0, &tex, 1);
        SDL_DrawGPUIndexedPrimitives(pass, static_cast<Uint32>(s.indices.size()), 1, 0, 0, 0);
    }
    SDL_EndGPURenderPass(pass);
}

SDL_GPUTexture* scene_texture() { return s.scene; }
void scene_size(int* w, int* h) {
    *w = s.width;
    *h = s.height;
}

bool save_screenshot(const std::string& path) {
    if (!s.scene) return false;
    Uint32 bytes = static_cast<Uint32>(s.width) * static_cast<Uint32>(s.height) * 4;
    SDL_GPUTransferBufferCreateInfo tinfo = {SDL_GPU_TRANSFERBUFFERUSAGE_DOWNLOAD, bytes, 0};
    SDL_GPUTransferBuffer* tb = SDL_CreateGPUTransferBuffer(s.device, &tinfo);
    if (!tb) return false;
    SDL_GPUCommandBuffer* cmd = SDL_AcquireGPUCommandBuffer(s.device);
    SDL_GPUCopyPass* copy = SDL_BeginGPUCopyPass(cmd);
    SDL_GPUTextureRegion src = {};
    src.texture = s.scene;
    src.w = static_cast<Uint32>(s.width);
    src.h = static_cast<Uint32>(s.height);
    src.d = 1;
    SDL_GPUTextureTransferInfo dst = {};
    dst.transfer_buffer = tb;
    dst.pixels_per_row = static_cast<Uint32>(s.width);
    dst.rows_per_layer = static_cast<Uint32>(s.height);
    SDL_DownloadFromGPUTexture(copy, &src, &dst);
    SDL_EndGPUCopyPass(copy);
    SDL_GPUFence* fence = SDL_SubmitGPUCommandBufferAndAcquireFence(cmd);
    bool ok = false;
    if (fence) {
        SDL_WaitForGPUFences(s.device, true, &fence, 1);
        SDL_ReleaseGPUFence(s.device, fence);
        void* pixels = SDL_MapGPUTransferBuffer(s.device, tb, false);
        if (pixels) {
            SDL_Surface* surface = SDL_CreateSurfaceFrom(s.width, s.height, SDL_PIXELFORMAT_RGBA32, pixels, s.width * 4);
            if (surface) {
                ok = SDL_SavePNG(surface, path.c_str());
                SDL_DestroySurface(surface);
            }
            SDL_UnmapGPUTransferBuffer(s.device, tb);
        }
    }
    SDL_ReleaseGPUTransferBuffer(s.device, tb);
    return ok;
}

Vec2 to_canvas(float wx, float wy) {
    int ww = 1, wh = 1;
    SDL_GetWindowSize(s.window, &ww, &wh);
    float px = wx * s.width / std::max(1, ww), py = wy * s.height / std::max(1, wh);
    return {(px - s.off_x) / s.scale, (py - s.off_y) / s.scale};
}

// ---------------------------------------------------------------- transforms

void push(float tx, float ty, float rotation, float scale) {
    const Transform p = s.transforms.back();
    float cs = std::cos(rotation) * scale, sn = std::sin(rotation) * scale;
    // local = T(tx,ty) * R * S
    Transform l{cs, sn, -sn, cs, tx, ty};
    Transform t;
    t.a = p.a * l.a + p.c * l.b;
    t.b = p.b * l.a + p.d * l.b;
    t.c = p.a * l.c + p.c * l.d;
    t.d = p.b * l.c + p.d * l.d;
    t.tx = p.a * l.tx + p.c * l.ty + p.tx;
    t.ty = p.b * l.tx + p.d * l.ty + p.ty;
    s.transforms.push_back(t);
}

void pop() {
    if (s.transforms.size() > 1) s.transforms.pop_back();
}

void push_alpha(float alpha) {
    s.alphas.push_back(s.alpha);
    s.alpha *= alpha;
}

void pop_alpha() {
    if (!s.alphas.empty()) {
        s.alpha = s.alphas.back();
        s.alphas.pop_back();
    }
}

// ---------------------------------------------------------------- shapes

void tri(Vec2 a, Vec2 b, Vec2 c, Color color) {
    Uint32 i = flat(a.x, a.y, color);
    flat(b.x, b.y, color);
    flat(c.x, c.y, color);
    s.indices.insert(s.indices.end(), {i, i + 1, i + 2});
}

void quad(Vec2 a, Vec2 b, Vec2 c, Vec2 d, Color color) {
    Uint32 i = flat(a.x, a.y, color);
    flat(b.x, b.y, color);
    flat(c.x, c.y, color);
    flat(d.x, d.y, color);
    s.indices.insert(s.indices.end(), {i, i + 1, i + 2, i, i + 2, i + 3});
}

void rect(float x, float y, float w, float h, Color color) { quad({x, y}, {x + w, y}, {x + w, y + h}, {x, y + h}, color); }

void gradient_rect(float x, float y, float w, float h, Color top, Color bottom) {
    Uint32 i = flat(x, y, top);
    flat(x + w, y, top);
    flat(x + w, y + h, bottom);
    flat(x, y + h, bottom);
    s.indices.insert(s.indices.end(), {i, i + 1, i + 2, i, i + 2, i + 3});
}

void rrect(float x, float y, float w, float h, float radius, Color color) {
    radius = std::max(0.0f, std::min(radius, std::min(w, h) * 0.5f));
    if (radius < 0.5f) {
        rect(x, y, w, h, color);
        return;
    }
    Uint32 center = flat(x + w * 0.5f, y + h * 0.5f, color);
    Uint32 first = static_cast<Uint32>(s.vertices.size());
    int seg = std::max(3, arc_segments(radius, kPi * 0.5f));
    const float cx[4] = {x + w - radius, x + radius, x + radius, x + w - radius};
    const float cy[4] = {y + h - radius, y + h - radius, y + radius, y + radius};
    int count = 0;
    for (int corner = 0; corner < 4; ++corner)
        for (int k = 0; k <= seg; ++k) {
            float a = (corner + static_cast<float>(k) / seg) * kPi * 0.5f;
            flat(cx[corner] + std::cos(a) * radius, cy[corner] + std::sin(a) * radius, color);
            ++count;
        }
    for (int k = 0; k < count; ++k)
        s.indices.insert(s.indices.end(), {center, first + static_cast<Uint32>(k), first + static_cast<Uint32>((k + 1) % count)});
}

void panel(float x, float y, float w, float h, float radius, Color fill, Color outline, float ow) {
    if (ow > 0) rrect(x - ow, y - ow, w + 2 * ow, h + 2 * ow, radius + ow, outline);
    rrect(x, y, w, h, radius, fill);
}

void circle(float cx, float cy, float r, Color color) {
    int seg = std::max(8, arc_segments(r, 2 * kPi));
    Uint32 center = flat(cx, cy, color);
    Uint32 first = static_cast<Uint32>(s.vertices.size());
    for (int k = 0; k < seg; ++k) {
        float a = 2 * kPi * k / seg;
        flat(cx + std::cos(a) * r, cy + std::sin(a) * r, color);
    }
    for (int k = 0; k < seg; ++k)
        s.indices.insert(s.indices.end(), {center, first + static_cast<Uint32>(k), first + static_cast<Uint32>((k + 1) % seg)});
}

void sector(float cx, float cy, float r0, float r1, float a0, float a1, Color color) {
    int seg = arc_segments(r1, a1 - a0);
    Uint32 first = static_cast<Uint32>(s.vertices.size());
    for (int k = 0; k <= seg; ++k) {
        float a = a0 + (a1 - a0) * k / seg;
        float dx = std::sin(a), dy = -std::cos(a); // clockwise from 12 o'clock
        flat(cx + dx * r0, cy + dy * r0, color);
        flat(cx + dx * r1, cy + dy * r1, color);
    }
    for (int k = 0; k < seg; ++k) {
        Uint32 i = first + static_cast<Uint32>(k) * 2;
        s.indices.insert(s.indices.end(), {i, i + 1, i + 3, i, i + 3, i + 2});
    }
}

void ring(float cx, float cy, float r_inner, float r_outer, Color color) { sector(cx, cy, r_inner, r_outer, 0, 2 * kPi, color); }

void line(Vec2 a, Vec2 b, float thickness, Color color) {
    float dx = b.x - a.x, dy = b.y - a.y;
    float len = std::sqrt(dx * dx + dy * dy);
    if (len < 1e-4f) return;
    float nx = -dy / len * thickness * 0.5f, ny = dx / len * thickness * 0.5f;
    quad({a.x + nx, a.y + ny}, {b.x + nx, b.y + ny}, {b.x - nx, b.y - ny}, {a.x - nx, a.y - ny}, color);
}

void ngon(float cx, float cy, float r, int sides, float rotation, Color color) {
    Uint32 center = flat(cx, cy, color);
    Uint32 first = static_cast<Uint32>(s.vertices.size());
    for (int k = 0; k < sides; ++k) {
        float a = rotation + 2 * kPi * k / sides;
        flat(cx + std::sin(a) * r, cy - std::cos(a) * r, color);
    }
    for (int k = 0; k < sides; ++k)
        s.indices.insert(s.indices.end(), {center, first + static_cast<Uint32>(k), first + static_cast<Uint32>((k + 1) % sides)});
}

void star(float cx, float cy, float r_outer, float r_inner, int points, float rotation, Color color) {
    Uint32 center = flat(cx, cy, color);
    Uint32 first = static_cast<Uint32>(s.vertices.size());
    int n = points * 2;
    for (int k = 0; k < n; ++k) {
        float a = rotation + kPi * k / points;
        float r = k % 2 == 0 ? r_outer : r_inner;
        flat(cx + std::sin(a) * r, cy - std::cos(a) * r, color);
    }
    for (int k = 0; k < n; ++k)
        s.indices.insert(s.indices.end(), {center, first + static_cast<Uint32>(k), first + static_cast<Uint32>((k + 1) % n)});
}

// ---------------------------------------------------------------- text

float text_width(Font font, float size, std::string_view str) {
    const FontData& f = s.fonts[static_cast<int>(font)];
    if (!f.ok || s.baked_scale <= 0) return 0;
    int b = bake_for(size);
    float k = size / kBakeSizes[b] / s.baked_scale;
    float w = 0;
    for (std::size_t i = 0; i < str.size();) w += f.glyphs[b][glyph_index(next_codepoint(str, i))].xadvance * k;
    return w;
}

void text(Font font, float size, float x, float y, std::string_view str, Color color, Align align) {
    const FontData& f = s.fonts[static_cast<int>(font)];
    if (!f.ok || s.baked_scale <= 0 || str.empty()) return;
    if (align != Align::Left) x -= text_width(font, size, str) * (align == Align::Center ? 0.5f : 1.0f);
    int b = bake_for(size);
    float k = size / kBakeSizes[b] / s.baked_scale;
    float pen_x = 0, pen_y = 0;
    for (std::size_t i = 0; i < str.size();) {
        int g = glyph_index(next_codepoint(str, i));
        stbtt_aligned_quad q;
        stbtt_GetPackedQuad(f.glyphs[b], s.atlas_size, s.atlas_size, g, &pen_x, &pen_y, &q, 0);
        if (q.x1 <= q.x0) continue;
        Uint32 v = add_vertex(x + q.x0 * k, y + q.y0 * k, q.s0, q.t0, color);
        add_vertex(x + q.x1 * k, y + q.y0 * k, q.s1, q.t0, color);
        add_vertex(x + q.x1 * k, y + q.y1 * k, q.s1, q.t1, color);
        add_vertex(x + q.x0 * k, y + q.y1 * k, q.s0, q.t1, color);
        s.indices.insert(s.indices.end(), {v, v + 1, v + 2, v, v + 2, v + 3});
    }
}

void text_outlined(Font font, float size, float x, float y, std::string_view str, Color color, Color outline, float width,
                   Align align) {
    if (align != Align::Left) {
        x -= text_width(font, size, str) * (align == Align::Center ? 0.5f : 1.0f);
        align = Align::Left;
    }
    for (int k = 0; k < 8; ++k) {
        float a = kPi * 0.25f * k;
        text(font, size, x + std::cos(a) * width, y + std::sin(a) * width, str, outline, align);
    }
    text(font, size, x, y + width * 1.5f, str, outline, align); // a little drop below, like a painted sign
    text(font, size, x, y, str, color, align);
}

float text_wrapped(Font font, float size, float x, float y, float max_width, std::string_view str, Color color, Align align,
                   float line_height, bool measure_only) {
    float used = 0;
    std::size_t pos = 0;
    while (pos < str.size()) {
        // Take words until the line is full.
        std::size_t end = pos, last_fit = pos;
        bool newline = false;
        while (end < str.size()) {
            std::size_t next = str.find_first_of(" \n", end);
            if (next == std::string_view::npos) next = str.size();
            if (text_width(font, size, str.substr(pos, next - pos)) > max_width && last_fit > pos) break;
            last_fit = next;
            end = next;
            if (end < str.size() && str[end] == '\n') {
                newline = true;
                break;
            }
            if (end < str.size()) ++end;
        }
        std::string_view ln = str.substr(pos, last_fit - pos);
        if (!measure_only) text(font, size, x, y + used, ln, color, align);
        used += size * line_height;
        pos = last_fit;
        if (pos < str.size() && (str[pos] == ' ' || (newline && str[pos] == '\n'))) ++pos;
    }
    return used;
}

} // namespace gfx
