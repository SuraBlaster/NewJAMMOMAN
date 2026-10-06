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
		wave_speed = speed;
		wave_amplitude = amplitude;
	}

private:
	void UpdateAttacking(float elapsedTime);
	void CollideWithPlayer();
	bool IsOutOfRange() const;
	void OnDead() override { Destroy(); }

	Player* player = nullptr; // Non-owning; GameScene owns Player.
	Direction direction = Direction::Right;
	float target_y = 0.0f;
	bool target_y_initialized = false;
	float rise_speed = 3.0f;
	float move_speed = 3.0f;
	float wave_speed = 4.0f;
	float wave_amplitude = 0.5f;
	float wave_time = 0.0f;
};
