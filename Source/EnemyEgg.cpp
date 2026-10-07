#include "ModelManager.h"
#include "EnemyEgg.h"

#include "EnemyManager.h"
#include "EnemyScatter.h"

EnemyEgg::EnemyEgg(
	ID3D11Device* device,
	Player* player,
	const DirectX::XMFLOAT3& spawnPosition)
	: device(device)
	, player(player)
{
	model = ModelManager::Instance().CreateInstance(device, "Data/Model/Enemy/egg.gltf");
	position = spawnPosition;
	scale = { 0.1f, 0.1f, 0.1f };
	radius = 0.25f;
	height = 0.4f;
	health = 1;
	use_gravity = true;
	UpdateTransform();
}

void EnemyEgg::Update(float elapsedTime)
{
	UpdateGroundPhysics(elapsedTime);
	UpdateInvincibleTimer(elapsedTime);
	UpdateTransform();
}

void EnemyEgg::OnLanding()
{
	Destroy();
	if (!device || !player) return;
	constexpr int scatterCount = 10;
	for (int i = 0; i < scatterCount; ++i)
	{
		EnemyManager::Instance().Register(
			std::make_shared<EnemyScatter>(device, player, position));
	}
}
