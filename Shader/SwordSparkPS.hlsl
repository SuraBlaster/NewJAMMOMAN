#include "SwordSpark.hlsli"

float4 main(SparkVsOutput pin) : SV_TARGET
{
    float2 p = pin.texcoord * 2.0f - 1.0f;
    float distanceSq = dot(p, p);

    // 白い芯と緑のにじみを重ねた丸い光
    float core = exp(-distanceSq * 10.0f);
    float halo = exp(-distanceSq * 3.0f);
    float3 color = (cbCoreColor.rgb * core * 2.5f + cbGlowColor.rgb * halo * 1.2f) * pin.alpha;

    return float4(color, 1.0f);
}
