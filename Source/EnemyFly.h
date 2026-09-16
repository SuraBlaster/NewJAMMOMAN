#pragma once

#include <d3d11.h>
#include "Enemy.h"

class Player;

class EnemyFly final : public Enemy
{
public:
	EnemyFly(ID3D11Device* device, Player* player);
	void Update(float elapsedTime) override;

private:
	void OnDead() override { Destroy(); }

	ID3D11Device* device = nullptr;
	Player* player = nullptr;
	float move_speed = 5.0f;
	bool egg_dropped = false;
};
