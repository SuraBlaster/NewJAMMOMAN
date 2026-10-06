#pragma once

#include <memory>
#include "Character.h"
#include "Effect/Effect.h"
#include <deque>
#include <unordered_set>
#include <Collision.h>

class Enemy;

class Player final : public Character
{
public:
	Player(ID3D11Device* device);
	~Player() override;
	bool IsClearing() const { return current_state == StateId::Clear || next_state == StateId::Clear; }

	void Update(float elapsed_time) override;

	void DrawGUI();

	void RenderTrail(class PrimitiveRenderer* primitiveRenderer);
	void DrawDebugPrimitive(class ShapeRenderer* shapeRenderer) const;

	void SetBladeActive(bool active);
	void SetTrailActive(bool active);

	AABB GetBodyAABB() const;

private:
	void OnDamaged()override;
	void OnDead() override;
	void UpdateVelocity(float elapsed_time);
	void UpdateStateMachine(float elapsed_time);
	void UpdateUpperBody(float elapsed_time);
	void UpdateDash();
	void UpdateMotionTrail(float elapsedTime);
	void UpdateSwordTrail(float elapsed_time);
	void CheckSwordCollision(
		const DirectX::XMFLOAT3& bladeRoot,
		const DirectX::XMFLOAT3& bladeTip,
		const DirectX::XMFLOAT3& previousRoot,
		const DirectX::XMFLOAT3& previousTip);

	bool InputMove();
	bool InputJump();
	bool InputShot();
	bool InputDash();
	bool InputTowardWall();
	bool InputAwayFromWall();
	void HoldJump();
	void GravityChange(float elapsed_time);

	/// <summary>
	/// Trueは前に壁が存在する
	/// </summary>
	/// <returns></returns>
	bool WallJudgement(float elapsed_time);

	void MoveAndCollide(float elapsed_time);

private:
	float							gravity = 10.0f;
	float							acceleration = 50.0f;
	float							deceleration = 20.0f;
	float							move_speed = 6.0f;
	float							jump_speed = 10.5f;
	float							cut_jump_velocity = 2.0f;
	float							air_control = 0.3f;
	float							input_move_x = 0.0f;
	float							input_move_z = 0.0f;

	AnimationLayerMask				upper_body_mask;
	float							upper_body_weight = 0.0f;
	bool							is_upper_body_active = false;

	// ダッシュ関連
	float dash_timer = 0.0f;           
	const float dash_duration = 0.8f;
	const float dash_speed = 10.0f;
	bool is_dash_jump = false;

	// 攻撃コンボ
	int combo_step = 0;
	bool has_next_combo_input = false;

	// 空中攻撃フラグ
	bool is_air_attack = false;

	// 壁つかまりフラグ
	bool wall_climb = false;
	
	// 壁の法線
	DirectX::XMFLOAT3 wallNormal = {};

	// 壁キック
	float wall_kick_lock_timer = 0.0f;
	float wall_kick_lock_duration = 0.2f;

	// ソードトレイル
	static const int MAX_POLYGON = 32;
	static constexpr float SWORD_COLLISION_RADIUS = 0.12f;
	static constexpr int SWORD_SWEEP_STEPS = 4;
	DirectX::XMFLOAT3 trailPositions[2][MAX_POLYGON];
	bool is_blade_active = false;
	bool is_trail_active = false;
	bool prev_blade_active = false;
	bool prev_trail_active = false;
	float trailFadeTimer = 0.0f;
	const float trailFadeDuration = 0.25f;
	std::unordered_set<const Enemy*> sword_hit_enemies;
	DirectX::XMFLOAT3 debug_blade_root = { 0, 0, 0 };
	DirectX::XMFLOAT3 debug_blade_tip = { 0, 0, 0 };
	DirectX::XMFLOAT3 debug_previous_blade_root = { 0, 0, 0 };
	DirectX::XMFLOAT3 debug_previous_blade_tip = { 0, 0, 0 };
	bool has_sword_collision_debug_data = false;
	bool show_sword_collision = false;

	// 自機AABB関連
	float body_half_width = 0.4f;
	float body_half_depth = 0.4f;
	float body_height = 1.4f;

	bool show_body_aabb = true;

	// 開始時点のめり込みを確認するためのフラグ。
	bool has_body_overlap = false;

	std::shared_ptr<Effect> spawnEffect;
	Effekseer::Handle spawnHandle = -1;

private:
	enum class StateId
	{
		None = 0,
		Idle,
		Move,
		Attack,
		Jump,
		JumpFall,
		Dash,
		WallSlide,
		WallKick,
		Clear,
		Damage,
		Death,
		Spawn,
		EnumCount
	};

	int stateid;

	std::string debugstatename [static_cast<int>(StateId::EnumCount) + 2] =
	{
		"None",
		"Idle",
		"Move",
		"Attack",
		"Jump",
		"JumpFall",
		"Dash",
		"WallSlide",
		"WallKick",
		"Clear",
		"Damage",
		"Death",
		"Spawn"
		"EnumCount"
	};

	void SetState(StateId state_id);

	struct TrailData {
		std::shared_ptr<Model> trailModel;
		DirectX::XMFLOAT3 position;
		DirectX::XMFLOAT3 angle;
		DirectX::XMFLOAT3 scale;
		DirectX::XMFLOAT4 drawColor;
		std::vector<Model::NodePose> nodePoses;
		float alpha;
	};

	std::deque<TrailData> trails;
	float trailRecordTimer = 0.0f;
	const int max_trails = 3;
	const float trail_interval = 0.05f;

public:
	const std::deque<TrailData>& GetTrails() const { return trails; }
public:
	class State
	{
	public:
		State(Player* owner) : owner(owner) {}
		virtual ~State() = default;

	public:
		virtual void OnEnter() {}
		virtual void OnExit() {}
		virtual void OnUpdate(float elapsed_time) {}

	protected:
		Player* owner;
	};

	class IdleState : public State
	{
	public:
		IdleState(Player* owner) : State(owner) {}
		void OnEnter() override;
		void OnUpdate(float elapsed_time) override;
	};

	class MoveState : public State
	{
	public:
		MoveState(Player* owner) : State(owner) {}
		void OnEnter() override;
		void OnUpdate(float elapsed_time) override;
	};

	class AttackState : public State
	{
	public:
		AttackState(Player* owner) : State(owner) {}
		void OnEnter() override;
		void OnUpdate(float elapsed_time) override;
	};

	class JumpState : public State
	{
	public:
		JumpState(Player* owner) : State(owner) {}
		void OnEnter() override;
		void OnUpdate(float elapsed_time) override;
	};

	class DashState : public State
	{
	public:
		DashState(Player* owner) : State(owner) {}
		void OnEnter() override;
		void OnUpdate(float elapsed_time) override;
	};

	class JumpFallState : public State
	{
	public:
		JumpFallState(Player* owner) : State(owner) {}
		void OnEnter() override;
		void OnUpdate(float elapsed_time) override;
	};

	class WallSlideState : public State
	{
	public:
		WallSlideState(Player* owner) : State(owner) {}
		void OnEnter() override;
		void OnUpdate(float elapsed_time) override;
		void OnExit() override;
	private:
		float kick_power = 5.0f;
	};

	class WallKickState : public State
	{
	public:
		WallKickState(Player* owner) : State(owner) {}
		void OnEnter() override;
		void OnUpdate(float elapsed_time) override;
	};

	class DamageState : public State
	{
	public:
		DamageState(Player* owner) : State(owner) {}
		void OnEnter() override;
		void OnUpdate(float elapsed_time) override;
	};

    class DeathState : public State
    {
    public:
        DeathState(Player* owner) : State(owner) {}
        void OnEnter() override;
        void OnUpdate(float elapsed_time) override;
        void OnExit() override;
    private:
        static constexpr float retry_delay = 3.0f;
        static constexpr float effect_scale = 0.2f;
        static constexpr float vibration_power = 1.0f;
        static constexpr float vibration_duration = 1.5f;
        float timer = 0.0f;
        bool active = false;
        bool scene_requested = false;
        std::unique_ptr<Effect> death_effect;
        Effekseer::Handle death_handle = -1;
    };
    class ClearState : public State
    {
    public:
        ClearState(Player* owner) : State(owner) {}
        void OnEnter() override;
        void OnUpdate(float elapsed_time) override;
        void OnExit() override;
    private:
        static constexpr float clear_audio_time = 5.0f;
        static constexpr float exit_effect_time = 11.0f;
        static constexpr float scene_change_time = 13.0f;
        float timer = 0.0f;
        bool clear_audio_played = false;
        bool exit_started = false;
        bool scene_requested = false;
        std::unique_ptr<Effect> exit_effect;
        Effekseer::Handle exit_handle = -1;
    };
	class SpawnState : public State
	{
	public:
		SpawnState(Player* owner) : State(owner) {}
		void OnEnter() override;
		void OnUpdate(float elapsed_time) override;
	private:
		float spawnTimer = 0.0f;
		static constexpr float SPAWN_ANIM_START_TIME = 2.5f;
		static constexpr float SPAWN_END_TIME = 3.0f;
		bool hasSpawned = false;;
	};

	StateId	current_state = StateId::None;
	StateId	next_state = StateId::None;
	std::unique_ptr<State> states[static_cast<size_t>(StateId::EnumCount)];
};
