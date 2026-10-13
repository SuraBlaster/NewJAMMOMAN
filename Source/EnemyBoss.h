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

	static bool GetDefeat() { return defeatFlag; }
	static void SetDefeat(bool value) { defeatFlag = value; }
	void SetCameraController(CameraController* controller) { cameraController = controller; }
	void SetActionEnable(bool value) { actionEnabled = value; }
	void SetArenaBounds(float minX, float maxX);

private:
	void UpdateStateMachine(float elapsedTime);
	void FireAtPlayer(float angleOffset = 0.0f);
	void UpdateProjectiles(float elapsedTime);
	void OnDead() override;

private:
	Player* player = nullptr;
	CameraController* cameraController = nullptr;
	std::vector<EnemyProjectile> projectiles;
	std::vector<std::unique_ptr<BossSideProjectile>> sideProjectiles;
	float stateTimer = 1.5f;
	float shotTimer = 0.0f;
	float arenaCenterX = 0.0f;
	// Legacy gravity was -1 per frame, multiplied by 60 * elapsedTime.
	static constexpr float BOSS_GRAVITY_ACCELERATION = 60.0f;
	static constexpr float DEFAULT_ARENA_HALF_WIDTH = 7.0f;
	float arenaMinX = 0.0f;
	float arenaMaxX = 0.0f;
	bool arenaInitialized = false;
	bool actionEnabled = true;
	int shotsRemaining = 0;
	static bool defeatFlag;

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

	void SetState(StateId stateId);

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
		virtual void OnUpdate(float elapsedTime) {}

	protected:
		EnemyBoss* owner;
	};

	class IdleState : public State
	{
	public:
		IdleState(EnemyBoss* owner) : State(owner) {}
		void OnEnter() override;
		void OnUpdate(float elapsedTime) override;
		void SelectNextState();
	};

	class JumpMoveState : public State
	{
	public:
		JumpMoveState(EnemyBoss* owner) : State(owner) {}
		void OnEnter() override;
		void OnUpdate(float elapsedTime) override;

	private:
		enum class Phase
		{
			Ready,
			Airborne,
			Waiting,
			Completed,
		};

		static constexpr int JUMP_COUNT_LIMIT = 3;
		static constexpr float JUMP_POWERS[JUMP_COUNT_LIMIT] = { 15.0f, 16.5f, 20.0f };
		static constexpr int LOW_HEALTH_THRESHOLD = 5;
		static constexpr float NORMAL_JUMP_DELAY = 0.1f;
		static constexpr float LOW_HEALTH_JUMP_DELAY = 0.05f;
		// Land one block inside either edge of the room.
		static constexpr float LANDING_EDGE_OFFSET = 1.0f;
		static constexpr float TURN_ANGLE_DEGREES = 180.0f;
		static constexpr int IDLE_ANIMATION = 0;
		static constexpr int JUMP_START_ANIMATION = 2;
		static constexpr const char* JUMP_SOUND = "SE_BOSS_JUMP";
		static constexpr const char* LANDING_SOUND = "SE_BOSS_LANDING";

		Phase phase = Phase::Ready;
		int jumpCount = 0;
		float jumpDelay = 0.0f;
		float moveStartX = 0.0f;
		float moveTargetX = 0.0f;
		float jumpTargetX = 0.0f;
	};

	class AttackState : public State
	{
	public:
		AttackState(EnemyBoss* owner) : State(owner) {}
		void OnEnter() override;
		void OnUpdate(float elapsedTime) override;

	private:
		enum class Phase { Windup, Fire, Recovery, Completed };
		struct SpawnOffset { float x; float y; };
		void FireSideProjectiles();
		static constexpr int PROJECTILE_COUNT = 6;
		static constexpr int PATTERN_COUNT = 3;
		static constexpr SpawnOffset PATTERNS[PATTERN_COUNT][PROJECTILE_COUNT] = {
			{{0.2f, 0.15f}, {0.4f, 0.12f}, {0.6f, 0.45f}, {0.8f, 0.30f}, {1.0f, 0.55f}, {1.2f, 0.40f}},
			{{0.0f, 0.30f}, {0.2f, 0.45f}, {0.35f, 0.25f}, {0.6f, 0.50f}, {0.8f, 0.05f}, {1.0f, 0.45f}},
			{{0.2f, 0.08f}, {0.4f, 0.15f}, {0.6f, 0.20f}, {0.8f, 0.55f}, {1.0f, 0.10f}, {1.2f, 0.42f}}
		};
		static constexpr float WINDUP_DURATION = 0.5f;
		static constexpr float RECOVERY_DURATION = 0.5f;
		static constexpr float SCATTER_BASE_DISTANCE = 2.0f;
		static constexpr float SCATTER_DISTANCE_SCALE = 6.0f;
		static constexpr float SCATTER_HEIGHT_SCALE = 10.0f;
		static constexpr float MUZZLE_HEIGHT_RATIO = 0.5f;
		static constexpr int ATTACK_ANIMATION = 1;
		static constexpr const char* ATTACK_SOUND = "SE_BOSS_ATTACK";
		Phase phase = Phase::Windup;
		float phaseTime = 0.0f;
		float directionX = -1.0f;
	};

	
	class ContinuousState : public State
	{
	public:
		ContinuousState(EnemyBoss* owner) : State(owner) {}
		void OnEnter() override;
		void OnUpdate(float elapsedTime) override;
	private:
		void FireVolley();
		std::array<DirectX::XMFLOAT3, 5> GenerateSpawnPositions();
		static constexpr int SHOT_COUNT = 5;
		static constexpr float SHOT_INTERVAL = 0.7f;
		static constexpr int MIN_PROJECTILES = 2;
		static constexpr int MAX_PROJECTILES = 4;
		static constexpr int SPAWN_SLOT_COUNT = 5;
		static constexpr DirectX::XMFLOAT2 SPAWN_OFFSETS[SPAWN_SLOT_COUNT] = {
			{1.0f, 0.5f}, {1.0f, 2.0f}, {2.0f, 1.25f}, {3.0f, 0.5f}, {3.0f, 2.0f}
		};
		static constexpr float SPAWN_JITTER = 0.08f;
		static constexpr float PROJECTILE_CLEARANCE = 0.1f;
		static constexpr float MINIMUM_SPACING = BossSideProjectile::GetRadius() * 2.0f + PROJECTILE_CLEARANCE;
		static_assert((1.0f - 2.0f * SPAWN_JITTER) * (1.0f - 2.0f * SPAWN_JITTER)
			+ (0.75f - 2.0f * SPAWN_JITTER) * (0.75f - 2.0f * SPAWN_JITTER)
			>= MINIMUM_SPACING * MINIMUM_SPACING, "Continuous projectile slots are too close");
		static constexpr int ATTACK_ANIMATION = 1;
		static constexpr const char* ATTACK_SOUND = "SE_BOSS_ATTACK";
		float timeUntilShot = SHOT_INTERVAL;
		float directionX = -1.0f;
		int volleysFired = 0;
		std::mt19937 randomEngine{ std::random_device{}() };
	};

	class TornadoState : public State
	{
	public:
		TornadoState(EnemyBoss* owner) : State(owner) {}
		void OnEnter() override;
		void OnUpdate(float elapsedTime) override;
		void OnExit() override;
		void DrawDebugPrimitive(ShapeRenderer* renderer) const;

	private:
		void UpdatePullAndCollision(float elapsedTime);
		void UpdateBacksidePush(float elapsedTime);
		float GetRadiusAtHeight(float relativeY) const;

		static constexpr int LOW_HEALTH_THRESHOLD = 5;
		static constexpr float NORMAL_DURATION = 3.0f;
		static constexpr float LOW_HEALTH_DURATION = 4.0f;
		static constexpr float TORNADO_HEIGHT = 10.0f;
		static constexpr float BOTTOM_RADIUS = 0.5f;
		static constexpr float TOP_RADIUS = 6.0f;
		static constexpr float BASE_Y_OFFSET = -2.0f;
		static constexpr float VERTICAL_MARGIN = 2.0f;
		static constexpr float PULL_RANGE_MULTIPLIER = 9.0f;
		static constexpr float PULL_SPEED = 4.0f;
		static constexpr float PULL_MIN_DISTANCE = 0.1f;
		static constexpr float LIFT_SPEED_THRESHOLD = 3.0f;
		static constexpr float LIFT_IMPULSE = 3.0f;
		static constexpr int DAMAGE = 1;
		static constexpr float INVINCIBLE_DURATION = 1.2f;
		static constexpr float BACKSIDE_HEIGHT_TOLERANCE = 0.5f;
		static constexpr float BACKSIDE_FRONT_MARGIN = 0.5f;
		static constexpr float BACKSIDE_NEAR_DISTANCE = 1.0f;
		static constexpr float BACKSIDE_PUSH_ACCELERATION = 30.0f;
		static constexpr float BACKSIDE_NEAR_ACCELERATION = 50.0f;
		static constexpr float BACKSIDE_LIFT_ACCELERATION = 15.0f;
		static constexpr float BACKSIDE_MAX_SPEED = 5.0f;
		static constexpr float BACKSIDE_NEAR_MAX_SPEED = 15.0f;
		static constexpr int ATTACK_ANIMATION = 1;
		static constexpr int DEBUG_SEGMENTS = 8;
		static constexpr const char* ATTACK_SOUND = "SE_BOSS_ATTACK";
		static constexpr const char* STORM_SOUND = "SE_BOSS_STORM";

		DirectX::XMFLOAT3 tornadoPosition = { 0.0f, 0.0f, 0.0f };
		float remainingTime = 0.0f;
		bool active = false;
		std::unique_ptr<Effect> windEffect;
		Effekseer::Handle windHandle = -1;
	};

	
	class DeathState : public State
	{
	public:
		DeathState(EnemyBoss* owner) : State(owner) {}
		void OnEnter() override;
		void OnUpdate(float elapsedTime) override;
		void OnExit() override;
	private:
		enum class Visual { Light, Shock, Smoke, Flare, BlackExplosion, SmallExplosion, Explosion, Fire, Count };
		static constexpr size_t VISUAL_COUNT = static_cast<size_t>(Visual::Count);
		static constexpr const char* EFFECT_PATHS[VISUAL_COUNT] = {
			"Data/Effect/LightShaft.efkefc", "Data/Effect/ShockWave.efkefc",
			"Data/Effect/BlackSmoke.efkefc", "Data/Effect/LeakLight.efkefc",
			"Data/Effect/BlackExplosion.efkefc", "Data/Effect/SmallExplosion.efkefc",
			"Data/Effect/Bossdeath.efkefc", "Data/Effect/Fire.efkefc"
		};
		void PlayVisual(Visual visual, const DirectX::XMFLOAT3& position, float scale = 1.0f);
		void StopVisuals();
		static constexpr float DEATH_DURATION = 8.0f;
		static constexpr float POSE_HOLD_TIME = 0.15f;
		static constexpr float EXPLOSION_TIME = 2.5f;
		static constexpr float SHOCK_TIME = 2.2f;
		static constexpr float SMALL_EXPLOSION_INTERVAL = 0.05f;
		static constexpr float SHAKE_POWER = 0.05f;
		static constexpr float SMALL_EXPLOSION_WIDTH = 0.5f;
		static constexpr int DEATH_ANIMATION = 6;
		float deathTime = 0.0f;
		float nextExplosionTime = POSE_HOLD_TIME;
		bool buildupStarted = false;
		bool shockStarted = false;
		bool exploded = false;
		bool active = false;
		DirectX::XMFLOAT3 basePosition = {};
		std::array<std::unique_ptr<Effect>, VISUAL_COUNT> effects;
		std::vector<std::pair<Visual, Effekseer::Handle>> effectHandles;
	};

	StateId	currentState = StateId::None;
	StateId	nextState = StateId::None;
	std::unique_ptr<State> states[static_cast<size_t>(StateId::EnumCount)];
	int consecutiveAttackCount = 0;
	StateId lastAction = StateId::None;
	
};
