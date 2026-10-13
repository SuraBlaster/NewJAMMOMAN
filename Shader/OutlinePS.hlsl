#include "Model.hlsli"
#include "Outline.hlsli"

Texture2D AlbedoMap : register(t0);
SamplerState LinearSampler : register(s0);

float4 main(VS_OUT pin) : SV_TARGET
{
    float4 albedo = AlbedoMap.Sample(LinearSampler, pin.texcoord);
    clip(albedo.a - 0.5f);

    // モデルの色を暗くした線と、指定色の線を混ぜる
    float3 darkened_albedo = albedo.rgb * 0.35f;
    float3 color = lerp(cb_outline_color.rgb, darkened_albedo, cb_outline_albedo_mix);

    return float4(color, 1.0f);
}
