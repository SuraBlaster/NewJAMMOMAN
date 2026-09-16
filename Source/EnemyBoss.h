#pragma once

#include <d3d11.h>
#include <vector>

#include "Enemy.h"
#include "EnemyProjectile.h"

class CameraController;
class Player;

class EnemyBoss final : public Enemy
{
public:
	EnemyBoss(ID3D11Device* device, Player* player);
	void Update(float elapsedTime) override;
	void DrawPrimitive(ShapeRenderer* shapeRenderer) const override;

	static bool GetDefeat() { return defeat_flag; }
	static void SetDefeat(bool value) { defeat_flag = value; }
	void SetCameraController(CameraController* controller) { camera_controller = controller; }
	void SetActionEnable(bool value) { action_enabled = value; }

private:
	enum class State { Idle, JumpMove, Attack, Continuous, Tornado, Death };

	void EnterIdle();
	void SelectNextAction();
	void FireAtPlayer(float angleOffset = 0.0f);
	void UpdateProjectiles(float elapsedTime);
	void UpdateTornado(float elapsedTime);
	void OnDead() override;

	Player* player = nullptr;
	CameraController* camera_controller = nullptr;
	std::vector<EnemyProjectile> projectiles;
	State state = State::Idle;
	State next_state = State::Attack;
	float state_timer = 1.5f;
	float shot_timer = 0.0f;
	float arena_center_x = 0.0f;
	bool arena_initialized = false;
	bool jump_started = false;
	bool action_enabled = true;
	int shots_remaining = 0;
	static bool defeat_flag;
};
