#include "PBR.hlsli"
#include "Model.hlsli"

cbuffer CbMesh : register(b0)
{
    float3 cb_emissive;
    float  cb_metalness;
    float  cb_smoothness;
    float3 cb_padding;
};

cbuffer CbTransparency : register(b9)
{
    float4 cb_transparency_color;
    float  cb_transparency_alpha;
    float3 cb_transparency_padding;
};

Texture2D AlbedoMap : register(t0);
Texture2D NormalMap : register(t1);
Texture2D MetalnessSmoothnessMap : register(t2);
Texture2D EmissiveMap : register(t3);
SamplerState LinearSampler	: register(s0);

float4 main(VS_OUT pin) : SV_TARGET
{
    float4 albedo = AlbedoMap.Sample(LinearSampler, pin.texcoord);

    float finalAlpha = albedo.a * cb_transparency_color.a * cb_transparency_alpha;
    clip(albedo.a - 0.01f);

    // 法線算出
    float sigma = pin.tangent.w;
    float3 n = normalize(pin.normal);
    float3 t = normalize(pin.tangent.xyz);
    t = normalize(t - dot(n, t));
    float3 b = normalize(cross(n, t) * sigma);
    float3 tangent_normal = NormalMap.Sample(LinearSampler, pin.texcoord).xyz * 2.0f - 1.0f;

    float3 normal = normalize(tangent_normal.x * t + tangent_normal.y * b + tangent_normal.z * n);

    // なぜかnanになる場合がある。tangent が不正なデータの場合がある？
    if (isnan(normal.x) || isnan(normal.y) || isnan(normal.z))
    {
        normal = float3(0, 1, 0);
    }


    // PBRパラメータ取得
    float4 metalness_smoothness = MetalnessSmoothnessMap.Sample(LinearSampler, pin.texcoord);
    float metalness = metalness_smoothness.r * cb_metalness;
    float smoothness = metalness_smoothness.a * cb_smoothness;
    
    float3 view_direction = normalize(cb_camera_position.xyz - pin.position);

    // 半球ライト
    float hemisphere_light_factor = dot(normal, float3(0, 1, 0)) * 0.5f + 0.5f;
    float3 ambient = lerp(cb_ground_color.rgb, cb_sky_color.rgb, hemisphere_light_factor) * cb_hemisphere_weight;
    // ディレクショナルライト
    float3 color = BRDF(albedo.rgb, metalness, smoothness, normal, view_direction, -cb_light_direction.xyz, cb_light_color.rgb, ambient);

    
    // エミッシブ
    float3 emissive = EmissiveMap.Sample(LinearSampler, pin.texcoord).rgb;
    color.rgb += emissive * cb_emissive;

    color.rgb *= cb_transparency_color.rgb;
       
    return float4(color, albedo.a);
}
