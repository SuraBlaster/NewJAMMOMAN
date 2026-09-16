#include "EnemyFly.h"

#include <cmath>

#include "EnemyEgg.h"
#include "EnemyManager.h"
#include "Player.h"

EnemyFly::EnemyFly(ID3D11Device* device, Player* player)
	: device(device)
	, player(player)
{
	model = std::make_shared<Model>(device, "Data/Model/Enemy/Swarm08.gltf");
	scale = { 0.01f, 0.01f, 0.01f };
	rotation.y = DirectX::XMConvertToRadians(-90.0f);
	radius = 0.45f;
	height = 0.8f;
	SetHealth(2);
	use_gravity = false;
	color = { 0.8f, 0.85f, 1.0f, 1.0f };
	InitializeAnimator();
	UpdateTransform();
}

void EnemyFly::Update(float elapsedTime)
{
	if (!player)
	{
		Destroy();
		return;
	}

	const DirectX::XMFLOAT3 playerPosition = player->GetPosition();
	if (position.x - playerPosition.x < 12.0f)
		position.x -= move_speed * elapsedTime;

	if (!egg_dropped && std::abs(position.x - playerPosition.x) < 5.0f)
	{
		EnemyManager::Instance().Register(
			std::make_shared<EnemyEgg>(device, player, position));
		egg_dropped = true;
	}

	if (position.x - playerPosition.x < -10.0f || position.x < -20.0f) Destroy();
	if (DamagePlayerOnContact(*player, 1, 1.2f, true))
	{
		const float direction = playerPosition.x < position.x ? -1.0f : 1.0f;
		player->AddImpulse({ direction * 6.0f, 5.0f, 0.0f });
	}

	UpdateInvincibleTimer(elapsedTime);
	if (animator) animator->Update(elapsedTime);
	UpdateTransform();
}
