#include "SwordTrail.hlsli"

float Hash(float2 p)
{
    return frac(sin(dot(p, float2(127.1f, 311.7f))) * 43758.5453f);
}

float Noise(float2 p)
{
    float2 i = floor(p);
    float2 f = frac(p);
    f = f * f * (3.0f - 2.0f * f);
    return lerp(lerp(Hash(i), Hash(i + float2(1, 0)), f.x), lerp(Hash(i + float2(0, 1)), Hash(i + 1), f.x),
                f.y);
}

float4 main(VsOutput pin) : SV_TARGET
{
    float u = pin.texcoord.x;
    float age = saturate(pin.texcoord.y);

    float life = pow(1.0f - age, 1.2f);

    // 柄に近い根元は弱く、刃先に向かって強くする
    float blade = smoothstep(0.0f, 0.35f, u) * step(u, 1.0f);

    // 刃先が描く弧を、芯の通った光の線にする
    float tipDistance = abs(u - 1.0f);
    float tipCore = exp(-tipDistance * 40.0f);
    float tipGlow = exp(-tipDistance * 8.0f);
    float edgeFade = 1.0f - smoothstep(1.0f, 1.0f + cbGlowMargin, u);

    // 今の刃の位置ほど明るくする
    float leading = exp(-age * 10.0f);

    // 刃の中を流れる光のすじと、尾へ向かって走る光の脈
    float streak = Noise(float2(u * 6.0f, age * 12.0f - cbTime * 10.0f));
    float energy = lerp(0.65f, 1.35f, streak);
    float pulsePhase = frac(age * 1.6f - cbTime * 2.5f) - 0.5f;
    float pulse = exp(-pulsePhase * pulsePhase / 0.004f);

    // ブルームで光としてにじむよう、芯は1.0を超える明るさにする
    float glow =
        ((blade * 0.6f * energy + tipGlow * 0.9f + pulse * blade * 0.8f) * life + leading * blade * 0.8f) *
        edgeFade;
    float core = (tipCore * 2.5f * life + leading * tipGlow * 1.3f) * edgeFade;

    float3 tint = lerp(cbRootColor.rgb, cbGlowColor.rgb, saturate(u * 0.7f + age * 0.6f));
    float3 color = tint * glow + cbCoreColor.rgb * core;

    // 尾は常に焼け崩れていくようにし、崩れる縁を光らせる
    float dissolveNoise = Noise(float2(u * 9.0f, age * 16.0f - cbTime * 1.5f) + 17.0f);
    float threshold = smoothstep(0.45f, 1.0f, age) * 0.9f + cbFade * 1.1f;
    float dissolve = smoothstep(threshold - 0.04f, threshold + 0.04f, dissolveNoise);
    float burnEdge = saturate(1.0f - abs(dissolveNoise - threshold) / 0.05f) * step(0.001f, threshold);
    float shape = (blade + tipGlow) * edgeFade;
    color = color * dissolve + cbGlowColor.rgb * burnEdge * shape * 2.0f * (1.0f - age * 0.5f);

    // 攻撃が終わったら、光の波が尾へ走り点滅しながら消える
    if (cbFade > 0.0f)
    {
        float wave = exp(-pow((age - cbFade * 1.3f) / 0.08f, 2.0f));
        float flicker = 1.0f - cbFade * 0.35f * (0.5f + 0.5f * sin(cbTime * 80.0f));

        color *= flicker;
        color += cbCoreColor.rgb * wave * 2.5f * shape * life;
        color *= 1.0f - smoothstep(0.85f, 1.0f, cbFade);
    }

    return float4(color, 1.0f);
}
