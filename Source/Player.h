#pragma once

#include <memory>
#include "Character.h"
#include "Effect/Effect.h"
#include "PlayerTrail.h"
#include <unordered_set>
#include <Collision.h>

class Enemy;

class Player final : public Character
{
public:
	Player(ID3D11Device* device);
	~Player() override;
	bool IsClearing() const { return currentState == StateId::Clear || nextState == StateId::Clear; }

	void Update(float elapsedTime) override;

	void DrawGUI();

	void RenderTrail(class PrimitiveRenderer* primitiveRenderer);
	void RenderMotionTrail(class ModelRenderer* modelRenderer) const;
	void DrawDebugPrimitive(class ShapeRenderer* shapeRenderer) const;

	void SetBladeActive(bool active);
	void SetTrailActive(bool active);

	AABB GetBodyAABB() const;

private:
	void OnDamaged()override;
	void OnDead() override;
	void UpdateVelocity(float elapsedTime);
	void UpdateStateMachine(float elapsedTime);
	void UpdateUpperBody(float elapsedTime);
	void UpdateDash();
	void UpdateMotionTrail(float elapsedTime);
	void UpdateSwordTrail(float elapsedTime);
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
	void GravityChange(float elapsedTime);

	/// <summary>
	/// Trueは前に壁が存在する
	/// </summary>
	/// <returns></returns>
	bool WallJudgement(float elapsedTime);

	void MoveAndCollide(float elapsedTime);

private:
	float							gravity = 10.0f;
	float							acceleration = 50.0f;
	float							deceleration = 20.0f;
	float							moveSpeed = 6.0f;
	float							jumpSpeed = 10.5f;
	float							cutJumpVelocity = 2.0f;
	float							airControl = 0.3f;
	float							inputMoveX = 0.0f;
	float							inputMoveZ = 0.0f;

	AnimationLayerMask				upperBodyMask;
	float							upperBodyWeight = 0.0f;
	bool							isUpperBodyActive = false;

	// ダッシュ関連
	float dashTimer = 0.0f;
	const float DASH_DURATION = 0.8f;
	const float DASH_SPEED = 10.0f;
	bool isDashJump = false;

	// 攻撃コンボ
	int comboStep = 0;
	bool hasNextComboInput = false;

	// 空中攻撃フラグ
	bool isAirAttack = false;

	// 壁つかまりフラグ
	bool wallClimb = false;

	// 壁の法線
	DirectX::XMFLOAT3 wallNormal = {};

	// 壁キック
	float wallKickLockTimer = 0.0f;
	float wallKickLockDuration = 0.2f;

	// ソードトレイル
	static constexpr float SWORD_COLLISION_RADIUS = 0.12f;
	static constexpr int SWORD_SWEEP_STEPS = 4;
	bool isBladeActive = false;
	bool isTrailActive = false;
	bool prevBladeActive = false;
	bool prevTrailActive = false;
	std::unordered_set<const Enemy*> swordHitEnemies;
	DirectX::XMFLOAT3 debugBladeRoot = { 0, 0, 0 };
	DirectX::XMFLOAT3 debugBladeTip = { 0, 0, 0 };
	DirectX::XMFLOAT3 debugPreviousBladeRoot = { 0, 0, 0 };
	DirectX::XMFLOAT3 debugPreviousBladeTip = { 0, 0, 0 };
	bool hasSwordCollisionDebugData = false;
	bool showSwordCollision = false;

	// 自機AABB関連
	float bodyHalfWidth = 0.4f;
	float bodyHalfDepth = 0.4f;
	float bodyHeight = 1.4f;

	bool showBodyAabb = true;

	// 開始時点のめり込みを確認するためのフラグ。
	bool hasBodyOverlap = false;

	std::shared_ptr<Effect> spawnEffect;
	std::shared_ptr<Effect> slashEffect;
	std::shared_ptr<Effect> explosionEffect;
	std::shared_ptr<Effect> damagedEffect;
	Effekseer::Handle spawnHandle = -1;
	Effekseer::Handle slashHandle = -1;
	Effekseer::Handle explosionHandle = -1;
	Effekseer::Handle damagedHandle = -1;

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


	std::string debugStateNames [static_cast<int>(StateId::EnumCount)] =
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
	};

	void SetState(StateId stateId);

	PlayerTrail trail;

public:
	using TrailData = PlayerTrail::MotionSnapshot;
	const std::deque<TrailData>& GetTrails() const { return trail.GetMotionSnapshots(); }
public:
	class State
	{
	public:
		State(Player* owner) : owner(owner) {}
		virtual ~State() = default;

	public:
		virtual void OnEnter() {}
		virtual void OnExit() {}
		virtual void OnUpdate(float elapsedTime) {}

	protected:
		Player* owner;
	};

	class IdleState : public State
	{
	public:
		IdleState(Player* owner) : State(owner) {}
		void OnEnter() override;
		void OnUpdate(float elapsedTime) override;
	};

	class MoveState : public State
	{
	public:
		MoveState(Player* owner) : State(owner) {}
		void OnEnter() override;
		void OnUpdate(float elapsedTime) override;
	};

	class AttackState : public State
	{
	public:
		AttackState(Player* owner) : State(owner) {}
		void OnEnter() override;
		void OnUpdate(float elapsedTime) override;
	};

	class JumpState : public State
	{
	public:
		JumpState(Player* owner) : State(owner) {}
		void OnEnter() override;
		void OnUpdate(float elapsedTime) override;
	};

	class DashState : public State
	{
	public:
		DashState(Player* owner) : State(owner) {}
		void OnEnter() override;
		void OnUpdate(float elapsedTime) override;
		void OnExit()override;
	};

	class JumpFallState : public State
	{
	public:
		JumpFallState(Player* owner) : State(owner) {}
		void OnEnter() override;
		void OnUpdate(float elapsedTime) override;
	};

	class WallSlideState : public State
	{
	public:
		WallSlideState(Player* owner) : State(owner) {}
		void OnEnter() override;
		void OnUpdate(float elapsedTime) override;
		void OnExit() override;
	private:
		float kickPower = 5.0f;
	};

	class WallKickState : public State
	{
	public:
		WallKickState(Player* owner) : State(owner) {}
		void OnEnter() override;
		void OnUpdate(float elapsedTime) override;
	};

	class DamageState : public State
	{
	public:
		DamageState(Player* owner) : State(owner) {}
		void OnEnter() override;
		void OnUpdate(float elapsedTime) override;
	};

    class DeathState : public State
    {
    public:
        DeathState(Player* owner) : State(owner) {}
        void OnEnter() override;
        void OnUpdate(float elapsedTime) override;
        void OnExit() override;
    private:
        static constexpr float RETRY_DELAY = 3.0f;
        static constexpr float EFFECT_SCALE = 0.2f;
        static constexpr float VIBRATION_POWER = 1.0f;
        static constexpr float VIBRATION_DURATION = 1.5f;
        float timer = 0.0f;
        bool active = false;
        bool sceneRequested = false;
        std::unique_ptr<Effect> deathEffect;
        Effekseer::Handle deathHandle = -1;
    };
    class ClearState : public State
    {
    public:
        ClearState(Player* owner) : State(owner) {}
        void OnEnter() override;
        void OnUpdate(float elapsedTime) override;
        void OnExit() override;
    private:
        static constexpr float CLEAR_AUDIO_TIME = 5.0f;
        static constexpr float EXIT_EFFECT_TIME = 11.0f;
        static constexpr float SCENE_CHANGE_TIME = 13.0f;
        float timer = 0.0f;
        bool clearAudioPlayed = false;
        bool exitStarted = false;
        bool sceneRequested = false;
        std::unique_ptr<Effect> exitEffect;
        Effekseer::Handle exitHandle = -1;
    };
	class SpawnState : public State
	{
	public:
		SpawnState(Player* owner) : State(owner) {}
		void OnEnter() override;
		void OnUpdate(float elapsedTime) override;
	private:
		float spawnTimer = 0.0f;
		static constexpr float SPAWN_ANIM_START_TIME = 0.5f;
		static constexpr float SPAWN_END_TIME = 1.0f;
		bool hasSpawned = false;;
	};

	StateId	currentState = StateId::None;
	StateId	nextState = StateId::None;
	std::unique_ptr<State> states[static_cast<size_t>(StateId::EnumCount)];
};
