#include "EnemyWalker.h"

#include <algorithm>
#include <cmath>

#include "CollisionManager.h"
#include "EnemyWalkerPatrol.h"
#include "ModelManager.h"
#include "Player.h"
#include "EnemyVisibility.h"

// コンストラクタ
EnemyWalker::EnemyWalker(ID3D11Device* device, Player* player) : player(player)
{

    model = ModelManager::Instance().CreateInstance(device, "Data/Model/Enemy/Walker/robot.gltf", 24.0f);

    scale = {0.15f, 0.15f, 0.15f};
    radius = 0.35f;
    height = 1.2f;

    SetHealth(1);
    SetDirection(-1.0f);
    useGravity = true;

    InitializeAnimator(model->GetAnimationIndex("iddle"));

    if (animator)
    {
        animator->Update(0.0f);
    }

    UpdateTransform();
}

// 更新処理
void EnemyWalker::Update(float elapsedTime)
{
    if (!player)
    {
        Destroy();
        return;
    }

    const float safeElapsedTime = (std::max)(0.0f, elapsedTime);

    if (!placementInitialized)
    {

        UpdateGroundPhysics(0.0f);
        UpdateTransform();
        placementInitialized = true;
    }

    if (!activation.IsActive() && !activation.UpdateVisibility(IsEnemyOnScreen(position, radius, height)))
    {
        UpdateInvincibleTimer(safeElapsedTime);
        return;
    }

    if (!walkingAnimationStarted)
    {
        PlayAnimation(model->GetAnimationIndex("walking"), true, 0.1f);
        walkingAnimationStarted = true;
    }

    float remainingTime = safeElapsedTime;

    while (remainingTime > 0.0f)
    {
        const float stepTime = (std::min)(remainingTime, 1.0f / 60.0f);

        UpdatePatrol(stepTime);
        UpdateGroundPhysics(stepTime);

        remainingTime -= stepTime;
    }

    DamagePlayerOnContact(*player, 1, 1.2f);
    UpdateInvincibleTimer(safeElapsedTime);

    if (animator)
    {
        animator->Update(safeElapsedTime);
    }

    UpdateTransform();
}

void EnemyWalker::SetDirection(float direction)
{
    moveDirection = direction < 0.0f ? -1.0f : 1.0f;
    rotation.y = DirectX::XMConvertToRadians(moveDirection * 90.0f);
}

// UpdatePatrol の更新処理
void EnemyWalker::UpdatePatrol(float elapsedTime)
{
    const float displacement = moveDirection * moveSpeed * elapsedTime;

    if (EnemyWalkerPatrol::ShouldTurn(position, radius, height, moveDirection, displacement))
    {
        SetDirection(-moveDirection);
        velocity.x = 0.0f;
        return;
    }

    velocity.x = moveDirection * moveSpeed;
    velocity.z = 0.0f;
}

void EnemyWalker::OnDead()
{
    Destroy();
}
