// The game's one fragment shader: vertex color times the atlas. Flat shapes
// sample the atlas's white texel; text samples glyph coverage in alpha.
Texture2D atlas : register(t0, space2);
SamplerState atlas_sampler : register(s0, space2);

float4 main(float2 uv : TEXCOORD0, float4 color : TEXCOORD1) : SV_Target0 {
    return color * atlas.Sample(atlas_sampler, uv);
}
