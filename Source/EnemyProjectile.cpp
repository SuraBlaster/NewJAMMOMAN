#include "EnemyProjectile.h"

#include "Collision.h"
#include "CollisionManager.h"
#include "Player.h"
#include "ShapeRenderer.h"
#include "Effect/Effect.h"

#include <algorithm>

struct EnemyProjectile::VisualEffect
{
    std::shared_ptr<Effect> effect;
    Effekseer::Handle handle = -1;
    float scale = 1.0f;

    // デストラクタ
    ~VisualEffect()
    {
        if (effect && handle >= 0)
        {
            effect->Stop(handle);
        }
    }
};

// コンストラクタ
EnemyProjectile::EnemyProjectile(const DirectX::XMFLOAT3& position, const DirectX::XMFLOAT3& velocity,
                                 float radius, float lifetime, int damage)
    : position(position), velocity(velocity), radius(radius), lifetime(lifetime), damage(damage)
{
}

// デストラクタ
EnemyProjectile::~EnemyProjectile() = default;

// コンストラクタ
EnemyProjectile::EnemyProjectile(EnemyProjectile&& other) noexcept = default;

EnemyProjectile& EnemyProjectile::operator=(EnemyProjectile&& other) noexcept = default;

void EnemyProjectile::AttachEffect(const std::shared_ptr<Effect>& effect, float scale)
{
    if (!active || !effect)
    {
        return;
    }

    visualEffect = std::make_unique<VisualEffect>();
    visualEffect->effect = effect;
    visualEffect->scale = scale;
    visualEffect->handle = effect->Play(position, scale);
}

void EnemyProjectile::Destroy()
{
    active = false;
    visualEffect.reset();
}

void EnemyProjectile::EnableTerrainCollision(const std::shared_ptr<Effect>& effect, float scale)
{
    collideWithTerrain = true;
    terrainImpactEffect = effect;
    terrainImpactScale = scale;
}

// 地形に接触した位置で弾を消し、撃破エフェクトを再生
bool EnemyProjectile::HitTerrain(const DirectX::XMFLOAT3& displacement)
{
    const AABB body = {{position.x - radius, position.y - radius, position.z - radius},
                       {position.x + radius, position.y + radius, position.z + radius}};

    TerrainSweepHit hit;
    const auto status = CollisionManager::Instance().SweepTerrain(body, displacement, hit);

    if (status == SweepStatus::NoHit)
    {
        return false;
    }

    const float hitTime = status == SweepStatus::InitialOverlap ? 0.0f : hit.time;
    position.x += displacement.x * hitTime;
    position.y += displacement.y * hitTime;
    position.z += displacement.z * hitTime;

    Destroy();

    if (terrainImpactEffect)
    {
        terrainImpactEffect->Play(position, terrainImpactScale);
    }

    return true;
}

// 更新処理
void EnemyProjectile::Update(float elapsedTime)
{
    if (!active)
    {
        return;
    }

    const float safeElapsedTime = (std::max)(elapsedTime, 0.0f);
    const float travelTime = (std::min)(safeElapsedTime, (std::max)(lifetime, 0.0f));
    const DirectX::XMFLOAT3 displacement = {velocity.x * travelTime, velocity.y * travelTime,
                                            velocity.z * travelTime};

    if (collideWithTerrain && HitTerrain(displacement))
    {
        return;
    }

    position.x += displacement.x;
    position.y += displacement.y;
    position.z += displacement.z;
    lifetime -= safeElapsedTime;

    if (lifetime <= 0.0f)
    {
        Destroy();
        return;
    }

    if (visualEffect && visualEffect->handle >= 0)
    {
        if (!visualEffect->effect->IsPlaying(visualEffect->handle))
        {
            visualEffect->handle = visualEffect->effect->Play(position, visualEffect->scale);
        }

        visualEffect->effect->SetPosition(visualEffect->handle, position);
    }
}

bool EnemyProjectile::HitPlayer(Player& player, float invincibleTime)
{
    if (!active)
    {
        return false;
    }

    DirectX::XMFLOAT3 contact;

    if (!Collision::IntersectSphereVsCylinder(position, radius, player.GetPosition(), player.GetRadius(),
                                              player.GetHeight(), contact))
    {
        return false;
    }

    player.ApplyDamage(damage, invincibleTime);
    Destroy();
    return true;
}

// 当たり判定などのデバッグ描画処理
void EnemyProjectile::DrawDebugPrimitive(ShapeRenderer* shapeRenderer) const
{
    if (active && shapeRenderer && !visualEffect)
    {
        shapeRenderer->DrawSphere(position, radius, {1.0f, 0.2f, 0.8f, 1.0f});
    }
}
