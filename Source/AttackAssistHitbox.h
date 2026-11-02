#pragma once

#include <optional>
#include <algorithm>
#include "Collision.h"

namespace AttackAssist
{
struct Settings
{
    float forwardNear;
    float forwardFar;
    float bottom;
    float top;
    float halfDepth;
    float activeStart;
    float activeEnd;
};

// プレイヤー原点基準のゲーム内距離。有効時間は各モーションの進行率。
// まずは3段の基本リーチを統一し、見た目の剣判定を前方だけ補助する。
inline constexpr Settings COMBO_SETTINGS[] = {
    {0.15f, 1.80f, 0.10f, 1.50f, 0.65f, 0.08f, 0.18f},
    {0.15f, 1.80f, 0.10f, 1.50f, 0.65f, 0.08f, 0.18f},
    {0.15f, 1.80f, 0.10f, 1.50f, 0.65f, 0.08f, 0.18f},
};

inline std::optional<AABB> Build(const DirectX::XMFLOAT3& position, float facingX,
                                  int comboStep, float progress)
{
    if (comboStep < 1 || comboStep > 3)
    {
        return std::nullopt;
    }
    const Settings& settings = COMBO_SETTINGS[comboStep - 1];
    if (progress < settings.activeStart || progress >= settings.activeEnd)
    {
        return std::nullopt;
    }
    const bool right = facingX >= 0.0f;
    return AABB{
        {position.x + (right ? settings.forwardNear : -settings.forwardFar),
         position.y + settings.bottom, position.z - settings.halfDepth},
        {position.x + (right ? settings.forwardFar : -settings.forwardNear),
         position.y + settings.top, position.z + settings.halfDepth}};
}

// 敵の直立円柱と箱を交差判定。円柱を箱に置き換えず、XZの角では距離を使う。
inline bool IntersectsCylinder(const AABB& box, const DirectX::XMFLOAT3& position,
                                float radius, float height)
{
    if (position.y > box.max.y || position.y + height < box.min.y)
    {
        return false;
    }
    const float dx = position.x - std::clamp(position.x, box.min.x, box.max.x);
    const float dz = position.z - std::clamp(position.z, box.min.z, box.max.z);
    return dx * dx + dz * dz <= radius * radius;
}
}
