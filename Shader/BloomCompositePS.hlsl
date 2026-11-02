#include "Bloom.hlsli"

Texture2D bloomTexture : register(t1);

float4 main(VsOutput pin) : SV_TARGET
{
    float3 scene = sourceTexture.Sample(linearSampler, pin.texcoord).rgb;
    float3 bloom = bloomTexture.Sample(linearSampler, pin.texcoord).rgb;

    // これまでの見た目を変えないよう、トーンマップはせずに1.0で切る
    return float4(saturate(scene + bloom * cbIntensity), 1.0f);
}
