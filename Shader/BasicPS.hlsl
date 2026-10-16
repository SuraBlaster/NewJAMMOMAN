#include "Basic.hlsli"

cbuffer CbMesh : register(b0){

                 };

cbuffer CbTransparency : register(b9)
{
    float4 cbTransparencyColor;
    float cbTransparencyAlpha;
    float3 cbTransparencyPadding;
};

Texture2D diffuseMap : register(t0);
SamplerState linearSampler : register(s0);

float4 main(VsOutput pin) : SV_TARGET
{
    float4 color = diffuseMap.Sample(linearSampler, pin.texcoord);

    color.rgb *= cbTransparencyColor.rgb;
    color.a *= cbTransparencyColor.a * cbTransparencyAlpha;

    return color;
}
