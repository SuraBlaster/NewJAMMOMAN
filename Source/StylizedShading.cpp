#include <imgui.h>
#include "StylizedShading.h"

// 調整用GUI
void StylizedShading::DrawGUI()
{
    if (ImGui::Begin("Stylized Shading"))
    {
        ImGui::SliderFloat(u8"PBR⇔スタイライズド", &settings.stylizeBlend, 0.0f, 1.0f);

        if (ImGui::CollapsingHeader(u8"陰影（トゥーン）", ImGuiTreeNodeFlags_DefaultOpen))
        {
            ImGui::SliderFloat(u8"境目の位置", &settings.toonThreshold, 0.0f, 1.0f);
            ImGui::SliderFloat(u8"境目のぼかし", &settings.toonSoftness, 0.001f, 0.5f);
            ImGui::ColorEdit3(u8"影の色", &settings.shadowColor.x);
            ImGui::SliderFloat(u8"環境光の強さ", &settings.ambientStrength, 0.0f, 2.0f);
        }

        if (ImGui::CollapsingHeader(u8"ハイライト", ImGuiTreeNodeFlags_DefaultOpen))
        {
            ImGui::SliderFloat(u8"出始める位置##specular", &settings.specularThreshold, 0.0f, 1.0f);
            ImGui::SliderFloat(u8"縁のぼかし##specular", &settings.specularSoftness, 0.001f, 0.5f);
            ImGui::SliderFloat(u8"強さ##specular", &settings.specularIntensity, 0.0f, 3.0f);
        }

        if (ImGui::CollapsingHeader(u8"リムライト", ImGuiTreeNodeFlags_DefaultOpen))
        {
            ImGui::SliderFloat(u8"出始める位置##rim", &settings.rimThreshold, 0.0f, 1.0f);
            ImGui::SliderFloat(u8"縁のぼかし##rim", &settings.rimSoftness, 0.001f, 0.5f);
            ImGui::SliderFloat(u8"強さ##rim", &settings.rimIntensity, 0.0f, 3.0f);
            ImGui::ColorEdit3(u8"色##rim", &settings.rimColor.x);
            ImGui::SliderFloat(u8"ステージ等への強さ##rim", &staticRimScale, 0.0f, 1.0f);
        }

        if (ImGui::CollapsingHeader(u8"輪郭線", ImGuiTreeNodeFlags_DefaultOpen))
        {
            ImGui::Checkbox(u8"表示##outline", &outlineSettings.enabled);
            ImGui::SliderFloat(u8"太さ（ピクセル）##outline", &outlineSettings.width, 0.0f, 8.0f);
            ImGui::ColorEdit3(u8"色##outline", &outlineSettings.color.x);
            ImGui::SliderFloat(u8"モデルの色を混ぜる##outline", &outlineSettings.albedoMix, 0.0f, 1.0f);
            ImGui::Checkbox(u8"ステージ等にも描く##outline", &outlineSettings.drawOnStatic);
        }

        if (ImGui::CollapsingHeader(u8"敵の色", ImGuiTreeNodeFlags_DefaultOpen))
        {
            ImGui::SliderFloat(u8"暗くなる分の補正", &settings.tintCompensation, 0.0f, 1.0f);

            bool gltfMetalRoughness = settings.gltfMetalRoughness > 0.5f;
            if (ImGui::Checkbox(u8"金属感をglTFの並びで読む", &gltfMetalRoughness))
            {
                settings.gltfMetalRoughness = gltfMetalRoughness ? 1.0f : 0.0f;
            }
        }

        if (ImGui::CollapsingHeader(u8"残像", ImGuiTreeNodeFlags_DefaultOpen))
        {
            ImGui::SliderFloat(u8"一番新しい残像の濃さ", &afterimageSettings.newestAlpha, 0.0f, 1.0f);
            ImGui::SliderFloat(u8"一番古い残像の濃さ", &afterimageSettings.oldestAlpha, 0.0f, 1.0f);
            ImGui::SliderFloat(u8"中心の濃さ", &settings.ghostCenterAlpha, 0.0f, 1.0f);
            ImGui::SliderFloat(u8"指定色で染める強さ", &settings.ghostColorize, 0.0f, 1.0f);
        }

        if (ImGui::Button(u8"初期値に戻す"))
        {
            settings = Settings();
            staticRimScale = 0.0f;
            outlineSettings = OutlineSettings();
            afterimageSettings = AfterimageSettings();
        }
    }
    ImGui::End();
}
