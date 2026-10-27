#include "ModelManager.h"
#include "EnemyWave.h"

#include <cmath>

#include "Collision.h"
#include "Player.h"
#include "EnemyVisibility.h"

namespace
{
    constexpr float ENEMY_SCALE = 0.01f;
    constexpr float DESPAWN_DISTANCE = 14.0f;
    constexpr int CONTACT_DAMAGE = 1;
    constexpr float DAMAGE_INVINCIBLE_TIME = 1.2f;
    constexpr float KNOCKBACK_HORIZONTAL = 6.0f;
    constexpr float KNOCKBACK_VERTICAL = 5.0f;

}

// コンストラクタ
EnemyWave::EnemyWave(ID3D11Device* device, Player* player) : player(player)
{
    model = ModelManager::Instance().CreateInstance(device, "Data/Model/Enemy/Swarm08.gltf");
    InitializeAnimator();

    if (animator)
    {
        animator->Update(0.0f);
    }

    scale = {ENEMY_SCALE, ENEMY_SCALE, ENEMY_SCALE};
    rotation.y = DirectX::XMConvertToRadians(90.0f);
    color = {1.0f, 0.25f, 0.25f, 1.0f};
    radius = 0.45f;
    height = 1.0f;
    SetHealth(1);
    UpdateTransform();
}

// 更新処理
void EnemyWave::Update(float elapsedTime)
{
    if (!player)
    {
        Destroy();
        return;
    }

    if (!placementInitialized)
    {

        UpdateTransform();
        placementInitialized = true;
    }

    const bool wasActive = activation.IsActive();

    if (!wasActive && !activation.UpdateVisibility(IsEnemyOnScreen(position, radius, height)))
    {

        UpdateInvincibleTimer(elapsedTime);
        return;
    }

    if (!wasActive)
    {

        targetY = position.y;
        waveTime = 0.0f;
    }

    UpdateAttacking(elapsedTime);

    CollideWithPlayer();
    UpdateInvincibleTimer(elapsedTime);

    if (animator)
    {
        animator->Update(elapsedTime);
    }

    UpdateTransform();
}

void EnemyWave::SetDirection(Direction value)
{
    direction = value;
    rotation.y = DirectX::XMConvertToRadians(direction == Direction::Right ? 90.0f : -90.0f);
}

// UpdateAttacking の更新処理
void EnemyWave::UpdateAttacking(float elapsedTime)
{
    waveTime += elapsedTime * waveSpeed;
    const float moveDirection = direction == Direction::Right ? 1.0f : -1.0f;
    position.x += moveDirection * moveSpeed * elapsedTime;
    position.y = targetY + std::sin(waveTime) * waveAmplitude;

    if (IsOutOfRange())
    {
        Destroy();
    }
}

void EnemyWave::CollideWithPlayer()
{
    DirectX::XMFLOAT3 contactPosition;

    if (!Collision::IntersectSphereVsCylinder(position, radius, player->GetPosition(), player->GetRadius(),
                                              player->GetHeight(), contactPosition))
    {
        return;
    }

    if (!player->ApplyDamage(CONTACT_DAMAGE, DAMAGE_INVINCIBLE_TIME))
    {
        return;
    }

    DirectX::XMFLOAT3 impulse = {player->GetPosition().x - position.x, 0.0f,
                                 player->GetPosition().z - position.z};
    const float length = std::sqrt(impulse.x * impulse.x + impulse.z * impulse.z);

    if (length > 0.0001f)
    {
        impulse.x = impulse.x / length * KNOCKBACK_HORIZONTAL;
        impulse.z = impulse.z / length * KNOCKBACK_HORIZONTAL;
    }
    else
    {
        impulse.x = direction == Direction::Right ? KNOCKBACK_HORIZONTAL : -KNOCKBACK_HORIZONTAL;
    }

    impulse.y = KNOCKBACK_VERTICAL;
    player->SetVelocity({0, 0, 0});
    player->AddImpulse(impulse);
}

bool EnemyWave::IsOutOfRange() const
{
    return std::abs(position.x - player->GetPosition().x) > DESPAWN_DISTANCE &&
           !IsEnemyOnScreen(position, radius, height);
}
