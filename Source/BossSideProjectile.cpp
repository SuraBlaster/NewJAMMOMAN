#include "BossSideProjectile.h"

#include <algorithm>
#include <cmath>
#include "Collision.h"
#include "Player.h"
#include "ShapeRenderer.h"

// コンストラクタ
BossSideProjectile::BossSideProjectile(const DirectX::XMFLOAT3& start, const DirectX::XMFLOAT3& scatterTarget,
                                       float directionX, Motion motion)
    : position(motion == Motion::Continuous ? scatterTarget : start), scatterTarget(scatterTarget),
      directionX(directionX < 0.0f ? -1.0f : 1.0f), motion(motion)
{
    windHandle = windEffect.Play({position.x, position.y + EFFECT_Y_OFFSET, position.z}, EFFECT_SCALE);
}

// デストラクタ
BossSideProjectile::~BossSideProjectile()
{
    Destroy();
}

void BossSideProjectile::Destroy()
{
    active = false;

    if (windHandle >= 0)
    {
        windEffect.Stop(windHandle);
        windHandle = -1;
    }
}

// 更新処理
void BossSideProjectile::Update(float elapsedTime)
{
    if (!active || elapsedTime <= 0.0f)
    {
        return;
    }

    float remainingTime = (std::min)(elapsedTime, LIFETIME - age);

    if (phase == Phase::Scattering)
    {
        const float duration = motion == Motion::Continuous ? CONTINUOUS_START_DELAY : SCATTER_DURATION;
        const float step = (std::min)(remainingTime, duration - age);
        const float dx = scatterTarget.x - position.x;
        const float dy = scatterTarget.y - position.y;
        const float dz = scatterTarget.z - position.z;
        const float distance = std::sqrt(dx * dx + dy * dy + dz * dz);
        const float movement = SCATTER_SPEED * step;

        if (distance <= movement)
        {
            position = scatterTarget;
        }
        else
        {
            position.x += dx / distance * movement;
            position.y += dy / distance * movement;
            position.z += dz / distance * movement;
        }

        age += step;
        remainingTime -= step;

        if (age >= duration)
        {
            phase = Phase::Moving;

            if (motion == Motion::Continuous)
            {
                currentSpeed = MAX_SPEED;
            }
        }
    }

    if (phase == Phase::Moving && remainingTime > 0.0f)
    {

        const float acceleratingTime = (std::min)(remainingTime, (MAX_SPEED - currentSpeed) / ACCELERATION);
        const float distance = currentSpeed * acceleratingTime +
                               0.5f * ACCELERATION * acceleratingTime * acceleratingTime +
                               MAX_SPEED * (remainingTime - acceleratingTime);
        currentSpeed = (std::min)(MAX_SPEED, currentSpeed + ACCELERATION * acceleratingTime);
        position.x += directionX * distance;
        age += remainingTime;
    }

    if (age >= LIFETIME)
    {
        Destroy();
        return;
    }

    windEffect.SetPosition(windHandle, {position.x, position.y + EFFECT_Y_OFFSET, position.z});
}

void BossSideProjectile::HitPlayer(Player& player)
{
    if (!active || player.GetInvincibleTime() > 0.0f)
    {
        return;
    }

    DirectX::XMFLOAT3 contact;

    if (!Collision::IntersectSphereVsCylinder(position, RADIUS, player.GetPosition(), player.GetRadius(),
                                              player.GetHeight(), contact))
    {
        return;
    }

    if (!player.ApplyDamage(DAMAGE, INVINCIBLE_DURATION))
    {
        return;
    }

    const auto& playerPosition = player.GetPosition();
    const float dx = playerPosition.x - position.x;
    const float dy = playerPosition.y - position.y;
    const float dz = playerPosition.z - position.z;
    const float distance = std::sqrt(dx * dx + dy * dy + dz * dz);
    const float pushX = distance > 0.0f ? dx / distance : directionX;
    player.AddImpulse({pushX * KNOCKBACK_POWER, 0.0f, 0.0f});
    Destroy();
}

// 当たり判定などのデバッグ描画処理
void BossSideProjectile::DrawDebugPrimitive(ShapeRenderer* renderer) const
{
    if (active && renderer)
    {
        renderer->DrawSphere(position, RADIUS, {1.0f, 0.2f, 0.8f, 1.0f});
    }
}
