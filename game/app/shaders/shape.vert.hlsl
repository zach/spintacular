// The game's one vertex shader: 2D positions in pixels, scaled into clip space.
cbuffer Frame : register(b0, space1) {
    float4 scale_offset; // xy: scale, zw: offset
};

struct Input {
    float2 position : TEXCOORD0;
    float2 uv : TEXCOORD1;
    float4 color : TEXCOORD2;
};

struct Output {
    float4 position : SV_Position;
    float2 uv : TEXCOORD0;
    float4 color : TEXCOORD1;
};

Output main(Input input) {
    Output output;
    output.position = float4(input.position * scale_offset.xy + scale_offset.zw, 0.0, 1.0);
    output.uv = input.uv;
    output.color = input.color;
    return output;
}
