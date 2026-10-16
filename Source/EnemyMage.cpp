#include "ModelManager.h"
#include "EnemyMage.h"

#include <algorithm>
#include <cmath>

#include "Player.h"
#include "EnemyVisibility.h"
#include "Effect/Effect.h"

// コンストラクタ
EnemyMage::EnemyMage(ID3D11Device* device, Player* player) : player(player)
{
    model = ModelManager::Instance().CreateInstance(device, "Data/Model/Enemy/Wizard.gltf");
    scale = {0.01f, 0.01f, 0.01f};
    rotation.y = DirectX::XMConvertToRadians(-90.0f);
    radius = 0.45f;
    height = 1.2f;
    health = 3;
    useGravity = true;
    color = {0.65f, 0.35f, 1.0f, 1.0f};

    InitializeAnimator();
    UpdateTransform();
}

// 更新処理
void EnemyMage::Update(float elapsedTime)
{
    if (!player)
    {
        Destroy();
        return;
    }

    if (!projectileEffect)
    {
        projectileEffect = std::make_shared<Effect>("Data/Effect/ThunderShot.efkefc");
    }

    attackTimer -= elapsedTime;
    const DirectX::XMFLOAT3 target = player->GetPosition();
    const float dx = target.x - position.x;

    if (std::abs(dx) > 0.01f)
    {
        SetDirection(dx);
    }

    // 攻撃してくる範囲
    if (attackTimer <= 0.0f && IsEnemyOnScreen(position, radius, height))
    {
        FireProjectile();
        attackTimer = 3.0f;
        PlayAnimation(1, false);
    }

    DamagePlayerOnContact(*player, 1, 1.2f);
    UpdateProjectiles(elapsedTime);
    UpdateGroundPhysics(elapsedTime);
    UpdateInvincibleTimer(elapsedTime);

    if (animator)
    {
        animator->Update(elapsedTime);
    }

    UpdateTransform();
}

void EnemyMage::SetDirection(float direction)
{
    facingDirection = direction < 0.0f ? -1.0f : 1.0f;
    rotation.y = DirectX::XMConvertToRadians(facingDirection * 90.0f);
}

void EnemyMage::FireProjectile()
{
    if (!terrainImpactEffect)
    {
        terrainImpactEffect = std::make_shared<Effect>("Data/Effect/Fire.efkefc");
    }

    const DirectX::XMFLOAT3 start = {position.x, position.y + height * 0.7f, position.z};
    const DirectX::XMFLOAT3 direction = {facingDirection * 7.0f, 0.0f, 0.0f};

    projectiles.emplace_back(start, direction, 0.2f, 5.0f, 1);
    projectiles.back().AttachEffect(projectileEffect, 0.5f);
    projectiles.back().EnableTerrainCollision(terrainImpactEffect);
}

// UpdateProjectiles の更新処理
void EnemyMage::UpdateProjectiles(float elapsedTime)
{
    for (auto& projectile : projectiles)
    {
        projectile.Update(elapsedTime);
        projectile.HitPlayer(*player, 1.0f);
    }

    projectiles.erase(std::remove_if(projectiles.begin(), projectiles.end(),
                                     [](const EnemyProjectile& projectile)
                                     { return !projectile.IsActive(); }),
                      projectiles.end());
}

// DrawPrimitive の描画処理
void EnemyMage::DrawPrimitive(ShapeRenderer* shapeRenderer) const
{
    for (const auto& projectile : projectiles)
    {
        projectile.DrawDebugPrimitive(shapeRenderer);
    }
}
