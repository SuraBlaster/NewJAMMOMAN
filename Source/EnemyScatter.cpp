#include "ModelManager.h"
#include "EnemyScatter.h"

#include <cmath>
#include <cstdlib>

#include "Player.h"

// コンストラクタ
EnemyScatter::EnemyScatter(ID3D11Device* device, Player* player, const DirectX::XMFLOAT3& spawnPosition)
    : player(player)
{
    model = ModelManager::Instance().CreateInstance(device, "Data/Model/Enemy/Swarm08.gltf");
    position = spawnPosition;
    scale = {0.002f, 0.002f, 0.002f};
    radius = 0.18f;
    height = 0.25f;
    health = 1;
    useGravity = false;
    color = {1.0f, 0.45f, 0.25f, 1.0f};

    scatterDuration = 0.5f + static_cast<float>(std::rand() % 100) / 100.0f;
    moveDirection = {static_cast<float>(std::rand() % 200) / 100.0f - 1.0f,
                     static_cast<float>(std::rand() % 150) / 100.0f + 0.5f, 0.0f};
    DirectX::XMVECTOR direction = DirectX::XMVector3Normalize(DirectX::XMLoadFloat3(&moveDirection));
    DirectX::XMStoreFloat3(&moveDirection, direction);
    InitializeAnimator();
    UpdateTransform();
}

// 更新処理
void EnemyScatter::Update(float elapsedTime)
{
    if (!player)
    {
        Destroy();
        return;
    }

    if (state == State::Scatter)
    {
        scatterTime += elapsedTime;

        if (scatterTime >= scatterDuration)
        {
            state = State::Charge;
            const DirectX::XMFLOAT3 target = player->GetPosition();
            DirectX::XMFLOAT3 toTarget = {target.x - position.x,
                                          target.y + player->GetHeight() * 0.5f - position.y,
                                          target.z - position.z};
            DirectX::XMStoreFloat3(&moveDirection,
                                   DirectX::XMVector3Normalize(DirectX::XMLoadFloat3(&toTarget)));
        }
    }

    const float moveScale = state == State::Scatter ? 0.5f : 1.0f;
    position.x += moveDirection.x * speed * moveScale * elapsedTime;
    position.y += moveDirection.y * speed * moveScale * elapsedTime;
    position.z += moveDirection.z * speed * moveScale * elapsedTime;

    DamagePlayerOnContact(*player, 1, 1.2f, true);
    const DirectX::XMFLOAT3 playerPosition = player->GetPosition();
    const float dx = position.x - playerPosition.x;
    const float dy = position.y - playerPosition.y;

    if (std::abs(dx) > 20.0f || std::abs(dy) > 12.0f)
    {
        Destroy();
    }

    UpdateInvincibleTimer(elapsedTime);

    if (animator)
    {
        animator->Update(elapsedTime);
    }

    UpdateTransform();
}
