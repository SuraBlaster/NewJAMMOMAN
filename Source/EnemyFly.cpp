#include "ModelManager.h"
#include "EnemyFly.h"

#include <cmath>

#include "EnemyEgg.h"
#include "EnemyManager.h"
#include "Player.h"
#include "EnemyVisibility.h"

// コンストラクタ
EnemyFly::EnemyFly(ID3D11Device* device, Player* player) : device(device), player(player)
{
    model = ModelManager::Instance().CreateInstance(device, "Data/Model/Enemy/Swarm08.gltf");
    scale = {0.01f, 0.01f, 0.01f};
    rotation.y = DirectX::XMConvertToRadians(-90.0f);
    radius = 0.45f;
    height = 0.8f;
    SetHealth(2);
    useGravity = false;
    color = {0.8f, 0.85f, 1.0f, 1.0f};
    InitializeAnimator();
    UpdateTransform();
}

// 更新処理
void EnemyFly::Update(float elapsedTime)
{
    if (!player)
    {
        Destroy();
        return;
    }

    const DirectX::XMFLOAT3 playerPosition = player->GetPosition();

    if (!activation.IsActive() && !activation.UpdateVisibility(IsEnemyOnScreen(position, radius, height)))
    {
        UpdateInvincibleTimer(elapsedTime);
        UpdateTransform();
        return;
    }

    position.x -= moveSpeed * elapsedTime;

    if (!eggDropped && std::abs(position.x - playerPosition.x) < 5.0f)
    {
        EnemyManager::Instance().Register(std::make_shared<EnemyEgg>(device, player, position));
        eggDropped = true;
    }

    if ((position.x - playerPosition.x < -10.0f || position.x < -20.0f) &&
        !IsEnemyOnScreen(position, radius, height))
    {
        Destroy();
    }

    if (DamagePlayerOnContact(*player, 1, 1.2f, true))
    {
        const float direction = playerPosition.x < position.x ? -1.0f : 1.0f;
        player->AddImpulse({direction * 6.0f, 5.0f, 0.0f});
    }

    UpdateInvincibleTimer(elapsedTime);

    if (animator)
    {
        animator->Update(elapsedTime);
    }

    UpdateTransform();
}
