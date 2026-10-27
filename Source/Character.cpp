#include "Character.h"

#include <algorithm>
#include "CollisionManager.h"

// 変換行列の更新処理
void Character::UpdateTransform()
{
    const DirectX::XMMATRIX scaling = DirectX::XMMatrixScaling(scale.x, scale.y, scale.z);
    const DirectX::XMMATRIX rotationMatrix =
        DirectX::XMMatrixRotationRollPitchYaw(rotation.x, rotation.y, rotation.z);
    const DirectX::XMMATRIX translation = DirectX::XMMatrixTranslation(position.x, position.y, position.z);
    DirectX::XMStoreFloat4x4(&transform, scaling * rotationMatrix * translation);

    if (model)
    {
        model->UpdateTransform(transform);
    }
}

bool Character::ApplyDamage(int damage, float invincibleDuration)
{
    if (damage <= 0 || IsDead() || invincibleTime > 0.0f)
    {
        return false;
    }

    health = (std::max)(0, health - damage);
    invincibleTime = (std::max)(0.0f, invincibleDuration);

    if (IsDead())
    {
        OnDead();
    }
    else
    {
        OnDamaged();
    }

    return true;
}

void Character::AddImpulse(const DirectX::XMFLOAT3& impulse)
{
    velocity.x += impulse.x;
    velocity.y += impulse.y;
    velocity.z += impulse.z;
}

// 無敵時間の更新処理
void Character::UpdateInvincibleTimer(float elapsedTime)
{
    invincibleTime = (std::max)(0.0f, invincibleTime - elapsedTime);
}

// UpdateGroundPhysics の更新処理
void Character::UpdateGroundPhysics(float elapsedTime)
{
    position.x += velocity.x * elapsedTime;
    position.z += velocity.z * elapsedTime;

    if (!useGravity)
    {
        position.y += velocity.y * elapsedTime;
        return;
    }

    if (!groundPlacementChecked)
    {
        float repairedY = position.y;

        if (CollisionManager::Instance().ResolveTerrainPlacement(position.x, position.y, position.z,
                                                                 repairedY))
        {
            position.y = repairedY;
        }

        groundPlacementChecked = true;
    }

    velocity.y -= gravityAcceleration * elapsedTime;
    const float moveY = velocity.y * elapsedTime;

    if (moveY >= 0.0f)
    {
        position.y += moveY;
        isGround = false;
        return;
    }

    const DirectX::XMFLOAT3 start = {position.x, position.y + height, position.z};
    const DirectX::XMFLOAT3 end = {position.x, position.y + moveY, position.z};
    HitResult hit;
    auto& collisions = CollisionManager::Instance();
    float floorY = 0.0f;

    const bool terrainHit =
        collisions.FindTerrainFloor(position.x, position.z, position.y + moveY, position.y + 0.01f, floorY);
    const bool meshHit = collisions.Raycast(start, end, hit);

    if (terrainHit || meshHit)
    {
        const bool landedThisFrame = !isGround;
        position.y = meshHit && (!terrainHit || hit.position.y > floorY) ? hit.position.y : floorY;
        velocity.y = 0.0f;
        velocity.x = 0.0f;
        velocity.z = 0.0f;
        isGround = true;

        if (landedThisFrame)
        {
            OnLanding();
        }
    }
    else
    {
        position.y += moveY;
        isGround = false;
    }
}

// InitializeAnimator の初期化処理
void Character::InitializeAnimator(int initialClip, bool loop)
{
    if (!model || model->GetAnimations().empty())
    {
        return;
    }

    animator = std::make_unique<Animator>(model.get());
    PlayAnimation(initialClip, loop, 0.0f);
}

void Character::PlayAnimation(int clip, bool loop, float blendDuration)
{
    if (!animator || !model || clip < 0 || static_cast<size_t>(clip) >= model->GetAnimations().size())
    {
        return;
    }

    animator->Play(0, clip, loop, blendDuration);
}

void Character::SetHealth(int value)
{
    maxHealth = (std::max)(1, value);
    health = maxHealth;
}
