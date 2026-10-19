#include "Model.hlsli"
#include "Outline.hlsli"

Texture2D albedoMap : register(t0);
SamplerState linearSampler : register(s0);

float4 main(VsOutput pin) : SV_TARGET
{
    float4 albedo = albedoMap.Sample(linearSampler, pin.texcoord);
    clip(albedo.a - 0.5f);

    float3 darkenedAlbedo = albedo.rgb * 0.35f;
    float3 color = lerp(cbOutlineColor.rgb, darkenedAlbedo, cbOutlineAlbedoMix);

    return float4(color, 1.0f);
}
