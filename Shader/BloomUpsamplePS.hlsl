#include "Bloom.hlsli"

float4 main(VsOutput pin) : SV_TARGET
{
    // 周囲8点を重み付けして読み、拡大時のブロック感を消す
    float2 offset = cbTexelSize;
    float3 color = sourceTexture.Sample(linearSampler, pin.texcoord + float2(-offset.x * 2.0f, 0.0f)).rgb;
    color += sourceTexture.Sample(linearSampler, pin.texcoord + float2(offset.x * 2.0f, 0.0f)).rgb;
    color += sourceTexture.Sample(linearSampler, pin.texcoord + float2(0.0f, -offset.y * 2.0f)).rgb;
    color += sourceTexture.Sample(linearSampler, pin.texcoord + float2(0.0f, offset.y * 2.0f)).rgb;
    color += sourceTexture.Sample(linearSampler, pin.texcoord + float2(-offset.x, -offset.y)).rgb * 2.0f;
    color += sourceTexture.Sample(linearSampler, pin.texcoord + float2(offset.x, -offset.y)).rgb * 2.0f;
    color += sourceTexture.Sample(linearSampler, pin.texcoord + float2(-offset.x, offset.y)).rgb * 2.0f;
    color += sourceTexture.Sample(linearSampler, pin.texcoord + float2(offset.x, offset.y)).rgb * 2.0f;

    return float4(color / 12.0f, 1.0f);
}
