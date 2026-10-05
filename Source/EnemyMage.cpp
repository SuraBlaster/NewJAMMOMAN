#include "ModelManager.h"
#include "EnemyMage.h"

#include <algorithm>
#include <cmath>

#include "Player.h"

EnemyMage::EnemyMage(ID3D11Device* device, Player* player)
	: player(player)
{
	model = ModelManager::Instance().CreateInstance(device, "Data/Model/Enemy/Wizard.gltf");
	scale = { 0.01f, 0.01f, 0.01f };
	rotation.y = DirectX::XMConvertToRadians(-90.0f);
	radius = 0.45f;
	height = 1.2f;
	SetHealth(3);
	use_gravity = true;
	color = { 0.65f, 0.35f, 1.0f, 1.0f };
	InitializeAnimator();
	UpdateTransform();
}

void EnemyMage::Update(float elapsedTime)
{
	if (!player)
	{
		Destroy();
		return;
	}

	attack_timer -= elapsedTime;
	const DirectX::XMFLOAT3 target = player->GetPosition();
	const float dx = target.x - position.x;
	const float dy = target.y - position.y;
	if (attack_timer <= 0.0f && std::abs(dx) < 8.0f && std::abs(dy) < 5.0f)
	{
		FireProjectile();
		attack_timer = 3.0f;
		PlayAnimation(1, false);
	}

	DamagePlayerOnContact(*player, 1, 1.2f);
	UpdateProjectiles(elapsedTime);
	UpdateGroundPhysics(elapsedTime);
	UpdateInvincibleTimer(elapsedTime);
	if (animator) animator->Update(elapsedTime);
	UpdateTransform();
}

void EnemyMage::FireProjectile()
{
	const DirectX::XMFLOAT3 start = { position.x, position.y + height * 0.7f, position.z };
	DirectX::XMFLOAT3 direction = {
		player->GetPosition().x - start.x,
		player->GetPosition().y + player->GetHeight() * 0.5f - start.y,
		player->GetPosition().z - start.z
	};
	DirectX::XMVECTOR vector = DirectX::XMVector3Normalize(DirectX::XMLoadFloat3(&direction));
	DirectX::XMStoreFloat3(&direction, DirectX::XMVectorScale(vector, 7.0f));
	projectiles.emplace_back(start, direction, 0.2f, 5.0f, 1);
}

void EnemyMage::UpdateProjectiles(float elapsedTime)
{
	for (auto& projectile : projectiles)
	{
		projectile.Update(elapsedTime);
		projectile.HitPlayer(*player, 1.0f);
	}
	projectiles.erase(
		std::remove_if(projectiles.begin(), projectiles.end(),
			[](const EnemyProjectile& projectile) { return !projectile.IsActive(); }),
		projectiles.end());
}

void EnemyMage::DrawPrimitive(ShapeRenderer* shapeRenderer) const
{
	for (const auto& projectile : projectiles) projectile.DrawDebugPrimitive(shapeRenderer);
}
