#pragma once

#include "Effect/Effect.h"

class Player;
class ShapeRenderer;

// Shared implementation for the former ProjectileSideL / ProjectileSideR.
class BossSideProjectile final
{
public:
    enum class Motion { ScatterAndAccelerate, Continuous };
    BossSideProjectile(const DirectX::XMFLOAT3& start,
        const DirectX::XMFLOAT3& scatter_target, float direction_x,
        Motion motion = Motion::ScatterAndAccelerate);
    ~BossSideProjectile();
    BossSideProjectile(const BossSideProjectile&) = delete;
    BossSideProjectile& operator=(const BossSideProjectile&) = delete;

    void Update(float elapsed_time);
    void HitPlayer(Player& player);
    void DrawDebugPrimitive(ShapeRenderer* renderer) const;
    bool IsActive() const { return active; }
    static constexpr float GetRadius() { return radius; }

private:
    enum class Phase { Scattering, Moving };
    void Destroy();

    static constexpr float scatter_speed = 20.0f;
    static constexpr float scatter_duration = 1.0f;
    static constexpr float continuous_start_delay = 0.1f;
    static constexpr float acceleration = 10.0f;
    static constexpr float max_speed = 15.0f;
    static constexpr float lifetime = 4.0f;
    static constexpr float radius = 0.4f;
    static constexpr int damage = 1;
    static constexpr float invincible_duration = 1.2f;
    static constexpr float knockback_power = 5.0f;
    static constexpr float effect_scale = 0.08f;
    static constexpr float effect_y_offset = -0.5f;

    DirectX::XMFLOAT3 position;
    DirectX::XMFLOAT3 scatter_target;
    float direction_x;
    Motion motion;
    float age = 0.0f;
    float current_speed = 0.0f;
    Phase phase = Phase::Scattering;
    bool active = true;
    Effect wind_effect{ "Data/Effect/wind.efkefc" };
    Effekseer::Handle wind_handle = -1;
};
