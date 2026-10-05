#include "BossSideProjectile.h"

#include <algorithm>
#include <cmath>
#include "Collision.h"
#include "Player.h"
#include "ShapeRenderer.h"

BossSideProjectile::BossSideProjectile(const DirectX::XMFLOAT3& start,
    const DirectX::XMFLOAT3& scatter_target, float direction_x, Motion motion)
    : position(motion == Motion::Continuous ? scatter_target : start), scatter_target(scatter_target),
      direction_x(direction_x < 0.0f ? -1.0f : 1.0f), motion(motion)
{
    wind_handle = wind_effect.Play(
        { position.x, position.y + effect_y_offset, position.z }, effect_scale);
}

BossSideProjectile::~BossSideProjectile()
{
    Destroy();
}

void BossSideProjectile::Destroy()
{
    active = false;
    if (wind_handle >= 0)
    {
        wind_effect.Stop(wind_handle);
        wind_handle = -1;
    }
}

void BossSideProjectile::Update(float elapsed_time)
{
    if (!active || elapsed_time <= 0.0f) return;
    float remaining_time = (std::min)(elapsed_time, lifetime - age);
    if (phase == Phase::Scattering)
    {
        const float duration = motion == Motion::Continuous ? continuous_start_delay : scatter_duration;
        const float step = (std::min)(remaining_time, duration - age);
        const float dx = scatter_target.x - position.x;
        const float dy = scatter_target.y - position.y;
        const float dz = scatter_target.z - position.z;
        const float distance = std::sqrt(dx * dx + dy * dy + dz * dz);
        const float movement = scatter_speed * step;
        if (distance <= movement)
            position = scatter_target;
        else
        {
            position.x += dx / distance * movement;
            position.y += dy / distance * movement;
            position.z += dz / distance * movement;
        }
        age += step;
        remaining_time -= step;
        if (age >= duration)
        {
            phase = Phase::Moving;
            if (motion == Motion::Continuous) current_speed = max_speed;
        }
    }
    if (phase == Phase::Moving && remaining_time > 0.0f)
    {
        // Integrate acceleration and the speed cap independently of frame rate.
        const float accelerating_time = (std::min)(remaining_time,
            (max_speed - current_speed) / acceleration);
        const float distance = current_speed * accelerating_time
            + 0.5f * acceleration * accelerating_time * accelerating_time
            + max_speed * (remaining_time - accelerating_time);
        current_speed = (std::min)(max_speed,
            current_speed + acceleration * accelerating_time);
        position.x += direction_x * distance;
        age += remaining_time;
    }
    if (age >= lifetime)
    {
        Destroy();
        return;
    }
    wind_effect.SetPosition(wind_handle,
        { position.x, position.y + effect_y_offset, position.z });
}

void BossSideProjectile::HitPlayer(Player& player)
{
    if (!active || player.GetInvincibleTime() > 0.0f) return;
    DirectX::XMFLOAT3 contact;
    if (!Collision::IntersectSphereVsCylinder(position, radius,
        player.GetPosition(), player.GetRadius(), player.GetHeight(), contact)) return;
    if (!player.ApplyDamage(damage, invincible_duration)) return;

    const auto& player_position = player.GetPosition();
    const float dx = player_position.x - position.x;
    const float dy = player_position.y - position.y;
    const float dz = player_position.z - position.z;
    const float distance = std::sqrt(dx * dx + dy * dy + dz * dz);
    const float push_x = distance > 0.0f ? dx / distance : direction_x;
    player.AddImpulse({ push_x * knockback_power, 0.0f, 0.0f });
    Destroy();
}

void BossSideProjectile::DrawDebugPrimitive(ShapeRenderer* renderer) const
{
    if (active && renderer)
        renderer->DrawSphere(position, radius, { 1.0f, 0.2f, 0.8f, 1.0f });
}
