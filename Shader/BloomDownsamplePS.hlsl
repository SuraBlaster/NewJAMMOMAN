#include "Bloom.hlsli"

float4 main(VsOutput pin) : SV_TARGET
{
    float2 offset = cbTexelSize;
    float3 color = sourceTexture.Sample(linearSampler, pin.texcoord).rgb * 4.0f;
    color += sourceTexture.Sample(linearSampler, pin.texcoord + float2(-offset.x, -offset.y)).rgb;
    color += sourceTexture.Sample(linearSampler, pin.texcoord + float2(offset.x, -offset.y)).rgb;
    color += sourceTexture.Sample(linearSampler, pin.texcoord + float2(-offset.x, offset.y)).rgb;
    color += sourceTexture.Sample(linearSampler, pin.texcoord + float2(offset.x, offset.y)).rgb;

    return float4(color / 8.0f, 1.0f);
}
