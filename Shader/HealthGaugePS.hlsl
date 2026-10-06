#include "Sprite.hlsli"

Texture2D spriteTexture : register(t0);
SamplerState spriteSampler : register(s0);
cbuffer HealthGauge : register(b1)
{
    float4 maskBounds;
    float4 fillColor;
    float4 health;
};

float4 main(VS_OUT pin) : SV_TARGET
{
    float4 source = spriteTexture.Sample(spriteSampler, pin.texcoord);
    // Combine a spatial mask with the colored interior of the supplied artwork.
    // Silver borders, transparent pixels and the lower emblem remain untouched.
    bool inside = all(pin.texcoord >= maskBounds.xy) && all(pin.texcoord <= maskBounds.zw);
    float strongest = max(source.r, source.g);
    bool colored = strongest > source.b * 1.5 && abs(source.r - source.g) > 0.06;
    if (inside && colored)
    {
        float cutoff = lerp(maskBounds.w, maskBounds.y, saturate(health.x));
        bool filled = health.x > 0 && pin.texcoord.y >= cutoff;
        float trailCutoff = lerp(maskBounds.w, maskBounds.y, saturate(health.y));
        bool damage = health.y > 0 && pin.texcoord.y >= trailCutoff;
        float shading = 0.65 + 0.35 * strongest;
        source.rgb = filled ? fillColor.rgb * shading
            : damage ? float3(1.0, 0.12, 0.12) * shading : float3(0.055, 0.065, 0.075);
    }
    return source * pin.color;
}
