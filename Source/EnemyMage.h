#pragma once

#include <d3d11.h>
#include <vector>

#include "Enemy.h"
#include "EnemyProjectile.h"

class Player;

class EnemyMage final : public Enemy
{
public:
	EnemyMage(ID3D11Device* device, Player* player);
	void Update(float elapsedTime) override;
	void DrawPrimitive(ShapeRenderer* shapeRenderer) const override;

private:
	void FireProjectile();
	void UpdateProjectiles(float elapsedTime);
	void OnDead() override { Destroy(); }

	Player* player = nullptr;
	std::vector<EnemyProjectile> projectiles;
	float attack_timer = 1.5f;
};
