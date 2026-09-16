#pragma once

#include <d3d11.h>
#include "Enemy.h"

class Player;

class EnemyFan final : public Enemy
{
public:
	EnemyFan(ID3D11Device* device, Player* player);
	void Update(float elapsedTime) override;

private:
	void OnDead() override { Destroy(); }
	Player* player = nullptr;
};
