#ifndef STYLIZED_HLSLI
#define STYLIZED_HLSLI

// PBR.hlsli と Scene.hlsli を先にincludeしておくこと

cbuffer CbStylize : register(b1)
{
    float3 cb_shadow_color;         // 影側の色味（拡散色に乗算する）
    float  cb_stylize_blend;        // 0:元のPBR 1:スタイライズド
    float3 cb_rim_color;
    float  cb_rim_intensity;
    float  cb_toon_threshold;       // 明暗の境目の位置（0～1）
    float  cb_toon_softness;        // 明暗の境目のぼかし幅
    float  cb_specular_threshold;   // ハイライトが出始める位置（0～1）
    float  cb_specular_softness;    // ハイライトの縁のぼかし幅
    float  cb_specular_intensity;
    float  cb_rim_threshold;        // リムライトが出始める位置（0～1）
    float  cb_rim_softness;         // リムライトの縁のぼかし幅
    float  cb_ambient_strength;
};

// threshold を境に、softness の幅で 0→1 へ切り替える
float StylizedStep(float threshold, float softness, float x)
{
    return smoothstep(threshold - softness, threshold + softness, x);
}

float3 StylizedLighting(
    float3 albedo,
    float metalness,
    float smoothness,
    float3 normal,
    float3 view_direction,
    float3 light_direction,
    float3 light_color,
    float3 ambient,
    float rim_weight)
{
    // 金属ほど拡散が消えて反射が色づく
    float3 specular_color;
    float one_minus_reflectivity;
    float3 diffuse_color = DiffuseAndSpecularFromMetallic(albedo, metalness, specular_color, one_minus_reflectivity);

    float nl = dot(normal, light_direction);
    float half_lambert = nl * 0.5f + 0.5f;
    float light_ramp = StylizedStep(cb_toon_threshold, cb_toon_softness, half_lambert);
    float3 diffuse = diffuse_color * lerp(cb_shadow_color, light_color, light_ramp);
    diffuse += diffuse_color * ambient * cb_ambient_strength;

    // GGXの形を残しつつ縁だけくっきりさせる
    float3 half_direction = normalize(light_direction + view_direction);
    float nh = saturate(dot(normal, half_direction));
    float roughness = max(PerceptualRoughnessToRoughness(SmoothnessToPerceptualRoughness(smoothness)), 0.05f);
    float d = GGXTerm(nh, roughness) * roughness * roughness * PI; // ピークが1になるよう正規化
    float specular_mask = StylizedStep(cb_specular_threshold, cb_specular_softness, d) * light_ramp;
    // 非金属は白、金属は地の色のハイライトにする
    float3 highlight_color = lerp(float3(1.0f, 1.0f, 1.0f), albedo, metalness);
    float3 specular = highlight_color * light_color * specular_mask * smoothness * cb_specular_intensity;

    // 環境マップが無いので、反射方向の上下で空と地面の色を映す
    float3 reflect_direction = reflect(-view_direction, normal);
    float3 environment = lerp(cb_ground_color, cb_sky_color, reflect_direction.y * 0.5f + 0.5f);
    float nv = saturate(dot(normal, view_direction));
    float grazing = saturate(smoothness + (1.0f - one_minus_reflectivity));
    float3 environment_specular = environment * FresnelLerp(specular_color, grazing, nv) * smoothness;

    // 平らな面は面ごと光るため、rim_weightで対象を絞る
    float rim = 1.0f - nv;
    float rim_mask = StylizedStep(cb_rim_threshold, cb_rim_softness, rim);
    float3 rim_light = cb_rim_color * rim_mask * cb_rim_intensity * rim_weight;

    return diffuse + specular + environment_specular + rim_light;
}

#endif
