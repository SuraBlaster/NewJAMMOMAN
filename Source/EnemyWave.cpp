#include "ModelManager.h"
#include "EnemyWave.h"

#include <cmath>

#include "Collision.h"
#include "Player.h"

namespace
{
	constexpr float EnemyScale = 0.01f;
	constexpr float DespawnDistance = 14.0f;
	constexpr int ContactDamage = 1;
	constexpr float DamageInvincibleTime = 1.2f;
	constexpr float KnockbackHorizontal = 6.0f;
	constexpr float KnockbackVertical = 5.0f;

}

EnemyWave::EnemyWave(ID3D11Device* device, Player* player)
	: player(player)
{
	model = ModelManager::Instance().CreateInstance(device, "Data/Model/Enemy/Swarm08.gltf");
	animator = std::make_unique<Animator>(model.get());
	animator->Play(0, "Idle_Seq_0", true);
	scale = { EnemyScale, EnemyScale, EnemyScale };
	rotation.y = DirectX::XMConvertToRadians(-90.0f);
	color = { 1.0f, 0.25f, 0.25f, 1.0f };
	radius = 0.45f;
	height = 1.0f;
	SetHealth(1);
	UpdateTransform();
}

void EnemyWave::Update(float elapsedTime)
{
	if (!player)
	{
		Destroy();
		return;
	}

	if (!target_y_initialized)
	{
		target_y = player->GetPosition().y;
		target_y_initialized = true;
	}

	UpdateAttacking(elapsedTime);

	CollideWithPlayer();
	UpdateInvincibleTimer(elapsedTime);
	if (animator) animator->Update(elapsedTime);
	UpdateTransform();
}

void EnemyWave::SetDirection(Direction value)
{
	direction = value;
	rotation.y = DirectX::XMConvertToRadians(
	direction == Direction::Right ? -90.0f : 90.0f);
}

void EnemyWave::UpdateAttacking(float elapsedTime)
{
	wave_time += elapsedTime * wave_speed;
	const float moveDirection = direction == Direction::Right ? 1.0f : -1.0f;
	position.x += moveDirection * move_speed * elapsedTime;
	position.y = target_y + std::sin(wave_time) * wave_amplitude;
	if (IsOutOfRange()) Destroy();
}

void EnemyWave::CollideWithPlayer()
{
	DirectX::XMFLOAT3 contactPosition;
	if (!Collision::IntersectSphereVsCylinder(
		position, radius,
		player->GetPosition(), player->GetRadius(), player->GetHeight(),
		contactPosition))
	{
		return;
	}

	if (!player->ApplyDamage(ContactDamage, DamageInvincibleTime)) return;

	DirectX::XMFLOAT3 impulse = {
		player->GetPosition().x - position.x,
		0.0f,
		player->GetPosition().z - position.z
	};
	const float length = std::sqrt(impulse.x * impulse.x + impulse.z * impulse.z);
	if (length > 0.0001f)
	{
		impulse.x = impulse.x / length * KnockbackHorizontal;
		impulse.z = impulse.z / length * KnockbackHorizontal;
	}
	else
	{
		impulse.x = direction == Direction::Right ? KnockbackHorizontal : -KnockbackHorizontal;
	}
	impulse.y = KnockbackVertical;
	player->SetVelocity({ 0, 0, 0 });
	player->AddImpulse(impulse);
}

bool EnemyWave::IsOutOfRange() const
{
	return std::abs(position.x - player->GetPosition().x) > DespawnDistance;
}
