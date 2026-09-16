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
	float attack_timer = 2.0f;
	float hover_time = 0.0f;
	float base_y = 0.0f;
};
