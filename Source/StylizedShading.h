#pragma once

#include <DirectXMath.h>

// スタイライズドPBR（トゥーン寄り）の調整値を管理する
class StylizedShading
{
private:
    StylizedShading() = default;
    ~StylizedShading() = default;

public:
    // Shader/Stylized.hlsli の CbStylize と同じ並びにすること
    struct Settings
    {
        DirectX::XMFLOAT3 shadowColor = {0.55f, 0.55f, 0.75f};
        float stylizeBlend = 1.0f;
        DirectX::XMFLOAT3 rimColor = {0.6f, 0.8f, 1.0f};
        float rimIntensity = 0.5f;
        float toonThreshold = 0.5f;
        float toonSoftness = 0.03f;
        float specularThreshold = 0.5f;
        float specularSoftness = 0.05f;
        float specularIntensity = 1.0f;
        float rimThreshold = 0.6f;
        float rimSoftness = 0.05f;
        float ambientStrength = 0.5f;
        float tintCompensation = 0.0f; // 敵の色を掛けて暗くなる分を補う量（0:そのまま掛ける）
        float ghostCenterAlpha = 0.6f; // 残像の中心の濃さ（縁に対する割合）
        float ghostColorize = 0.0f;    // 残像を指定色で染める強さ（0:色を掛けるだけ）
        float gltfMetalRoughness = 0.0f; // 1でglTFの並び（G:ラフネス B:メタルネス）として読む
    };

    static_assert(sizeof(Settings) % 16 == 0, "constant buffer size must be a multiple of 16");

    // 輪郭線（背面法）の調整値
    struct OutlineSettings
    {
        bool enabled = true;
        bool drawOnStatic = false; // ボーンを持たないモデル（ステージ等）にも描くか
        float width = 2.0f;        // 線の太さ（ピクセル）
        DirectX::XMFLOAT3 color = {0.08f, 0.06f, 0.1f};
        float albedoMix = 0.5f; // 0:線の色そのまま 1:モデルの色を暗くした色
    };

    // ダッシュの残像の濃さ
    struct AfterimageSettings
    {
        float newestAlpha = 1.0f;
        float oldestAlpha = 0.3f;
    };

    static StylizedShading& Instance()
    {
        static StylizedShading instance;
        return instance;
    }

    const Settings& GetSettings() const
    {
        return settings;
    }

    // ボーンを持たないモデル（ステージ等）にかけるリムライトの倍率
    float GetStaticRimScale() const
    {
        return staticRimScale;
    }

    const OutlineSettings& GetOutlineSettings() const
    {
        return outlineSettings;
    }

    const AfterimageSettings& GetAfterimageSettings() const
    {
        return afterimageSettings;
    }

    // 調整用GUI
    void DrawGUI();

private:
    Settings settings;
    float staticRimScale = 0.0f;
    OutlineSettings outlineSettings;
    AfterimageSettings afterimageSettings;
};
