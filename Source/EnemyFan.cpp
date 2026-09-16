#include "EnemyFan.h"

#include <cmath>

#include "Player.h"

EnemyFan::EnemyFan(ID3D11Device* device, Player* player)
	: player(player)
{
	model = std::make_shared<Model>(device, "Data/Model/Enemy/Fan.gltf");
	rotation.y = DirectX::XMConvertToRadians(-90.0f);
	radius = 0.55f;
	height = 1.0f;
	SetHealth(3);
	use_gravity = true;
	InitializeAnimator();
	UpdateTransform();
}

void EnemyFan::Update(float elapsedTime)
{
	if (!player)
	{
		Destroy();
		return;
	}

	const DirectX::XMFLOAT3 target = player->GetPosition();
	const float dx = target.x - position.x;
	const float dz = target.z - position.z;
	const float distance = std::sqrt(dx * dx + dz * dz);
	if (distance <= 3.0f && target.x < position.x)
	{
		const float strength = (1.0f - distance / 3.0f) * 12.0f;
		player->AddImpulse({ -strength * elapsedTime, 0.0f, 0.0f });
	}

	DamagePlayerOnContact(*player, 1, 1.2f);
	UpdateGroundPhysics(elapsedTime);
	UpdateInvincibleTimer(elapsedTime);
	if (animator) animator->Update(elapsedTime);
	UpdateTransform();
}
