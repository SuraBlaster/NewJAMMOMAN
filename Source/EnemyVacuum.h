#pragma once

#include <d3d11.h>
#include "Enemy.h"

class Player;

class EnemyVacuum final : public Enemy
{
public:
	EnemyVacuum(ID3D11Device* device, Player* player);
	void Update(float elapsedTime) override;
	void SetDirection(float direction) { move_direction = direction < 0.0f ? -1.0f : 1.0f; }

	enum class StateId
	{
		None,
		Move,
		Pursuit,
		EnumCount,
	};
	StateId GetState() const { return current_state; }

	class State
	{
	public:
		explicit State(EnemyVacuum* owner) : owner(owner) {}
		virtual ~State() = default;
		virtual void OnEnter() = 0;
		virtual void OnUpdate(float elapsedTime) = 0;

	protected:
		EnemyVacuum* owner;
	};

	class MoveState final : public State
	{
	public:
		explicit MoveState(EnemyVacuum* owner) : State(owner) {}
		void OnEnter() override;
		void OnUpdate(float elapsedTime) override;
	};

	class PursuitState final : public State
	{
	public:
		explicit PursuitState(EnemyVacuum* owner) : State(owner) {}
		void OnEnter() override;
		void OnUpdate(float elapsedTime) override;
	};

private:
	void SetState(StateId stateId);
	void UpdateStateMachine(float elapsedTime);
	float GetPlayerDistance() const;
	void UpdateWallCollision(float elapsedTime);
	void OnDead() override { Destroy(); }

	Player* player = nullptr;
	StateId current_state = StateId::None;
	StateId next_state = StateId::None;
	std::unique_ptr<State> states[static_cast<size_t>(StateId::EnumCount)];
	float move_speed = 1.5f;
	float move_direction = -1.0f;
	float wall_turn_timer = 0.0f;
	float pursuit_speed = 3.0f;
	float pursuit_start_distance = 5.0f;
	float pursuit_end_distance = 7.0f;
};
