#include "ModelManager.h"
#include "EnemyVacuum.h"

#include <algorithm>
#include <cmath>

#include "Player.h"
#include "CollisionManager.h"

// コンストラクタ
EnemyVacuum::EnemyVacuum(ID3D11Device* device, Player* player) : player(player)
{
    model = ModelManager::Instance().CreateInstance(device, "Data/Model/Enemy/lab_spiked_vacuum.gltf");
    scale = {0.75f, 0.75f, 0.75f};
    rotation.y = DirectX::XMConvertToRadians(-90.0f);
    radius = 0.6f;
    height = 0.5f;
    SetHealth(3);
    useGravity = true;
    InitializeAnimator();
    states[static_cast<size_t>(StateId::Move)] = std::make_unique<MoveState>(this);
    states[static_cast<size_t>(StateId::Pursuit)] = std::make_unique<PursuitState>(this);
    SetState(StateId::Move);
    UpdateTransform();
}

// 更新処理
void EnemyVacuum::Update(float elapsedTime)
{
    if (!player)
    {
        Destroy();
        return;
    }

    wallTurnTimer = (std::max)(0.0f, wallTurnTimer - elapsedTime);
    UpdateStateMachine(elapsedTime);
    UpdateWallCollision(elapsedTime);

    UpdateGroundPhysics(elapsedTime);
    DamagePlayerOnContact(*player, 1, 1.2f);
    UpdateInvincibleTimer(elapsedTime);

    if (animator)
    {
        animator->Update(elapsedTime);
    }

    UpdateTransform();
}

void EnemyVacuum::SetState(StateId stateId)
{
    if (stateId == StateId::None || stateId == StateId::EnumCount || stateId == currentState)
    {
        return;
    }

    nextState = stateId;
}

// 状態遷移と現在の状態の更新処理
void EnemyVacuum::UpdateStateMachine(float elapsedTime)
{

    if (nextState != StateId::None)
    {
        currentState = nextState;
        nextState = StateId::None;
        states[static_cast<size_t>(currentState)]->OnEnter();
    }

    if (currentState != StateId::None)
    {
        states[static_cast<size_t>(currentState)]->OnUpdate(elapsedTime);
    }
}

float EnemyVacuum::GetPlayerDistance() const
{
    const DirectX::XMFLOAT3 target = player->GetPosition();
    const float dx = target.x - position.x;
    const float dy = target.y - position.y;
    const float dz = target.z - position.z;
    return std::sqrt(dx * dx + dy * dy + dz * dz);
}

// 状態に入るときの処理
void EnemyVacuum::MoveState::OnEnter()
{
    owner->velocity.x = owner->moveDirection * owner->moveSpeed;
    owner->velocity.z = 0.0f;
    owner->rotation.y = DirectX::XMConvertToRadians(owner->moveDirection * 90.0f);
}

// 現在の状態の更新処理
void EnemyVacuum::MoveState::OnUpdate(float elapsedTime)
{
    owner->rotation.y += NORMAL_ROTATION_SPEED * elapsedTime;
    owner->velocity.x = owner->moveDirection * owner->moveSpeed;
    owner->velocity.z = 0.0f;

    if (owner->wallTurnTimer <= 0.0f && owner->GetPlayerDistance() <= owner->pursuitStartDistance)
    {
        owner->SetState(StateId::Pursuit);
    }
}

// 状態に入るときの処理
void EnemyVacuum::PursuitState::OnEnter()
{
    owner->velocity.x = owner->moveDirection * owner->pursuitSpeed;
    owner->velocity.z = 0.0f;
}

// 現在の状態の更新処理
void EnemyVacuum::PursuitState::OnUpdate(float elapsedTime)
{
    owner->rotation.y += PURSUIT_ROTATION_SPEED * elapsedTime;

    owner->velocity.x = owner->moveDirection * owner->pursuitSpeed;
    owner->velocity.z = 0.0f;

    if (owner->GetPlayerDistance() >= owner->pursuitEndDistance)
    {
        owner->SetState(StateId::Move);
    }
}

// UpdateWallCollision の更新処理
void EnemyVacuum::UpdateWallCollision(float elapsedTime)
{
    if (elapsedTime <= 0.0f || velocity.x == 0.0f)
    {
        return;
    }

    const AABB body = {{position.x - radius, position.y + 0.01f, position.z - radius},
                       {position.x + radius, position.y + height, position.z + radius}};
    const float displacement = velocity.x * elapsedTime;
    TerrainSweepHit hit;

    if (!CollisionManager::Instance().CheckWall(body, displacement, hit))
    {
        return;
    }

    const float direction = displacement < 0.0f ? -1.0f : 1.0f;
    const float safeDistance = (std::max)(0.0f, std::abs(displacement) * hit.time - 0.003f);
    velocity.x = direction * safeDistance / elapsedTime;
    moveDirection = -direction;
    wallTurnTimer = 0.5f;
    rotation.y = DirectX::XMConvertToRadians(moveDirection * 90.0f);
}
