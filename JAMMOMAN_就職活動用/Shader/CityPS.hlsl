cbuffer City : register(b0)
{
    float4 settings;
    float4 view;
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

float3 Buildings(float2 p, float layer, float3 behind)
{
    float spacing = 5.0 + layer * 2.0;
    float worldX = p.x * spacing + view.x * (0.35 + layer * 0.22);
    float cell = floor(worldX), local = frac(worldX);
    float seed = Hash(float2(cell, layer * 17.0));
    float base = view.z - layer * 0.085 + view.y;
    float height = (0.19 + seed * 0.38) * (1.0 - layer * 0.08) * view.w;
    float roof = base + height;
    float width = 0.62 + Hash(float2(cell, layer + 39)) * 0.25;
    float inside = step(abs(local - 0.5), width * 0.5) * step(p.y, roof);
    float3 facade = lerp(float3(0.065, 0.105, 0.17), float3(0.017, 0.032, 0.057), layer / 2.0);
    facade *= 0.80 + seed * 0.35;

    float tower = step(abs(local - 0.5), 0.045) * step(p.y, roof + 0.06) * step(roof, p.y) * step(0.72, seed);
    behind = lerp(behind, facade, max(inside, tower));
    float2 grid = float2(local * 11.0, (p.y - base) * 66.0);
    float2 windowCell = floor(grid), windowUV = frac(grid);
    float lit = step(0.62, Hash(float2(cell * 19 + windowCell.x, windowCell.y + layer * 31)));
    float mask =
        step(0.24, windowUV.x) * step(windowUV.x, 0.64) * step(0.23, windowUV.y) * step(windowUV.y, 0.60);
    mask *= inside * step(base + 0.035, p.y) * step(p.y, roof - 0.025);
    float3 light = lerp(float3(0.15, 0.40, 0.52), float3(0.76, 0.40, 0.12), step(0.65, seed));
    behind += light * mask * lit * settings.w * (0.35 + layer * 0.16);
    float beacon = step(abs(local - 0.5), 0.027) * step(abs(p.y - roof - 0.061), 0.0035) * step(0.72, seed);
    behind += float3(0.7, 0.10, 0.035) * beacon * (0.55 + 0.45 * sin(settings.x * 2.0 + cell)) * settings.w;
    return behind;
}

float4 main(float4 position : SV_POSITION, float2 uv : TEXCOORD0) : SV_TARGET
{
    float2 p = float2(uv.x * settings.y, 1 - uv.y);
    float3 color = lerp(float3(0.11, 0.17, 0.23), float3(0.014, 0.025, 0.060), smoothstep(0.1, 1, p.y));
    float clouds = Noise(float2(p.x * 3 + settings.x * 0.008 + view.x * 0.1, p.y * 9));
    clouds += Noise(float2(p.x * 7 - settings.x * 0.006, p.y * 17)) * 0.35;
    color += float3(0.025, 0.038, 0.050) * smoothstep(0.35, 0.9, clouds) * smoothstep(0.35, 0.95, p.y);
    float2 starGrid = float2(p.x * 150 + view.x, p.y * 150);
    float star = step(0.994, Hash(floor(starGrid))) * pow(saturate(1 - length(frac(starGrid) - 0.5) * 3), 3) *
                 step(0.70, p.y);
    color += float3(0.18, 0.25, 0.30) * star;
    [unroll] for (int layer = 0; layer < 3; layer++) color = Buildings(p, float(layer), color);

    color = lerp(color, float3(0.07, 0.13, 0.18), 0.35 * (1 - smoothstep(0, 0.38, p.y)));
    return float4(color * settings.z, 1);
}
