#include "ModelManager.h"
#include "EnemyVacuum.h"

#include <algorithm>
#include <cmath>

#include "Player.h"
#include "CollisionManager.h"

EnemyVacuum::EnemyVacuum(ID3D11Device* device, Player* player)
	: player(player)
{
	model = ModelManager::Instance().CreateInstance(device, "Data/Model/Enemy/lab_spiked_vacuum.gltf");
	scale = { 0.5f, 0.5f, 0.5f };
	rotation.y = DirectX::XMConvertToRadians(-90.0f);
	radius = 0.6f;
	height = 0.5f;
	SetHealth(3);
	use_gravity = true;
	InitializeAnimator();
	states[static_cast<size_t>(StateId::Move)] = std::make_unique<MoveState>(this);
	states[static_cast<size_t>(StateId::Pursuit)] = std::make_unique<PursuitState>(this);
	SetState(StateId::Move);
	UpdateTransform();
}

void EnemyVacuum::Update(float elapsedTime)
{
	if (!player)
	{
		Destroy();
		return;
	}

	wall_turn_timer = (std::max)(0.0f, wall_turn_timer - elapsedTime);
	UpdateStateMachine(elapsedTime);
	UpdateWallCollision(elapsedTime);

	UpdateGroundPhysics(elapsedTime);
	DamagePlayerOnContact(*player, 1, 1.2f);
	UpdateInvincibleTimer(elapsedTime);
	if (animator) animator->Update(elapsedTime);
	UpdateTransform();
}

void EnemyVacuum::SetState(StateId stateId)
{
	if (stateId == StateId::None || stateId == StateId::EnumCount
		|| stateId == current_state) return;
	next_state = stateId;
}

void EnemyVacuum::UpdateStateMachine(float elapsedTime)
{
	// Apply transitions on the next update, as in EnemyBoss.
	if (next_state != StateId::None)
	{
		current_state = next_state;
		next_state = StateId::None;
		states[static_cast<size_t>(current_state)]->OnEnter();
	}
	if (current_state != StateId::None)
		states[static_cast<size_t>(current_state)]->OnUpdate(elapsedTime);
}

float EnemyVacuum::GetPlayerDistance() const
{
	const DirectX::XMFLOAT3 target = player->GetPosition();
	const float dx = target.x - position.x;
	const float dy = target.y - position.y;
	const float dz = target.z - position.z;
	return std::sqrt(dx * dx + dy * dy + dz * dz);
}

void EnemyVacuum::MoveState::OnEnter()
{
	owner->velocity.x = owner->move_direction * owner->move_speed;
	owner->velocity.z = 0.0f;
	owner->rotation.y = DirectX::XMConvertToRadians(owner->move_direction * 90.0f);
}

void EnemyVacuum::MoveState::OnUpdate(float elapsedTime)
{
	// Move along X in the current patrol direction.
	(void)elapsedTime;
	owner->velocity.x = owner->move_direction * owner->move_speed;
	owner->velocity.z = 0.0f;
	if (owner->wall_turn_timer <= 0.0f && owner->GetPlayerDistance() <= owner->pursuit_start_distance)
		owner->SetState(StateId::Pursuit);
}

void EnemyVacuum::PursuitState::OnEnter()
{
	owner->velocity.x = 0.0f;
	owner->velocity.z = 0.0f;
}

void EnemyVacuum::PursuitState::OnUpdate(float elapsedTime)
{
	if (owner->wall_turn_timer > 0.0f)
	{
		owner->velocity.x = owner->move_direction * owner->pursuit_speed;
		owner->velocity.z = 0.0f;
		return;
	}
	// Separate thresholds prevent rapid state changes at the detection boundary.
	if (owner->GetPlayerDistance() >= owner->pursuit_end_distance)
	{
		owner->SetState(StateId::Move);
	}
}

void EnemyVacuum::UpdateWallCollision(float elapsedTime)
{
	if (elapsedTime <= 0.0f || velocity.x == 0.0f) return;
	const AABB body = {
		{ position.x - radius, position.y + 0.01f, position.z - radius },
		{ position.x + radius, position.y + height, position.z + radius }
	};
	const float displacement = velocity.x * elapsedTime;
	TerrainSweepHit hit;
	if (!CollisionManager::Instance().CheckWall(body, displacement, hit)) return;
	// Stop at the wall this frame; move away on the next frame.
	const float direction = displacement < 0.0f ? -1.0f : 1.0f;
	const float safeDistance = (std::max)(0.0f, std::abs(displacement) * hit.time - 0.003f);
	velocity.x = direction * safeDistance / elapsedTime;
	move_direction = -direction;
	wall_turn_timer = 0.5f;
	rotation.y = DirectX::XMConvertToRadians(move_direction * 90.0f);
}
