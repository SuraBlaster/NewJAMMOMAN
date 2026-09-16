#pragma once

#include <d3d11.h>
#include "Enemy.h"

class Player;

class EnemyScatter final : public Enemy
{
public:
	EnemyScatter(ID3D11Device* device, Player* player, const DirectX::XMFLOAT3& spawnPosition);
	void Update(float elapsedTime) override;

private:
	enum class State { Scatter, Charge };
	void OnDead() override { Destroy(); }

	Player* player = nullptr;
	State state = State::Scatter;
	DirectX::XMFLOAT3 move_direction = { 0, 0, 0 };
	float scatter_time = 0.0f;
	float scatter_duration = 1.0f;
	float speed = 6.0f;
};
