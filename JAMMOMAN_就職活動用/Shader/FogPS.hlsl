cbuffer Fog : register(b0)
{
    float4 parameters;
    float4 tint;
    float4 placement;
    float4x4 inverseViewProjection;
};

float Hash(float2 p)
{
    return frac(sin(dot(p, float2(127.1, 311.7))) * 43758.5453);
}

float Noise(float2 p)
{
    float2 i = floor(p), f = frac(p);
    f = f * f * (3 - 2 * f);
    return lerp(lerp(Hash(i), Hash(i + float2(1, 0)), f.x), lerp(Hash(i + float2(0, 1)), Hash(i + 1), f.x),
                f.y);
}

float Fbm(float2 p)
{
    float value = 0, weight = 0.5;
    [unroll] for (int i = 0; i < 4; ++i)
    {
        value += Noise(p) * weight;
        p = float2(p.x * 1.6 - p.y * 1.2, p.x * 1.2 + p.y * 1.6) + 7.3;
        weight *= 0.5;
    }

    return value;
}

float4 main(float4 position : SV_POSITION, float2 uv : TEXCOORD0) : SV_TARGET
{
    float t = parameters.x * parameters.w;

    float4 world = mul(float4(uv * float2(2, -2) + float2(-1, 1), placement.w, 1), inverseViewProjection);
    float2 worldPosition = world.xy / world.w;
    float2 p = worldPosition * float2(0.018, -0.018) * placement.z;
    p += float2(-t, t * 0.12);
    float warp = Noise(p * 0.65 + float2(0, -t * 0.2));
    float mist = smoothstep(0.23, 0.78, Fbm(p + warp * 1.8));

    float lowerMist = 1.0 - smoothstep(-15.0, 15.0, worldPosition.y);
    float alpha = parameters.z * mist * lerp(0.45, 1.0, lowerMist);
    return float4(tint.rgb, alpha);
}
