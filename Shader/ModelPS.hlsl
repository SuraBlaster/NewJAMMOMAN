#include "Model.hlsli"
#include "PBR.hlsli"
#include "Stylized.hlsli"

cbuffer CbMesh : register(b0)
{
    float4 cbMaterialColor;
    float3 cbEmissive;
    float cbMetalness;
    float cbSmoothness;
    float cbRimWeight;
    float2 cbPadding;
};

cbuffer CbTransparency : register(b9)
{
    float4 cbTransparencyColor;
    float cbTransparencyAlpha;
    float3 cbTransparencyPadding;
};

Texture2D albedoMap : register(t0);
Texture2D normalMap : register(t1);
Texture2D metalnessSmoothnessMap : register(t2);
Texture2D emissiveMap : register(t3);
SamplerState linearSampler : register(s0);

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
    // 元の読み方（R:メタルネス A:スムースネス）とglTFの並び（G:ラフネス B:メタルネス）を見比べられるようにしている
    float4 metalnessSmoothness = metalnessSmoothnessMap.Sample(linearSampler, pin.texcoord);
    float metalness = lerp(metalnessSmoothness.r, metalnessSmoothness.b, cbGltfMetalRoughness) * cbMetalness;
    float smoothness = lerp(metalnessSmoothness.a * cbSmoothness,
                            1.0f - metalnessSmoothness.g * (1.0f - cbSmoothness), cbGltfMetalRoughness);

    float3 viewDirection = normalize(cbCameraPosition.xyz - pin.position);

    // 半球ライト
    float hemisphereLightFactor = dot(normal, float3(0, 1, 0)) * 0.5f + 0.5f;
    float3 ambient = lerp(cbGroundColor.rgb, cbSkyColor.rgb, hemisphereLightFactor) * cbHemisphereWeight;
    // ディレクショナルライト
    float3 lightDirection = -cbLightDirection.xyz;
    float3 pbrColor = BRDF(albedo.rgb, metalness, smoothness, normal, viewDirection, lightDirection,
                           cbLightColor.rgb, ambient);
    float3 stylizedColor = StylizedLighting(albedo.rgb, metalness, smoothness, normal, viewDirection,
                                            lightDirection, cbLightColor.rgb, ambient, cbRimWeight);
    float3 color = lerp(pbrColor, stylizedColor, cbStylizeBlend);

    // エミッシブ
    float3 emissive = emissiveMap.Sample(linearSampler, pin.texcoord).rgb;
    color.rgb += emissive * cbEmissive;

    float3 tint = cbTransparencyColor.rgb;
    float tintGain = max(tint.r, max(tint.g, tint.b));
    float3 tintHue = tint / max(tintGain, 0.0001f);
    float alpha = finalAlpha;

    // 色をそのまま掛けると彩度の高い色ほど暗くなるため、その分を補えるようにしておく
    float tintLuminance = dot(tintHue, float3(0.299f, 0.587f, 0.114f));
    float3 compensatedHue = tintHue / lerp(1.0f, max(tintLuminance, 0.0001f), cbTintCompensation);
    float3 tintedColor = color.rgb * tintGain * compensatedHue;

    if (cbTransparencyAlpha < 0.999f)
    {
        // 残像などの半透明は、指定色に染めたり中心を薄くしたりして実体感を減らせるようにする
        float luminance = dot(color.rgb, float3(0.299f, 0.587f, 0.114f));
        float3 colorizedColor = tintHue * tintGain * (0.35f + luminance);
        tintedColor = lerp(tintedColor, colorizedColor, cbGhostColorize);
        float rim = 1.0f - saturate(dot(normal, viewDirection));
        alpha *= lerp(cbGhostCenterAlpha, 1.0f, rim);
    }

    color.rgb = tintedColor;

    return float4(color, alpha);
}
