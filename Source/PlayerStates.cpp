#include "Player.h"
#include "EnemyBoss.h"
#include "Audio/Audio.h"
#include "SceneLoading.h"
#include "SceneClear.h"
#include "GameScene.h"
#include "Camera.h"
#include "InputManager.h"
#include "HitStopManager.h"

namespace
{
	constexpr float GROUND_ATTACK_PLAYBACK_SPEED = 1.3f;
	constexpr float AIR_ATTACK_PLAYBACK_SPEED = 2.0f;
	constexpr float AIR_COMBO_CANCEL_START = 0.30f;
}

void Player::IdleState::OnEnter()
{
	owner->animator->Play(0, "Idle_Seq_0", true);
}

void Player::IdleState::OnUpdate(float elapsedTime)
{
	if (owner->InputMove())
	{
		owner->SetState(StateId::Move);
	}
	if (owner->InputJump())
	{
		owner->SetState(StateId::Jump);
	}
	if (owner->InputDash() && owner->IsGround())
	{
		owner->SetState(StateId::Dash);
	}
	if (owner->InputShot())
	{
		owner->SetState(StateId::Attack);
	}
	if (owner->wallKickLockTimer <= 0.0f)
	{
		if (owner->WallJudgement(elapsedTime) && owner->InputTowardWall())
		{
			owner->isDashJump = false;
			owner->SetState(StateId::WallSlide);
			return;
		}
	}
	owner->InputShot();
}

void Player::MoveState::OnEnter()
{
	owner->animator->Play(0, "Run_Fast_Loop_Seq_0", true, 0.1f);
	Audio::Instance().Play("SE_PLAYER_RUN");
}

void Player::MoveState::OnUpdate(float elapsedTime)
{
	if (!owner->InputMove())
	{
		Audio::Instance().Stop("SE_PLAYER_RUN");
		owner->SetState(StateId::Idle);
		owner->velocity.x = 0.0f;
		return;
	}
	if (owner->InputJump())
	{
		Audio::Instance().Stop("SE_PLAYER_RUN");
		owner->SetState(StateId::Jump);
		return;
	}
	if (owner->InputDash() && owner->IsGround())
	{
		Audio::Instance().Stop("SE_PLAYER_RUN");
		owner->SetState(StateId::Dash);
		return;
	}
	if (owner->wallKickLockTimer <= 0.0f)
	{
		if (owner->WallJudgement(elapsedTime) && owner->InputTowardWall())
		{
			owner->isDashJump = false;
			owner->SetState(StateId::WallSlide);
			return;
		}
	}

	owner->InputShot();
}

void Player::JumpState::OnEnter()
{
	owner->animator->Play(0, "Jump_Start_0_Seq_0", false, 0.1f, 0.2f);
	Audio::Instance().Play("SE_PLAYER_JUMP");
}

void Player::JumpState::OnUpdate(float elapsedTime)
{
	owner->InputMove();

	owner->HoldJump();

	if (owner->isDashJump)
	{
		owner->UpdateMotionTrail(elapsedTime);
	}

	if (owner->animator->GetProgress(0))
	{
		owner->SetState(StateId::JumpFall);
		return;
	}
}

void Player::JumpFallState::OnEnter()
{
	owner->animator->Play(0, "Jump_Loop_0_Seq_0", true, 0.05f);
}

void Player::JumpFallState::OnUpdate(float elapsedTime)
{
	bool move = owner->InputMove();

	owner->HoldJump();

	if (owner->WallJudgement(elapsedTime) && owner->InputTowardWall())
	{
		owner->isDashJump = false;
		owner->SetState(StateId::WallSlide);
		return;
	}

	if (owner->isDashJump)
	{
		owner->UpdateMotionTrail(elapsedTime);
	}


	if (owner->IsGround())
	{
		owner->trail.ClearMotion();

		if (move)
		{
			owner->isDashJump = false;
			owner->SetState(StateId::Move);
		}
		else
		{
			owner->isDashJump = false;
			owner->SetState(StateId::Idle);
		}
	}
	else
	{
		if (owner->InputShot())
		{
			owner->trail.ClearMotion();
		};
	}

	if (owner->InputDash())
	{
		owner->SetState(StateId::Dash);
		return;
	}
}

void Player::DashState::OnEnter()
{
	owner->animator->Play(0, "ComboAttack0204Seq", false, 0.1f, 0.0f, 0.5f);
	Audio::Instance().Play("SE_PLAYER_DASH");
	owner->dashTimer = owner->DASH_DURATION;

	owner->velocity.x = 0.0f;
	owner->inputMoveX = 0.0f;
}

void Player::DashState::OnUpdate(float elapsedTime)
{
	owner->UpdateDash();
	owner->dashTimer -= elapsedTime;
	owner->UpdateMotionTrail(elapsedTime);

	InputManager& gamePad = InputManager::Instance();

	if ((gamePad.GetButtonUp() & InputManager::BTN_B) || owner->dashTimer <= 0.0f)
	{
		owner->trail.ClearMotion();
		owner->velocity.x = 0.0f;
		owner->velocity.z = 0.0f;
		Audio::Instance().Stop("SE_PLAYER_DASH");
		owner->SetState(StateId::Idle);
		return;
	}

	if (owner->InputJump())
	{
		owner->isDashJump = true;

		if (!owner->InputMove())
		{
			owner->velocity.x = 0.0f;
			owner->velocity.z = 0.0f;
		}
		Audio::Instance().Stop("SE_PLAYER_DASH");
		owner->SetState(StateId::Jump);
	}

	if (!owner->IsGround() && owner->InputShot())
	{
		owner->trail.ClearMotion();
		Audio::Instance().Stop("SE_PLAYER_DASH");
		return;
	}
}

void Player::DashState::OnExit()
{
	owner->useGravity = true;
}

void Player::WallSlideState::OnEnter()
{
	owner->animator->Play(0, "WallSlide_R", false, 0.1f, 0.0f, 0.3f);
	Audio::Instance().Play("SE_PLAYER_WALLSLIDE");
	owner->wallClimb = true;
	owner->trail.ClearMotion();
	owner->velocity.y = 0;
}

void Player::WallSlideState::OnUpdate(float elapsedTime)
{
	if (owner->IsGround())
	{
		owner->wallClimb = false;
		Audio::Instance().Stop("SE_PLAYER_WALLSLIDE");
		owner->SetState(StateId::Idle);
		return;
	}

	if (!owner->WallJudgement(elapsedTime))
	{
		owner->wallClimb = false;
		Audio::Instance().Stop("SE_PLAYER_WALLSLIDE");
		owner->SetState(StateId::JumpFall);
		return;
	}

	if (owner->InputAwayFromWall())
	{
		owner->wallClimb = false;
		Audio::Instance().Stop("SE_PLAYER_WALLSLIDE");
		owner->SetState(StateId::JumpFall);
		return;
	}

	if (owner->InputJump())
	{
		owner->wallClimb = false;

		owner->velocity.x = owner->wallNormal.x * kickPower;
		owner->wallKickLockTimer = owner->wallKickLockDuration;
		owner->rotation.y = atan2f(owner->wallNormal.x, owner->wallNormal.z);

		owner->SetState(StateId::WallKick);
		return;
	}
}

void Player::WallSlideState::OnExit()
{
	owner->wallClimb = false;
}

void Player::WallKickState::OnEnter()
{
	owner->animator->Play(0, "Jump_Start_0_Seq_0", false);
	Audio::Instance().Stop("SE_PLAYER_WALLSLIDE");
	Audio::Instance().Play("SE_PLAYER_WALLJUMP");

}

void Player::WallKickState::OnUpdate(float elapsedTime)
{
	if (owner->isDashJump)
	{
		owner->UpdateMotionTrail(elapsedTime);
	}

	bool move = false;

	if (owner->wallKickLockTimer <= 0.0f)
	{
		move = owner->InputMove();

		if (owner->WallJudgement(elapsedTime) && owner->InputTowardWall())
		{
			owner->isDashJump = false;
			owner->SetState(StateId::WallSlide);
			return;
		}
	}

	if (owner->IsGround())
	{
		owner->isDashJump = false;
		owner->trail.ClearMotion();
		if (owner->InputMove())
		{
			owner->isDashJump = false;
			owner->SetState(StateId::Move);
		}
		else
		{
			owner->isDashJump = false;
			owner->SetState(StateId::Idle);
		}
	}
}

void Player::AttackState::OnEnter()
{
	owner->comboStep = 1;
	owner->hasNextComboInput = false;

	owner->isUpperBodyActive = false;
	// Each swing can hit an enemy once; the next swing starts a new hit list.
	owner->swordHitEnemies.clear();
	owner->prevBladeActive = false;
	owner->animator->Play(0, "ComboAttack0301Seq", false, 0.1f);
	Audio::Instance().Play("SE_PLAYER_SWISH");
	owner->animator->SetLayerSpeed(
		0,
		owner->IsGround() ? GROUND_ATTACK_PLAYBACK_SPEED : AIR_ATTACK_PLAYBACK_SPEED);

	// 攻撃開始時に移動をリセット
	owner->velocity.x = 0.0f;
	owner->velocity.z = 0.0f;
	owner->inputMoveX = 0.0f;
	owner->inputMoveZ = 0.0f;

	owner->SetBladeActive(true);
	owner->SetTrailActive(false);

	owner->isAirAttack = !owner->IsGround();
}

void Player::AttackState::OnUpdate(float elapsedTime)
{
	InputManager& gamePad = InputManager::Instance();
	bool isGrounded = owner->IsGround();

	if (owner->isAirAttack && isGrounded)
	{
		owner->SetBladeActive(false);
		owner->SetTrailActive(false);
		owner->animator->SetLayerSpeed(0, 1.0f);
		owner->SetState(StateId::Idle);
		return;
	}

	owner->isAirAttack = !isGrounded;

	float progress = owner->animator->GetProgress(0);

	// 振りの出だし（例: progress >= 0.15f）以降ならいつでもダッシュやジャンプで硬直をキャンセル可能にする
	if (progress >= 0.15f)
	{
		if (owner->InputDash())
		{
			owner->SetBladeActive(false);
			owner->SetTrailActive(false);
			owner->animator->SetLayerSpeed(0, 1.0f);

			// 地上ならダッシュ、空中なら空中ダッシュ（またはジャンプ落下）へ
			if (isGrounded)
			{
				owner->SetState(StateId::Dash);
			}
			return;
		}

		if (owner->InputJump())
		{
			owner->SetBladeActive(false);
			owner->SetTrailActive(false);
			owner->animator->SetLayerSpeed(0, 1.0f);
			owner->SetState(StateId::Jump);
			return;
		}
	}

	// 攻撃中（1～3段目）のみコンボの先行入力を受け付ける
	if (owner->comboStep >= 1 && owner->comboStep <= 3)
	{
		if (gamePad.GetButtonDown() & InputManager::BTN_X)
		{
			owner->hasNextComboInput = true;
			Audio::Instance().Play("SE_PLAYER_SWISH");
		}
	}

	bool isPlaying = owner->animator->IsPlaying(0);

	// 振り抜き中だけをトレイルとして表示
	const bool isComboSwing = owner->comboStep >= 1 && owner->comboStep <= 3
		&& progress >= 0.05f && progress < 0.30f;
	owner->SetTrailActive(isComboSwing);

	// 攻撃モーション
	if (owner->comboStep >= 1 && owner->comboStep <= 3)
	{
		const float comboCancelStart = isGrounded ? 0.2f : AIR_COMBO_CANCEL_START;
		const float comboCancelEnd = isGrounded ? 0.65f : 0.80f;

		if (owner->hasNextComboInput && progress >= comboCancelStart && progress <= comboCancelEnd)
		{
			owner->hasNextComboInput = false;

			if (isGrounded)
			{
				if (owner->comboStep == 1)
				{
					owner->comboStep = 2;
					// Each swing can hit an enemy once; the next swing starts a new hit list.
					owner->swordHitEnemies.clear();
					owner->prevBladeActive = false;
					owner->animator->Play(0, "ComboAttack0302Seq", false, 0.03f);
				}
				else if (owner->comboStep == 2)
				{
					owner->comboStep = 3;
					// Each swing can hit an enemy once; the next swing starts a new hit list.
					owner->swordHitEnemies.clear();
					owner->prevBladeActive = false;
					owner->animator->Play(0, "ComboAttack0401Seq", false, 0.03f);
				}
			}
			else
			{
				owner->comboStep = 1;
				// Each swing can hit an enemy once; the next swing starts a new hit list.
				owner->swordHitEnemies.clear();
				owner->prevBladeActive = false;
				owner->animator->Play(0, "ComboAttack0301Seq", false, 0.03f);
			}
		}
		else if (progress >= 0.45f || !isPlaying)
		{
			// 次の入力がなければ納刀を挟まずに直接Idleへ戻す（硬直カット）
			owner->SetBladeActive(false);
			owner->SetTrailActive(false);
			owner->animator->SetLayerSpeed(0, 1.0f);

			// 移動入力があればそのままMove、無ければIdle
			if (owner->InputMove())
			{
				owner->SetState(StateId::Move);
			}
			else
			{
				owner->SetState(StateId::Idle);
			}
		}
	}
}

void Player::ClearState::OnEnter()
{
    OnExit();
    timer = 0.0f;
    clearAudioPlayed = exitStarted = sceneRequested = false;
    EnemyBoss::SetDefeat(false);
    owner->velocity = { 0.0f, 0.0f, 0.0f };
    owner->invincibleTime = SCENE_CHANGE_TIME;
    owner->inputMoveX = owner->inputMoveZ = 0.0f;
    owner->SetBladeActive(false);
    owner->SetTrailActive(false);
    owner->prevBladeActive = owner->prevTrailActive = false;
    owner->trail.ClearMotion();
    owner->isUpperBodyActive = false;
    owner->upperBodyWeight = 0.0f;
    owner->animator->SetLayerState(1, 0.0f, AnimationBlendMode::Override);
    owner->animator->Play(0, "Idle_Seq_0", true);
    Audio::Instance().Stop("SE_PLAYER_DAMAGE_SPARK");
    Audio::Instance().Stop("SE_PLAYER_RUN");
    Audio::Instance().Stop("SE_PLAYER_CHARGE");
}

void Player::ClearState::OnUpdate(float elapsedTime)
{
    const float step = (std::max)(elapsedTime, 0.0f);
    timer += step;
    if (!exitStarted)
    {
        owner->velocity.x = owner->velocity.z = 0.0f;
        owner->velocity.y -= owner->gravity * step;
        owner->MoveAndCollide(step);
    }
    if (timer >= CLEAR_AUDIO_TIME && !clearAudioPlayed)
    {
        clearAudioPlayed = true;
        Audio::Instance().Play("SE_CLEAR");
    }
    if (timer >= EXIT_EFFECT_TIME && !exitStarted)
    {
        exitStarted = true;
        if (!exitEffect) exitEffect = std::make_unique<Effect>("Data/Effect/ExitEffect.efkefc");
        exitHandle = exitEffect->Play(owner->position);
        Audio::Instance().Play("SE_PLAYER_RETURN");
        owner->scale = { 0.0f, 0.0f, 0.0f };
        owner->velocity = { 0.0f, 0.0f, 0.0f };
    }
    if (timer >= SCENE_CHANGE_TIME && !sceneRequested)
    {
        sceneRequested = true;
        SceneManager::Instance().ChangeScene([]() {
            return std::make_shared<SceneLoading>([]() { return std::make_shared<SceneClear>(); });
        });
    }
}

void Player::ClearState::OnExit()
{
    if (exitEffect && exitHandle >= 0) exitEffect->Stop(exitHandle);
    exitHandle = -1;
}

void Player::DeathState::OnEnter()
{
    OnExit();
    active = true;
    sceneRequested = false;
    timer = 0.0f;
    owner->scale = { 0.0f, 0.0f, 0.0f };
    owner->velocity = { 0.0f, 0.0f, 0.0f };
    owner->inputMoveX = owner->inputMoveZ = 0.0f;
    owner->SetBladeActive(false);
    owner->SetTrailActive(false);
    owner->prevBladeActive = owner->prevTrailActive = false;
    owner->trail.ClearMotion();
    owner->isUpperBodyActive = false;
    owner->upperBodyWeight = 0.0f;
    owner->animator->SetLayerState(1, 0.0f, AnimationBlendMode::Override);
    if (!deathEffect) deathEffect = std::make_unique<Effect>("Data/Effect/Death.efkefc");
    deathHandle = deathEffect->Play(owner->position, EFFECT_SCALE);
    Audio::Instance().StopAll();
    Audio::Instance().Play("SE_DEATH_GENERIC");
    InputManager::Instance().GetGamePad().Vibrate(VIBRATION_POWER, VIBRATION_POWER, VIBRATION_DURATION);
}

void Player::DamageState::OnEnter()
{
	Camera::Instance().StartShake(0.3f, 0.15f);
	HitStopManager::Instance().Request(0.1f);
	owner->animator->Play(0, "Hit_Combat_F_Seq_0", false);
	Audio::Instance().Play("SE_PLAYER_DAMAGE");

	if (!owner->damagedEffect)
		owner->damagedEffect = std::make_unique<Effect>("Data/Effect/thunder.efkefc");

	InputManager::Instance().GetGamePad().Vibrate(0.3f, 0.3f, 0.3f);

	owner->damagedHandle = owner->damagedEffect->Play({ owner->position.x, owner->position.y + owner->height * 0.5f, owner->position.z }, 0.08f);
}

void Player::DamageState::OnUpdate(float elapsedTime)
{
	owner->damagedEffect->SetPosition(owner->damagedHandle,
		{ owner->position.x, owner->position.y + owner->height * 0.5f, owner->position.z });

	if (!owner->animator->IsPlaying(0))
	{
		owner->SetState(StateId::Idle);
		owner->damagedEffect->Stop(owner->damagedHandle);
	}
}

void Player::DeathState::OnUpdate(float elapsedTime)
{
    if (!active || sceneRequested) return;
    timer += (std::max)(elapsedTime, 0.0f);

	InputManager::Instance().GetGamePad().Vibrate(0.5f, 0.5f, 0.3f);
    if (timer >= RETRY_DELAY)
    {
        sceneRequested = true;
        OnExit();
        SceneManager::Instance().ChangeScene([]() {
            return std::make_shared<SceneLoading>([]() { return std::make_shared<GameScene>(); });
        });
    }
}

void Player::DeathState::OnExit()
{
    if (deathEffect && deathHandle >= 0) deathEffect->Stop(deathHandle);
    deathHandle = -1;
    if (active) InputManager::Instance().GetGamePad().StopVibration();
    active = false;
}

void Player::SpawnState::OnEnter()
{
    // Effekseer uses the immediate context; load on the main update thread.
    if (!owner->spawnEffect)
        owner->spawnEffect = std::make_unique<Effect>("Data/Effect/SpawnEffect.efkefc");

	owner->spawnHandle = owner->spawnEffect->Play(owner->position);
	spawnTimer = 0.0f;
	Audio::Instance().Play("SE_PLAYER_APPEAR");
}

void Player::SpawnState::OnUpdate(float elapsedTime)
{
	spawnTimer += elapsedTime;

	if (!owner->animator->IsPlaying(0) && spawnTimer > SPAWN_END_TIME)
	{
		owner->SetState(StateId::Idle);
		Audio::Instance().Play("SE_PLAYER_AFTER_APPEAR");
	}
	else if (spawnTimer > SPAWN_ANIM_START_TIME && !hasSpawned)
	{
		owner->scale = { 0.01f, 0.01f, 0.01f };
		owner->animator->Play(0, "Spawn", false);
		Audio::Instance().Play("SE_PLAYER_APPEAR_LANDING");
		hasSpawned = true;
	}
}
