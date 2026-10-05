#pragma once

#include <d3d11.h>
#include <vector>
#include <array>
#include <random>

#include "Enemy.h"
#include "EnemyProjectile.h"
#include "BossSideProjectile.h"

class CameraController;
class Player;

class EnemyBoss final : public Enemy
{
public:
	EnemyBoss(ID3D11Device* device, Player* player);
	~EnemyBoss() override;
	void Update(float elapsedTime) override;
	void DrawPrimitive(ShapeRenderer* shapeRenderer) const override;

	static bool GetDefeat() { return defeat_flag; }
	static void SetDefeat(bool value) { defeat_flag = value; }
	void SetCameraController(CameraController* controller) { camera_controller = controller; }
	void SetActionEnable(bool value) { action_enabled = value; }
	void SetArenaBounds(float min_x, float max_x);

private:
	void UpdateStateMachine(float elapsed_time);
	void FireAtPlayer(float angleOffset = 0.0f);
	void UpdateProjectiles(float elapsedTime);
	void OnDead() override;

private:
	Player* player = nullptr;
	CameraController* camera_controller = nullptr;
	std::vector<EnemyProjectile> projectiles;
	std::vector<std::unique_ptr<BossSideProjectile>> side_projectiles;
	float state_timer = 1.5f;
	float shot_timer = 0.0f;
	float arena_center_x = 0.0f;
	// Legacy gravity was -1 per frame, multiplied by 60 * elapsedTime.
	static constexpr float boss_gravity_acceleration = 60.0f;
	static constexpr float default_arena_half_width = 7.0f;
	float arena_min_x = 0.0f;
	float arena_max_x = 0.0f;
	bool arena_initialized = false;
	bool action_enabled = true;
	int shots_remaining = 0;
	static bool defeat_flag;

private:
	enum class StateId
	{
		None = 0,
		Idle,
		JumpMove,
		Attack,
		Continuous,
		Tornado,
		Death,
		EnumCount,
	};

	void SetState(StateId state_id);

	std::vector<StateId> candidates;

public:
	class State
	{
	public:
		State(EnemyBoss* owner) : owner(owner) {}
		virtual ~State() = default;

	public:
		virtual void OnEnter() {}
		virtual void OnExit() {}
		virtual void OnUpdate(float elapsed_time) {}

	protected:
		EnemyBoss* owner;
	};

	class IdleState : public State
	{
	public:
		IdleState(EnemyBoss* owner) : State(owner) {}
		void OnEnter() override;
		void OnUpdate(float elapsed_time) override;
		void SelectNextState();
	};

	class JumpMoveState : public State
	{
	public:
		JumpMoveState(EnemyBoss* owner) : State(owner) {}
		void OnEnter() override;
		void OnUpdate(float elapsed_time) override;

	private:
		enum class Phase
		{
			Ready,
			Airborne,
			Waiting,
			Completed,
		};

		static constexpr int jump_count_limit = 3;
		static constexpr float jump_powers[jump_count_limit] = { 15.0f, 16.5f, 20.0f };
		static constexpr int low_health_threshold = 5;
		static constexpr float normal_jump_delay = 0.1f;
		static constexpr float low_health_jump_delay = 0.05f;
		// Land one block inside either edge of the room.
		static constexpr float landing_edge_offset = 1.0f;
		static constexpr float turn_angle_degrees = 180.0f;
		static constexpr int idle_animation = 0;
		static constexpr int jump_start_animation = 2;
		static constexpr const char* jump_sound = "SE_BOSS_JUMP";
		static constexpr const char* landing_sound = "SE_BOSS_LANDING";

		Phase phase = Phase::Ready;
		int jump_count = 0;
		float jump_delay = 0.0f;
		float move_start_x = 0.0f;
		float move_target_x = 0.0f;
		float jump_target_x = 0.0f;
	};

	class AttackState : public State
	{
	public:
		AttackState(EnemyBoss* owner) : State(owner) {}
		void OnEnter() override;
		void OnUpdate(float elapsed_time) override;

	private:
		enum class Phase { Windup, Fire, Recovery, Completed };
		struct SpawnOffset { float x; float y; };
		void FireSideProjectiles();
		static constexpr int projectile_count = 6;
		static constexpr int pattern_count = 3;
		static constexpr SpawnOffset patterns[pattern_count][projectile_count] = {
			{{0.2f, 0.15f}, {0.4f, 0.12f}, {0.6f, 0.45f}, {0.8f, 0.30f}, {1.0f, 0.55f}, {1.2f, 0.40f}},
			{{0.0f, 0.30f}, {0.2f, 0.45f}, {0.35f, 0.25f}, {0.6f, 0.50f}, {0.8f, 0.05f}, {1.0f, 0.45f}},
			{{0.2f, 0.08f}, {0.4f, 0.15f}, {0.6f, 0.20f}, {0.8f, 0.55f}, {1.0f, 0.10f}, {1.2f, 0.42f}}
		};
		static constexpr float windup_duration = 0.5f;
		static constexpr float recovery_duration = 0.5f;
		static constexpr float scatter_base_distance = 2.0f;
		static constexpr float scatter_distance_scale = 6.0f;
		static constexpr float scatter_height_scale = 10.0f;
		static constexpr float muzzle_height_ratio = 0.5f;
		static constexpr int attack_animation = 1;
		static constexpr const char* attack_sound = "SE_BOSS_ATTACK";
		Phase phase = Phase::Windup;
		float phase_time = 0.0f;
		float direction_x = -1.0f;
	};

	
	class ContinuousState : public State
	{
	public:
		ContinuousState(EnemyBoss* owner) : State(owner) {}
		void OnEnter() override;
		void OnUpdate(float elapsed_time) override;
	private:
		void FireVolley();
		std::array<DirectX::XMFLOAT3, 5> GenerateSpawnPositions();
		static constexpr int shot_count = 5;
		static constexpr float shot_interval = 0.7f;
		static constexpr int min_projectiles = 2;
		static constexpr int max_projectiles = 4;
		static constexpr int spawn_slot_count = 5;
		static constexpr DirectX::XMFLOAT2 spawn_offsets[spawn_slot_count] = {
			{1.0f, 0.5f}, {1.0f, 2.0f}, {2.0f, 1.25f}, {3.0f, 0.5f}, {3.0f, 2.0f}
		};
		static constexpr float spawn_jitter = 0.08f;
		static constexpr float projectile_clearance = 0.1f;
		static constexpr float minimum_spacing = BossSideProjectile::GetRadius() * 2.0f + projectile_clearance;
		static_assert((1.0f - 2.0f * spawn_jitter) * (1.0f - 2.0f * spawn_jitter)
			+ (0.75f - 2.0f * spawn_jitter) * (0.75f - 2.0f * spawn_jitter)
			>= minimum_spacing * minimum_spacing, "Continuous projectile slots are too close");
		static constexpr int attack_animation = 1;
		static constexpr const char* attack_sound = "SE_BOSS_ATTACK";
		float time_until_shot = shot_interval;
		float direction_x = -1.0f;
		int volleys_fired = 0;
		std::mt19937 random_engine{ std::random_device{}() };
	};

	class TornadoState : public State
	{
	public:
		TornadoState(EnemyBoss* owner) : State(owner) {}
		void OnEnter() override;
		void OnUpdate(float elapsed_time) override;
		void OnExit() override;
		void DrawDebugPrimitive(ShapeRenderer* renderer) const;

	private:
		void UpdatePullAndCollision(float elapsed_time);
		void UpdateBacksidePush(float elapsed_time);
		float GetRadiusAtHeight(float relative_y) const;

		static constexpr int low_health_threshold = 5;
		static constexpr float normal_duration = 3.0f;
		static constexpr float low_health_duration = 4.0f;
		static constexpr float tornado_height = 10.0f;
		static constexpr float bottom_radius = 0.5f;
		static constexpr float top_radius = 6.0f;
		static constexpr float base_y_offset = -2.0f;
		static constexpr float vertical_margin = 2.0f;
		static constexpr float pull_range_multiplier = 9.0f;
		static constexpr float pull_speed = 4.0f;
		static constexpr float pull_min_distance = 0.1f;
		static constexpr float lift_speed_threshold = 3.0f;
		static constexpr float lift_impulse = 3.0f;
		static constexpr int damage = 1;
		static constexpr float invincible_duration = 1.2f;
		static constexpr float backside_height_tolerance = 0.5f;
		static constexpr float backside_front_margin = 0.5f;
		static constexpr float backside_near_distance = 1.0f;
		static constexpr float backside_push_acceleration = 30.0f;
		static constexpr float backside_near_acceleration = 50.0f;
		static constexpr float backside_lift_acceleration = 15.0f;
		static constexpr float backside_max_speed = 5.0f;
		static constexpr float backside_near_max_speed = 15.0f;
		static constexpr int attack_animation = 1;
		static constexpr int debug_segments = 8;
		static constexpr const char* attack_sound = "SE_BOSS_ATTACK";
		static constexpr const char* storm_sound = "SE_BOSS_STORM";

		DirectX::XMFLOAT3 tornado_position = { 0.0f, 0.0f, 0.0f };
		float remaining_time = 0.0f;
		bool active = false;
		std::unique_ptr<Effect> wind_effect;
		Effekseer::Handle wind_handle = -1;
	};

	
	class DeathState : public State
	{
	public:
		DeathState(EnemyBoss* owner) : State(owner) {}
		void OnEnter() override;
		void OnUpdate(float elapsed_time) override;
		void OnExit() override;
	private:
		enum class Visual { Light, Shock, Smoke, Flare, BlackExplosion, SmallExplosion, Explosion, Fire, Count };
		static constexpr size_t visual_count = static_cast<size_t>(Visual::Count);
		static constexpr const char* effect_paths[visual_count] = {
			"Data/Effect/LightShaft.efkefc", "Data/Effect/ShockWave.efkefc",
			"Data/Effect/BlackSmoke.efkefc", "Data/Effect/LeakLight.efkefc",
			"Data/Effect/BlackExplosion.efkefc", "Data/Effect/SmallExplosion.efkefc",
			"Data/Effect/Bossdeath.efkefc", "Data/Effect/Fire.efkefc"
		};
		void PlayVisual(Visual visual, const DirectX::XMFLOAT3& position, float scale = 1.0f);
		void StopVisuals();
		static constexpr float death_duration = 8.0f;
		static constexpr float pose_hold_time = 0.15f;
		static constexpr float explosion_time = 2.5f;
		static constexpr float shock_time = 2.2f;
		static constexpr float small_explosion_interval = 0.05f;
		static constexpr float shake_power = 0.05f;
		static constexpr float small_explosion_width = 0.5f;
		static constexpr int death_animation = 6;
		float death_time = 0.0f;
		float next_explosion_time = pose_hold_time;
		bool buildup_started = false;
		bool shock_started = false;
		bool exploded = false;
		bool active = false;
		DirectX::XMFLOAT3 base_position = {};
		std::array<std::unique_ptr<Effect>, visual_count> effects;
		std::vector<std::pair<Visual, Effekseer::Handle>> effect_handles;
	};

	StateId	current_state = StateId::None;
	StateId	next_state = StateId::None;
	std::unique_ptr<State> states[static_cast<size_t>(StateId::EnumCount)];
	int consecutiveAttackCount = 0;
	StateId lastAction = StateId::None;
	
};
