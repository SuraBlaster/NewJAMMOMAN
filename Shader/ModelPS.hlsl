#include "PBR.hlsli"
#include "Model.hlsli"

cbuffer CbMesh : register(b0)
{
    float4 cbMaterialColor;
    float3 cbEmissive;
    float  cbMetalness;
    float  cbSmoothness;
    float3 cbPadding;
};

cbuffer CbTransparency : register(b9)
{
    float4 cbTransparencyColor;
    float  cbTransparencyAlpha;
    float3 cbTransparencyPadding;
};

Texture2D albedoMap : register(t0);
Texture2D normalMap : register(t1);
Texture2D metalnessSmoothnessMap : register(t2);
Texture2D emissiveMap : register(t3);
SamplerState linearSampler	: register(s0);

float4 main(VsOutput pin) : SV_TARGET
{
    float4 albedo = albedoMap.Sample(linearSampler, pin.texcoord) * cbMaterialColor;

    float finalAlpha = albedo.a * cbTransparencyColor.a * cbTransparencyAlpha;
    clip(albedo.a - 0.01f);

    // 法線算出
    float sigma = pin.tangent.w;
    float3 n = normalize(pin.normal);
    float3 t = normalize(pin.tangent.xyz);
    t = normalize(t - dot(n, t));
    float3 b = normalize(cross(n, t) * sigma);
    float3 tangentNormal = normalMap.Sample(linearSampler, pin.texcoord).xyz * 2.0f - 1.0f;

    float3 normal = normalize(tangentNormal.x * t + tangentNormal.y * b + tangentNormal.z * n);

    // なぜかnanになる場合がある。tangent が不正なデータの場合がある？
    if (isnan(normal.x) || isnan(normal.y) || isnan(normal.z))
    {
        normal = float3(0, 1, 0);
    }


    // PBRパラメータ取得
    float4 metalnessSmoothness = metalnessSmoothnessMap.Sample(linearSampler, pin.texcoord);
    float metalness = metalnessSmoothness.r * cbMetalness;
    float smoothness = metalnessSmoothness.a * cbSmoothness;
    
    float3 viewDirection = normalize(cbCameraPosition.xyz - pin.position);

    // 半球ライト
    float hemisphereLightFactor = dot(normal, float3(0, 1, 0)) * 0.5f + 0.5f;
    float3 ambient = lerp(cbGroundColor.rgb, cbSkyColor.rgb, hemisphereLightFactor) * cbHemisphereWeight;
    // ディレクショナルライト
    float3 color = BRDF(albedo.rgb, metalness, smoothness, normal, viewDirection, -cbLightDirection.xyz, cbLightColor.rgb, ambient);

    
    // エミッシブ
    float3 emissive = emissiveMap.Sample(linearSampler, pin.texcoord).rgb;
    color.rgb += emissive * cbEmissive;

    color.rgb *= cbTransparencyColor.rgb;
       
    return float4(color, albedo.a);
}
