#include "Bloom.hlsli"

float4 main(VsOutput pin) : SV_TARGET
{
    // 縮小しながら読むので、4点の平均でちらつきを抑える
    float2 offset = cbTexelSize * 0.5f;
    float3 color = sourceTexture.Sample(linearSampler, pin.texcoord + float2(-offset.x, -offset.y)).rgb;
    color += sourceTexture.Sample(linearSampler, pin.texcoord + float2(offset.x, -offset.y)).rgb;
    color += sourceTexture.Sample(linearSampler, pin.texcoord + float2(-offset.x, offset.y)).rgb;
    color += sourceTexture.Sample(linearSampler, pin.texcoord + float2(offset.x, offset.y)).rgb;
    color *= 0.25f;

    // しきい値で切ると光の縁が硬くなるため、knee の幅でなめらかにつなぐ
    float brightness = max(color.r, max(color.g, color.b));
    float soft = clamp(brightness - cbThreshold + cbKnee, 0.0f, 2.0f * cbKnee);
    soft = soft * soft / (4.0f * cbKnee + 0.00001f);
    float contribution = max(soft, brightness - cbThreshold) / max(brightness, 0.00001f);

    return float4(color * contribution, 1.0f);
}
