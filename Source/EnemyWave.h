#pragma once

#include <d3d11.h>

#include "Enemy.h"

class Player;

// Rises from below and moves horizontally with a wave motion.
class EnemyWave final : public Enemy
{
public:
	enum class Direction
	{
		Left,
		Right
	};

	EnemyWave(ID3D11Device* device, Player* player);
	~EnemyWave() override = default;

	void Update(float elapsedTime) override;

	void SetDirection(Direction value);
	void SetWave(float speed, float amplitude)
	{
		waveSpeed = speed;
		waveAmplitude = amplitude;
	}

private:
	void UpdateAttacking(float elapsedTime);
	void CollideWithPlayer();
	bool IsOutOfRange() const;
	void OnDead() override { Destroy(); }

	Player* player = nullptr; // Non-owning; GameScene owns Player.
	Direction direction = Direction::Right;
	float targetY = 0.0f;
	bool targetYInitialized = false;
	float riseSpeed = 3.0f;
	float moveSpeed = 3.0f;
	float waveSpeed = 4.0f;
	float waveAmplitude = 0.5f;
	float waveTime = 0.0f;
};
