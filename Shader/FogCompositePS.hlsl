cbuffer Fog : register(b0) { float4 parameters; float4 tint; float4 placement; };
Texture2D fogTexture : register(t0);
SamplerState linearClamp : register(s0);
struct Output { float4 color : SV_TARGET; float depth : SV_DEPTH; };
Output main(float4 position : SV_POSITION, float2 uv : TEXCOORD0)
{
    Output o;
    o.color = fogTexture.Sample(linearClamp, uv);
    o.depth = placement.w;
    return o;
}
