#ifndef STYLIZED_HLSLI
#define STYLIZED_HLSLI

// PBR.hlsli と Scene.hlsli を先にincludeしておくこと

cbuffer CbStylize : register(b1)
{
    float3 cbShadowColor; // 影側の色味（拡散色に乗算する）
    float cbStylizeBlend; // 0:元のPBR 1:スタイライズド
    float3 cbRimColor;
    float cbRimIntensity;
    float cbToonThreshold;     // 明暗の境目の位置（0〜1）
    float cbToonSoftness;      // 明暗の境目のぼかし幅
    float cbSpecularThreshold; // ハイライトが出始める位置（0〜1）
    float cbSpecularSoftness;  // ハイライトの縁のぼかし幅
    float cbSpecularIntensity;
    float cbRimThreshold; // リムライトが出始める位置（0〜1）
    float cbRimSoftness;  // リムライトの縁のぼかし幅
    float cbAmbientStrength;
};

// threshold を境に、softness の幅で 0→1 へ切り替える
float StylizedStep(float threshold, float softness, float x)
{
    return smoothstep(threshold - softness, threshold + softness, x);
}

float3 StylizedLighting(float3 albedo, float metalness, float smoothness, float3 normal, float3 viewDirection,
                        float3 lightDirection, float3 lightColor, float3 ambient, float rimWeight)
{
    // 金属ほど拡散が消えて反射が色づく
    float3 specularColor;
    float oneMinusReflectivity;
    float3 diffuseColor =
        DiffuseAndSpecularFromMetallic(albedo, metalness, specularColor, oneMinusReflectivity);

    float nl = dot(normal, lightDirection);
    float halfLambert = nl * 0.5f + 0.5f;
    float lightRamp = StylizedStep(cbToonThreshold, cbToonSoftness, halfLambert);
    float3 diffuse = diffuseColor * lerp(cbShadowColor, lightColor, lightRamp);
    diffuse += diffuseColor * ambient * cbAmbientStrength;

    // GGXの形を残しつつ縁だけくっきりさせる
    float3 halfDirection = normalize(lightDirection + viewDirection);
    float nh = saturate(dot(normal, halfDirection));
    float roughness = max(PerceptualRoughnessToRoughness(SmoothnessToPerceptualRoughness(smoothness)), 0.05f);
    float d = GGXTerm(nh, roughness) * roughness * roughness * PI; // ピークが1になるよう正規化
    float specularMask = StylizedStep(cbSpecularThreshold, cbSpecularSoftness, d) * lightRamp;
    // 非金属は白、金属は地の色のハイライトにする
    float3 highlightColor = lerp(float3(1.0f, 1.0f, 1.0f), albedo, metalness);
    float3 specular = highlightColor * lightColor * specularMask * smoothness * cbSpecularIntensity;

    // 環境マップが無いので、反射方向の上下で空と地面の色を映す
    float3 reflectDirection = reflect(-viewDirection, normal);
    float3 environment = lerp(cbGroundColor, cbSkyColor, reflectDirection.y * 0.5f + 0.5f);
    float nv = saturate(dot(normal, viewDirection));
    float grazing = saturate(smoothness + (1.0f - oneMinusReflectivity));
    float3 environmentSpecular = environment * FresnelLerp(specularColor, grazing, nv) * smoothness;

    // 平らな面は面ごと光るため、rimWeightで対象を絞る
    float rim = 1.0f - nv;
    float rimMask = StylizedStep(cbRimThreshold, cbRimSoftness, rim);
    float3 rimLight = cbRimColor * rimMask * cbRimIntensity * rimWeight;

    return diffuse + specular + environmentSpecular + rimLight;
}

#endif
