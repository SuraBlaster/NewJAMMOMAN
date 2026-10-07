#include "ModelManager.h"
#include "EnemyScatter.h"

#include <cmath>
#include <cstdlib>

#include "Player.h"

EnemyScatter::EnemyScatter(
	ID3D11Device* device,
	Player* player,
	const DirectX::XMFLOAT3& spawnPosition)
	: player(player)
{
	model = ModelManager::Instance().CreateInstance(device, "Data/Model/Enemy/Swarm08.gltf");
	position = spawnPosition;
	scale = { 0.002f, 0.002f, 0.002f };
	radius = 0.18f;
	height = 0.25f;
	health = 1;
	use_gravity = false;
	color = { 1.0f, 0.45f, 0.25f, 1.0f };

	scatter_duration = 0.5f + static_cast<float>(std::rand() % 100) / 100.0f;
	move_direction = {
		static_cast<float>(std::rand() % 200) / 100.0f - 1.0f,
		static_cast<float>(std::rand() % 150) / 100.0f + 0.5f,
		0.0f
	};
	DirectX::XMVECTOR direction = DirectX::XMVector3Normalize(
		DirectX::XMLoadFloat3(&move_direction));
	DirectX::XMStoreFloat3(&move_direction, direction);
	InitializeAnimator();
	UpdateTransform();
}

void EnemyScatter::Update(float elapsedTime)
{
	if (!player)
	{
		Destroy();
		return;
	}

	if (state == State::Scatter)
	{
		scatter_time += elapsedTime;
		if (scatter_time >= scatter_duration)
		{
			state = State::Charge;
			const DirectX::XMFLOAT3 target = player->GetPosition();
			DirectX::XMFLOAT3 toTarget = {
				target.x - position.x,
				target.y + player->GetHeight() * 0.5f - position.y,
				target.z - position.z
			};
			DirectX::XMStoreFloat3(&move_direction,
				DirectX::XMVector3Normalize(DirectX::XMLoadFloat3(&toTarget)));
		}
	}

	const float moveScale = state == State::Scatter ? 0.5f : 1.0f;
	position.x += move_direction.x * speed * moveScale * elapsedTime;
	position.y += move_direction.y * speed * moveScale * elapsedTime;
	position.z += move_direction.z * speed * moveScale * elapsedTime;

	DamagePlayerOnContact(*player, 1, 1.2f, true);
	const DirectX::XMFLOAT3 playerPosition = player->GetPosition();
	const float dx = position.x - playerPosition.x;
	const float dy = position.y - playerPosition.y;
	if (std::abs(dx) > 20.0f || std::abs(dy) > 12.0f) Destroy();

	UpdateInvincibleTimer(elapsedTime);
	if (animator) animator->Update(elapsedTime);
	UpdateTransform();
}
