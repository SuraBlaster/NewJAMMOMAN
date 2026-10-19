#pragma once

#include <d3d11.h>
#include <vector>

#include "Enemy.h"
#include "EnemyProjectile.h"

class Player;

class EnemyHover final : public Enemy
{
public:
	EnemyHover(ID3D11Device* device, Player* player);
	void Update(float elapsedTime) override;
	void DrawPrimitive(ShapeRenderer* shapeRenderer) const override;

private:
	void FireRadialProjectiles();
	void UpdateProjectiles(float elapsedTime);
	void OnDead() override { Destroy(); }

	Player* player = nullptr;
	std::vector<EnemyProjectile> projectiles;
	float attackTimer = 2.0f;
	float hoverTime = 0.0f;
	float baseY = 0.0f;
};
