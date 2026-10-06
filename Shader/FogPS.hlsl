// Original procedural effect; no third-party ShaderToy code or textures.
cbuffer Fog : register(b0)
{
    float4 parameters; // time, aspect, density, speed
    float4 tint;
    float4 placement; // camera offset XY, scale, composite depth
};
float hash(float2 p) { return frac(sin(dot(p, float2(127.1, 311.7))) * 43758.5453); }
float noise(float2 p)
{
    float2 i = floor(p), f = frac(p);
    f = f * f * (3 - 2 * f);
    return lerp(lerp(hash(i), hash(i + float2(1, 0)), f.x),
        lerp(hash(i + float2(0, 1)), hash(i + 1), f.x), f.y);
}
float fbm(float2 p)
{
    float value = 0, weight = 0.5;
    [unroll] for (int i = 0; i < 4; ++i)
    {
        value += noise(p) * weight;
        p = float2(p.x * 1.6 - p.y * 1.2, p.x * 1.2 + p.y * 1.6) + 7.3;
        weight *= 0.5;
    }
    return value;
}
float4 main(float4 position : SV_POSITION, float2 uv : TEXCOORD0) : SV_TARGET
{
    float t = parameters.x * parameters.w;
    float2 p = (uv * float2(parameters.y, 1) + placement.xy) * placement.z;
    p += float2(-t, t * 0.12);
    float warp = noise(p * 0.65 + float2(0, -t * 0.2));
    float mist = smoothstep(0.23, 0.78, fbm(p + warp * 1.8));
    // More mist below, but no hard edge when the camera moves vertically.
    float alpha = parameters.z * mist * lerp(0.45, 1.0, uv.y);
    return float4(tint.rgb, alpha);
}
