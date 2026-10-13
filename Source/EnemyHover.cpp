#include "ModelManager.h"
#include "EnemyHover.h"

#include <algorithm>
#include <cmath>

#include "Player.h"

EnemyHover::EnemyHover(ID3D11Device* device, Player* player)
	: player(player)
{
	model = ModelManager::Instance().CreateInstance(device, "Data/Model/Enemy/FlyEnemy.gltf");
	scale = { 0.01f, 0.01f, 0.01f };
	rotation.y = DirectX::XMConvertToRadians(20.0f);
	radius = 0.5f;
	height = 0.8f;
	health = 3;
	useGravity = false;
	color = { 0.35f, 0.8f, 1.0f, 1.0f };
	InitializeAnimator();
	UpdateTransform();
}

void EnemyHover::Update(float elapsedTime)
{
	if (!player)
	{
		Destroy();
		return;
	}

	if (baseY == 0.0f) baseY = position.y;
	hoverTime += elapsedTime;
	position.y = baseY + std::sin(hoverTime * 2.0f) * 0.25f;
	attackTimer -= elapsedTime;
	const DirectX::XMFLOAT3 target = player->GetPosition();
	if (attackTimer <= 0.0f
		&& std::abs(position.x - target.x) < 7.0f
		&& std::abs(position.y - target.y) < 5.0f)
	{
		FireRadialProjectiles();
		attackTimer = 3.0f;
		PlayAnimation(1, false);
	}

	DamagePlayerOnContact(*player, 1, 1.2f);
	UpdateProjectiles(elapsedTime);
	UpdateInvincibleTimer(elapsedTime);
	if (animator) animator->Update(elapsedTime);
	UpdateTransform();
}

void EnemyHover::FireRadialProjectiles()
{
	constexpr int PROJECTILE_COUNT = 6;
	constexpr float PROJECTILE_SPEED = 5.0f;
	const DirectX::XMFLOAT3 start = { position.x, position.y + 0.5f, position.z };
	for (int i = 0; i < PROJECTILE_COUNT; ++i)
	{
		const float angle = DirectX::XM_2PI * i / PROJECTILE_COUNT;
		const DirectX::XMFLOAT3 velocity = {
			std::cos(angle) * PROJECTILE_SPEED,
			std::sin(angle) * PROJECTILE_SPEED,
			0.0f
		};
		projectiles.emplace_back(start, velocity);
	}
}

void EnemyHover::UpdateProjectiles(float elapsedTime)
{
	for (auto& projectile : projectiles)
	{
		projectile.Update(elapsedTime);
		projectile.HitPlayer(*player, 1.2f);
	}
	projectiles.erase(
		std::remove_if(projectiles.begin(), projectiles.end(),
			[](const EnemyProjectile& projectile) { return !projectile.IsActive(); }),
		projectiles.end());
}

void EnemyHover::DrawPrimitive(ShapeRenderer* shapeRenderer) const
{
	for (const auto& projectile : projectiles) projectile.DrawDebugPrimitive(shapeRenderer);
}
