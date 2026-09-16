#pragma once

#include <d3d11.h>
#include "Enemy.h"

class Player;

class EnemyEgg final : public Enemy
{
public:
	EnemyEgg(ID3D11Device* device, Player* player, const DirectX::XMFLOAT3& spawnPosition);
	void Update(float elapsedTime) override;

private:
	void OnLanding() override;
	void OnDead() override { Destroy(); }

	ID3D11Device* device = nullptr;
	Player* player = nullptr;
};
