#include "ModelManager.h"
#include "EnemyBoss.h"
#include "Audio/Audio.h"

#include <algorithm>
#include <cmath>
#include <cstdlib>

#include "Player.h"
#include "ShapeRenderer.h"

bool EnemyBoss::defeatFlag = false;

// コンストラクタ
EnemyBoss::EnemyBoss(ID3D11Device* device, Player* player) : player(player)
{
    model = ModelManager::Instance().CreateInstance(device, "Data/Model/Enemy/Boss.gltf");
    scale = {0.015f, 0.015f, 0.015f};
    rotation.y = DirectX::XMConvertToRadians(-90.0f);
    radius = 0.65f;
    height = 2.0f;
    health = maxHealth = 20;
    useGravity = true;
    gravityAcceleration = BOSS_GRAVITY_ACCELERATION;
    color = {1.0f, 0.35f, 0.2f, 1.0f};
    InitializeAnimator();
    UpdateTransform();
    defeatFlag = false;

    states[static_cast<size_t>(StateId::Idle)] = std::make_unique<IdleState>(this);
    states[static_cast<size_t>(StateId::JumpMove)] = std::make_unique<JumpMoveState>(this);
    states[static_cast<size_t>(StateId::Attack)] = std::make_unique<AttackState>(this);
    states[static_cast<size_t>(StateId::ChargeAttack)] = std::make_unique<ChargeAttackState>(this);
    states[static_cast<size_t>(StateId::Continuous)] = std::make_unique<ContinuousState>(this);
    states[static_cast<size_t>(StateId::Tornado)] = std::make_unique<TornadoState>(this);
    states[static_cast<size_t>(StateId::Death)] = std::make_unique<DeathState>(this);

    SetState(StateId::Idle);
}

void EnemyBoss::SetActionEnable(bool value)
{
    actionEnabled = value;

    if (!value)
    {
        velocity = {0.0f, 0.0f, 0.0f};
        StopBackWind();

        if (animator)
        {
            animator->Update(0.0f);
        }

        UpdateTransform();
    }
}

void EnemyBoss::SetArenaBounds(float minX, float maxX)
{
    arenaMinX = (std::min)(minX, maxX);
    arenaMaxX = (std::max)(minX, maxX);

    arenaCenterX = (arenaMinX + arenaMaxX) * 0.5f;
    arenaInitialized = true;
}

// 更新処理
void EnemyBoss::Update(float elapsedTime)
{
    if (!player)
    {
        Destroy();
        return;
    }

    if (!arenaInitialized)
    {
        SetArenaBounds(position.x - DEFAULT_ARENA_HALF_WIDTH, position.x + DEFAULT_ARENA_HALF_WIDTH);
    }

    if (actionEnabled || IsDead())
    {
        UpdateStateMachine(elapsedTime);
        UpdateBackWind(elapsedTime);
    }
    else
    {
        StopBackWind();
    }

    if (!IsDead())
    {
        DamagePlayerOnContact(*player, 1, 1.2f);
    }

    UpdateProjectiles(elapsedTime);
    UpdateGroundPhysics(elapsedTime);
    position.x = (std::clamp)(position.x, arenaMinX, arenaMaxX);
    UpdateInvincibleTimer(elapsedTime);

    if (animator)
    {
        animator->Update(elapsedTime);
    }

    UpdateTransform();
}

// デストラクタ
EnemyBoss::~EnemyBoss()
{
    if (currentState == StateId::ChargeAttack)
    {
        states[static_cast<size_t>(StateId::ChargeAttack)]->OnExit();
    }

    StopBackWind();

    if (states[static_cast<size_t>(StateId::Tornado)])
    {
        states[static_cast<size_t>(StateId::Tornado)]->OnExit();
    }

    states[static_cast<size_t>(StateId::Death)]->OnExit();
}

// 状態遷移と現在の状態の更新処理
void EnemyBoss::UpdateStateMachine(float elapsedTime)
{
    if (nextState != StateId::None)
    {
        StopBackWind();

        if (currentState != StateId::None)
        {
            states[static_cast<size_t>(currentState)]->OnExit();
        }

        currentState = nextState;
        nextState = StateId::None;
        states[static_cast<size_t>(currentState)]->OnEnter();
    }

    if (currentState != StateId::None)
    {
        states[static_cast<size_t>(currentState)]->OnUpdate(elapsedTime);
    }
}

void EnemyBoss::StartBackWind(float delay, bool repeat)
{
    StopBackWind();
    backWindRepeat = repeat;
    backWindPhase = delay > 0.0f ? BackWindPhase::Delay : BackWindPhase::Warning;
    backWindTime = delay > 0.0f ? delay : BACK_WIND_WARNING_DURATION;

    if (backWindPhase == BackWindPhase::Warning)
    {
        UpdateBackWindVisual(true);
    }
}

void EnemyBoss::StopBackWind()
{
    if (backWindEffect && backWindHandle >= 0)
    {
        backWindEffect->Stop(backWindHandle);
    }

    backWindHandle = -1;
    backWindPhase = BackWindPhase::Off;
    backWindTime = 0.0f;
    backWindRepeat = false;
}

// UpdateBackWindVisual の更新処理
void EnemyBoss::UpdateBackWindVisual(bool warning)
{
    if (!backWindEffect)
    {
        backWindEffect = std::make_unique<Effect>("Data/Effect/BossBackWind.efkefc");
    }

    const float direction = position.x >= arenaCenterX ? -1.0f : 1.0f;

    const DirectX::XMFLOAT3 effectScale = {
        BACK_WIND_EFFECT_LENGTH_SCALE, warning ? 0.7f : BACK_WIND_EFFECT_HEIGHT_SCALE, warning ? 0.6f : 1.5f};
    const DirectX::XMFLOAT3 origin = {position.x - direction * BACK_WIND_EFFECT_OFFSET,
                                      position.y + height * 0.35f, position.z};

    if (!backWindEffect->IsPlaying(backWindHandle))
    {
        backWindHandle = backWindEffect->Play(origin);
    }

    backWindEffect->SetPosition(backWindHandle, origin);
    backWindEffect->SetScale(backWindHandle, effectScale);

    backWindEffect->SetRotation(backWindHandle, {0.0f, direction < 0.0f ? DirectX::XM_PI : 0.0f, 0.0f});
}

void EnemyBoss::ApplyBackWindPush(float elapsedTime)
{
    const auto& playerPosition = player->GetPosition();
    const float direction = position.x >= arenaCenterX ? -1.0f : 1.0f;
    const float forwardDistance = (playerPosition.x - position.x) * direction;

    if (forwardDistance < -BACK_WIND_RANGE - player->GetRadius() ||
        forwardDistance > BACK_WIND_FRONT_MARGIN ||
        std::abs(playerPosition.z - position.z) > radius + player->GetRadius() ||
        playerPosition.y >= position.y + height || playerPosition.y + player->GetHeight() <= position.y)
    {
        return;
    }

    const float forwardSpeed = player->GetVelocity().x * direction;

    if (forwardSpeed >= BACK_WIND_MAX_SPEED)
    {
        return;
    }

    const float impulse =
        (std::min)(BACK_WIND_ACCELERATION * elapsedTime, BACK_WIND_MAX_SPEED - forwardSpeed);

    player->AddImpulse({direction * impulse, 0.0f, 0.0f});
}

// UpdateBackWind の更新処理
void EnemyBoss::UpdateBackWind(float elapsedTime)
{
    if (backWindPhase == BackWindPhase::Off)
    {
        return;
    }

    if (IsDead() || !player || player->IsDead())
    {
        StopBackWind();
        return;
    }

    float remaining = (std::max)(elapsedTime, 0.0f);

    while (remaining > 0.0f && backWindPhase != BackWindPhase::Off)
    {
        const float step = (std::min)(remaining, backWindTime);

        if (backWindPhase == BackWindPhase::Burst)
        {
            ApplyBackWindPush(step);
        }

        backWindTime -= step;
        remaining -= step;

        if (backWindTime > 0.0f)
        {
            break;
        }

        if (backWindPhase == BackWindPhase::Delay)
        {
            backWindPhase = BackWindPhase::Warning;
            backWindTime = BACK_WIND_WARNING_DURATION;
        }
        else if (backWindPhase == BackWindPhase::Warning)
        {
            if (backWindEffect)
            {
                backWindEffect->Stop(backWindHandle);
            }

            backWindHandle = -1;
            backWindPhase = BackWindPhase::Burst;
            backWindTime = BACK_WIND_BURST_DURATION;
        }
        else if (backWindRepeat)
        {
            if (backWindEffect)
            {
                backWindEffect->Stop(backWindHandle);
            }

            backWindHandle = -1;
            backWindPhase = BackWindPhase::Delay;
            backWindTime = BACK_WIND_PAUSE_DURATION;
        }
        else
        {
            StopBackWind();
        }
    }

    if (backWindPhase == BackWindPhase::Warning || backWindPhase == BackWindPhase::Burst)
    {
        UpdateBackWindVisual(backWindPhase == BackWindPhase::Warning);
    }
}

void EnemyBoss::FireAtPlayer(float angleOffset)
{
    const DirectX::XMFLOAT3 start = {position.x, position.y + height * 0.65f, position.z};
    DirectX::XMFLOAT3 direction = {player->GetPosition().x - start.x,
                                   player->GetPosition().y + player->GetHeight() * 0.5f - start.y,
                                   player->GetPosition().z - start.z};
    DirectX::XMVECTOR vector = DirectX::XMVector3Normalize(DirectX::XMLoadFloat3(&direction));
    DirectX::XMStoreFloat3(&direction, vector);
    const float cosine = std::cos(angleOffset);
    const float sine = std::sin(angleOffset);
    const float x = direction.x * cosine - direction.y * sine;
    const float y = direction.x * sine + direction.y * cosine;
    projectiles.emplace_back(start, DirectX::XMFLOAT3{x * 7.0f, y * 7.0f, direction.z * 7.0f}, 0.25f, 6.0f,
                             1);
}

// UpdateProjectiles の更新処理
void EnemyBoss::UpdateProjectiles(float elapsedTime)
{
    for (auto& projectile : sideProjectiles)
    {
        projectile->Update(elapsedTime);
        projectile->HitPlayer(*player);
    }

    sideProjectiles.erase(std::remove_if(sideProjectiles.begin(), sideProjectiles.end(),
                                         [](const auto& projectile) { return !projectile->IsActive(); }),
                          sideProjectiles.end());

    for (auto& projectile : projectiles)
    {
        projectile.Update(elapsedTime);
        projectile.HitPlayer(*player, 1.2f);
    }

    projectiles.erase(std::remove_if(projectiles.begin(), projectiles.end(),
                                     [](const EnemyProjectile& projectile)
                                     { return !projectile.IsActive(); }),
                      projectiles.end());
}

void EnemyBoss::OnDead()
{
    if (currentState == StateId::ChargeAttack)
    {
        states[static_cast<size_t>(StateId::ChargeAttack)]->OnExit();
    }

    StopBackWind();

    states[static_cast<size_t>(StateId::Tornado)]->OnExit();
    sideProjectiles.clear();
    projectiles.clear();
    SetState(StateId::Death);
    useGravity = false;
    velocity = {0.0f, 0.0f, 0.0f};
    defeatFlag = true;
}

// DrawPrimitive の描画処理
void EnemyBoss::DrawPrimitive(ShapeRenderer* shapeRenderer) const
{
    for (const auto& projectile : sideProjectiles)
    {
        projectile->DrawDebugPrimitive(shapeRenderer);
    }

    for (const auto& projectile : projectiles)
    {
        projectile.DrawDebugPrimitive(shapeRenderer);
    }

    const auto* tornado =
        static_cast<const TornadoState*>(states[static_cast<size_t>(StateId::Tornado)].get());
    tornado->DrawDebugPrimitive(shapeRenderer);
}

void EnemyBoss::SetState(StateId stateId)
{
    nextState = stateId;
}

// 状態に入るときの処理
void EnemyBoss::IdleState::OnEnter()
{
    owner->stateTimer = owner->nextIdleDuration >= 0.0f
                            ? owner->nextIdleDuration
                            : (owner->GetHealth() <= owner->GetMaxHealth() / 2 ? 0.8f : 1.3f);
    owner->nextIdleDuration = -1.0f;
    owner->PlayAnimation(0, true);
}

// 現在の状態の更新処理
void EnemyBoss::IdleState::OnUpdate(float elapsedTime)
{
    owner->stateTimer -= elapsedTime;

    if (owner->stateTimer <= 0.0f)
    {
        SelectNextState();
    }
}

void EnemyBoss::IdleState::SelectNextState()
{
    owner->candidates.clear();

    if (owner->lastAction == StateId::Tornado)
    {
        owner->nextState = StateId::Continuous;
    }
    else
    {
        owner->candidates.push_back(StateId::JumpMove);
        owner->candidates.push_back(StateId::Attack);
        owner->candidates.push_back(StateId::ChargeAttack);

        if (owner->health <= owner->maxHealth * 0.5f && owner->lastAction != StateId::Continuous)
        {
            owner->candidates.push_back(StateId::Tornado);
        }

        bool isValid = false;
        int safetyLoop = 0;
        do
        {
            int index = rand() % owner->candidates.size();
            owner->nextState = owner->candidates[index];
            isValid = true;

            if (owner->nextState == StateId::Attack)
            {
                if (owner->lastAction == StateId::Attack && owner->consecutiveAttackCount >= 2)
                {
                    isValid = false;
                }
            }
            else
            {
                if (owner->nextState == owner->lastAction)
                {
                    isValid = false;
                }
            }

            safetyLoop++;
        } while (!isValid && owner->candidates.size() > 1 && safetyLoop < 100);
    }

    owner->consecutiveAttackCount =
        (owner->nextState == StateId::Attack) ? owner->consecutiveAttackCount + 1 : 0;
    owner->lastAction = owner->nextState;

    owner->SetState(owner->nextState);
}

// 状態に入るときの処理
void EnemyBoss::JumpMoveState::OnEnter()
{
    owner->useGravity = true;
    owner->gravityAcceleration = BOSS_GRAVITY_ACCELERATION;
    phase = Phase::Ready;
    jumpCount = 0;
    jumpDelay = 0.0f;

    if (!owner->arenaInitialized)
    {
        owner->SetArenaBounds(owner->position.x - DEFAULT_ARENA_HALF_WIDTH,
                              owner->position.x + DEFAULT_ARENA_HALF_WIDTH);
    }

    moveStartX = owner->position.x;
    owner->velocity = {0.0f, 0.0f, 0.0f};

    const float centerX = owner->arenaCenterX;
    const bool moveRight =
        moveStartX < centerX || (moveStartX == centerX && std::sin(owner->rotation.y) > 0.0f);

    const float edgeOffset =
        (std::min)(owner->radius + BACKSIDE_CLEARANCE, (owner->arenaMaxX - owner->arenaMinX) * 0.5f);
    moveTargetX = moveRight ? owner->arenaMaxX - edgeOffset : owner->arenaMinX + edgeOffset;
    jumpTargetX = moveStartX;
}

// 現在の状態の更新処理
void EnemyBoss::JumpMoveState::OnUpdate(float elapsedTime)
{
    switch (phase)
    {
    case Phase::Ready:
    {

        jumpTargetX =
            moveStartX + (moveTargetX - moveStartX) * static_cast<float>(jumpCount + 1) / JUMP_COUNT_LIMIT;
        const float jumpPower = JUMP_POWERS[jumpCount];
        const float flightTime = 2.0f * jumpPower / owner->gravityAcceleration;
        owner->velocity.x = (jumpTargetX - owner->position.x) / flightTime;
        owner->velocity.y = jumpPower;
        owner->isGround = false;
        owner->PlayAnimation(JUMP_START_ANIMATION, false);
        Audio::Instance().Play(JUMP_SOUND);
        phase = Phase::Airborne;
        break;
    }

    case Phase::Airborne:
        if (!owner->IsGround())
        {
            break;
        }

        owner->position.x = jumpTargetX;
        owner->velocity.x = 0.0f;
        ++jumpCount;
        owner->PlayAnimation(IDLE_ANIMATION, false);
        Audio::Instance().Play(LANDING_SOUND);

        if (jumpCount >= JUMP_COUNT_LIMIT)
        {
            phase = Phase::Completed;
            owner->rotation.y += DirectX::XMConvertToRadians(TURN_ANGLE_DEGREES);
            transitionTimer = TRANSITION_IDLE;
        }
        else
        {
            jumpDelay =
                owner->GetHealth() <= LOW_HEALTH_THRESHOLD ? LOW_HEALTH_JUMP_DELAY : NORMAL_JUMP_DELAY;
            phase = Phase::Waiting;
        }

        break;

    case Phase::Waiting:
        jumpDelay -= elapsedTime;

        if (jumpDelay <= 0.0f)
        {
            phase = Phase::Ready;
        }

        break;

    case Phase::Completed:
        transitionTimer -= elapsedTime;

        if (transitionTimer <= 0.0f)
        {
            owner->nextIdleDuration = 0.0f;
            owner->SetState(StateId::Idle);
        }

        break;
    }
}

// 状態に入るときの処理
void EnemyBoss::AttackState::OnEnter()
{
    phase = Phase::Windup;
    phaseTime = 0.0f;

    if (!owner->arenaInitialized)
    {
        owner->SetArenaBounds(owner->position.x - DEFAULT_ARENA_HALF_WIDTH,
                              owner->position.x + DEFAULT_ARENA_HALF_WIDTH);
    }

    directionX = owner->position.x >= owner->arenaCenterX ? -1.0f : 1.0f;
    owner->rotation.y = DirectX::XMConvertToRadians(directionX > 0.0f ? 90.0f : -90.0f);
    owner->velocity.x = 0.0f;
    owner->PlayAnimation(ATTACK_ANIMATION, false);
    Audio::Instance().Play(ATTACK_SOUND);
}

// 現在の状態の更新処理
void EnemyBoss::AttackState::OnUpdate(float elapsedTime)
{
    phaseTime += (std::max)(elapsedTime, 0.0f);

    switch (phase)
    {
    case Phase::Windup:
        if (phaseTime >= WINDUP_DURATION)
        {
            phase = Phase::Fire;
            phaseTime = 0.0f;
        }

        break;
    case Phase::Fire:
        FireSideProjectiles();
        owner->StartBackWind(0.0f, false);
        phase = Phase::Recovery;
        phaseTime = 0.0f;
        break;
    case Phase::Recovery:
        if (phaseTime >= RECOVERY_DURATION)
        {
            phase = Phase::Completed;
            owner->nextIdleDuration = 0.0f;
            owner->SetState(StateId::Idle);
        }

        break;
    case Phase::Completed:
        break;
    }
}

void EnemyBoss::AttackState::FireSideProjectiles()
{
    const auto& pattern = PATTERNS[std::rand() % PATTERN_COUNT];
    const DirectX::XMFLOAT3 start = {
        owner->position.x, owner->position.y + owner->height * MUZZLE_HEIGHT_RATIO, owner->position.z};

    for (const auto& offset : pattern)
    {
        const DirectX::XMFLOAT3 target = {
            owner->position.x + directionX * (SCATTER_BASE_DISTANCE + offset.x * SCATTER_DISTANCE_SCALE),
            owner->position.y + offset.y * SCATTER_HEIGHT_SCALE, owner->position.z};
        owner->sideProjectiles.push_back(std::make_unique<BossSideProjectile>(start, target, directionX));
    }
}

// 状態に入るときの処理
void EnemyBoss::ContinuousState::OnEnter()
{
    timeUntilShot = SHOT_INTERVAL;
    volleysFired = 0;

    if (!owner->arenaInitialized)
    {
        owner->SetArenaBounds(owner->position.x - DEFAULT_ARENA_HALF_WIDTH,
                              owner->position.x + DEFAULT_ARENA_HALF_WIDTH);
    }

    directionX = owner->position.x >= owner->arenaCenterX ? -1.0f : 1.0f;
    owner->rotation.y = DirectX::XMConvertToRadians(directionX > 0.0f ? 90.0f : -90.0f);
    owner->velocity.x = 0.0f;
    owner->velocity.z = 0.0f;
    owner->PlayAnimation(ATTACK_ANIMATION, false);
}

// 現在の状態の更新処理
void EnemyBoss::ContinuousState::OnUpdate(float elapsedTime)
{
    if (volleysFired >= SHOT_COUNT)
    {
        return;
    }

    timeUntilShot -= (std::max)(elapsedTime, 0.0f);

    if (timeUntilShot > 0.0f)
    {
        return;
    }

    FireVolley();
    ++volleysFired;

    if (volleysFired == 2)
    {
        owner->StartBackWind(0.0f, false);
    }

    timeUntilShot = SHOT_INTERVAL;
    owner->PlayAnimation(ATTACK_ANIMATION, false);
    Audio::Instance().Play(ATTACK_SOUND);

    if (volleysFired >= SHOT_COUNT)
    {
        owner->nextIdleDuration = RECOVERY_DURATION;
        owner->PlayAnimation(0, true);
        owner->SetState(StateId::Idle);
    }
}

std::array<DirectX::XMFLOAT3, 5> EnemyBoss::ContinuousState::GenerateSpawnPositions()
{
    std::array<DirectX::XMFLOAT3, SPAWN_SLOT_COUNT> positions;
    std::uniform_real_distribution<float> jitter(-SPAWN_JITTER, SPAWN_JITTER);

    for (int index = 0; index < SPAWN_SLOT_COUNT; ++index)
    {
        const auto& offset = SPAWN_OFFSETS[index];
        positions[index] = {owner->position.x + directionX * (offset.x + jitter(randomEngine)),
                            owner->position.y + offset.y + jitter(randomEngine), owner->position.z};
    }

    std::shuffle(positions.begin(), positions.end(), randomEngine);
    return positions;
}

void EnemyBoss::ContinuousState::FireVolley()
{
    const auto positions = GenerateSpawnPositions();
    std::uniform_int_distribution<int> countDistribution(MIN_PROJECTILES, MAX_PROJECTILES);
    const int count = countDistribution(randomEngine);

    for (int index = 0; index < count; ++index)
    {

        owner->sideProjectiles.push_back(std::make_unique<BossSideProjectile>(
            positions[index], positions[index], directionX, BossSideProjectile::Motion::Continuous));
    }
}

// 状態に入るときの処理
void EnemyBoss::DeathState::OnEnter()
{
    OnExit();
    active = true;
    deathTime = 0.0f;
    nextExplosionTime = POSE_HOLD_TIME;
    buildupStarted = shockStarted = exploded = false;
    basePosition = owner->position;
    owner->velocity = {0.0f, 0.0f, 0.0f};
    owner->useGravity = false;

    if (owner->animator)
    {
        owner->animator->Play(0, DEATH_ANIMATION, false, 0.0f, 0.0f, POSE_HOLD_TIME);
    }

    Audio::Instance().Stop("BGM_BOSS");
    Audio::Instance().Stop("BGM_STAGE");
    Audio::Instance().Stop("SE_BOSS_STORM");
}

void EnemyBoss::DeathState::PlayVisual(Visual visual, const DirectX::XMFLOAT3& position, float scale)
{
    const size_t index = static_cast<size_t>(visual);

    if (!effects[index])
    {
        effects[index] = std::make_unique<Effect>(EFFECT_PATHS[index]);
    }

    const auto handle = effects[index]->Play(position, scale);

    if (handle >= 0)
    {
        effectHandles.emplace_back(visual, handle);
    }
}

void EnemyBoss::DeathState::StopVisuals()
{
    for (const auto& entry : effectHandles)
    {
        effects[static_cast<size_t>(entry.first)]->Stop(entry.second);
    }

    effectHandles.clear();
}

// 現在の状態の更新処理
void EnemyBoss::DeathState::OnUpdate(float elapsedTime)
{
    if (!active)
    {
        return;
    }

    deathTime += (std::max)(elapsedTime, 0.0f);

    if (deathTime < EXPLOSION_TIME)
    {
        const float randomX = static_cast<float>(std::rand()) / RAND_MAX - 0.5f;
        const float randomY = static_cast<float>(std::rand()) / RAND_MAX - 0.5f;
        owner->position = {basePosition.x + randomX * SHAKE_POWER, basePosition.y + randomY * SHAKE_POWER,
                           basePosition.z};

        if (deathTime >= POSE_HOLD_TIME && !buildupStarted)
        {
            buildupStarted = true;
            PlayVisual(Visual::Light, {basePosition.x, basePosition.y + 0.75f, basePosition.z}, 0.3f);
            PlayVisual(Visual::Smoke, {basePosition.x, basePosition.y, basePosition.z - 0.5f}, 3.0f);
            PlayVisual(Visual::Flare, {basePosition.x, basePosition.y, basePosition.z + 0.5f}, 7.0f);
            PlayVisual(Visual::BlackExplosion, {basePosition.x, basePosition.y, basePosition.z - 0.3f}, 1.5f);
            Audio::Instance().Play("SE_BOSS_DEATH");
        }

        if (deathTime >= nextExplosionTime)
        {
            nextExplosionTime = deathTime + SMALL_EXPLOSION_INTERVAL;
            PlayVisual(Visual::SmallExplosion,
                       {basePosition.x + randomX * SMALL_EXPLOSION_WIDTH * 2.0f,
                        basePosition.y + (randomY + 0.5f) * owner->height, basePosition.z - 0.1f});
        }

        if (deathTime >= SHOCK_TIME && !shockStarted)
        {
            shockStarted = true;
            PlayVisual(Visual::Shock, basePosition);
        }
    }

    if (deathTime >= EXPLOSION_TIME && !exploded)
    {
        exploded = true;
        StopVisuals();
        owner->position = basePosition;
        const DirectX::XMFLOAT3 center = {basePosition.x, basePosition.y + owner->height * 0.6f,
                                          basePosition.z};
        PlayVisual(Visual::Explosion, center, 0.5f);
        PlayVisual(Visual::Fire, center);
        Audio::Instance().Stop("SE_BOSS_DEATH");
        Audio::Instance().Play("SE_DEATH_GENERIC");
        owner->scale = {0.0f, 0.0f, 0.0f};
    }

    if (deathTime >= DEATH_DURATION)
    {
        OnExit();
        owner->Destroy();
    }
}

// 状態を抜けるときの処理
void EnemyBoss::DeathState::OnExit()
{
    StopVisuals();

    if (active)
    {
        Audio::Instance().Stop("SE_BOSS_DEATH");
    }

    active = false;
}

// 状態に入るときの処理
void EnemyBoss::TornadoState::OnEnter()
{
    if (!owner->arenaInitialized)
    {
        owner->SetArenaBounds(owner->position.x - DEFAULT_ARENA_HALF_WIDTH,
                              owner->position.x + DEFAULT_ARENA_HALF_WIDTH);
    }

    tornadoPosition = {owner->arenaCenterX, owner->position.y + BASE_Y_OFFSET, owner->position.z};
    remainingTime = owner->GetHealth() <= LOW_HEALTH_THRESHOLD ? LOW_HEALTH_DURATION : NORMAL_DURATION;
    owner->velocity.x = 0.0f;
    owner->velocity.z = 0.0f;

    if (!windEffect)
    {
        windEffect = std::make_unique<Effect>("Data/Effect/wind.efkefc");
    }

    windHandle = windEffect->Play(tornadoPosition);
    active = true;
    owner->StartBackWind(BACK_WIND_PAUSE_DURATION, true);
    owner->PlayAnimation(ATTACK_ANIMATION, false);
    Audio::Instance().Play(ATTACK_SOUND);
    Audio::Instance().Play(STORM_SOUND, true);
}

// 現在の状態の更新処理
void EnemyBoss::TornadoState::OnUpdate(float elapsedTime)
{
    if (!active)
    {
        return;
    }

    if (owner->IsDead() || !owner->player)
    {
        OnExit();
        return;
    }

    const float step = (std::min)((std::max)(elapsedTime, 0.0f), remainingTime);

    if (step > 0.0f && !owner->player->IsDead())
    {
        UpdatePullAndCollision(step);
    }

    remainingTime -= step;

    if (remainingTime <= 0.0f)
    {
        OnExit();
        owner->nextIdleDuration = RECOVERY_DURATION;
        owner->SetState(StateId::Idle);
    }
}

// 状態を抜けるときの処理
void EnemyBoss::TornadoState::OnExit()
{
    owner->StopBackWind();

    if (windEffect && windHandle >= 0)
    {
        windEffect->Stop(windHandle);
    }

    windHandle = -1;

    if (active)
    {
        Audio::Instance().Stop(STORM_SOUND);
    }

    active = false;
    remainingTime = 0.0f;
}

float EnemyBoss::TornadoState::GetRadiusAtHeight(float relativeY) const
{
    const float heightRatio = (std::clamp)(relativeY / TORNADO_HEIGHT, 0.0f, 1.0f);
    return BOTTOM_RADIUS + (TOP_RADIUS - BOTTOM_RADIUS) * heightRatio;
}

// UpdatePullAndCollision の更新処理
void EnemyBoss::TornadoState::UpdatePullAndCollision(float elapsedTime)
{
    Player& player = *owner->player;
    auto playerPosition = player.GetPosition();
    const float relativeY = playerPosition.y - tornadoPosition.y;

    if (relativeY < -VERTICAL_MARGIN || relativeY > TORNADO_HEIGHT + VERTICAL_MARGIN)
    {
        return;
    }

    const float radiusAtHeight = GetRadiusAtHeight(relativeY);
    float dx = tornadoPosition.x - playerPosition.x;
    const float dz = tornadoPosition.z - playerPosition.z;
    float distance = std::sqrt(dx * dx + dz * dz);

    if (distance < radiusAtHeight * PULL_RANGE_MULTIPLIER && distance > PULL_MIN_DISTANCE)
    {

        const float moveX = dx / distance * PULL_SPEED * elapsedTime;
        playerPosition.x += (std::clamp)(moveX, -std::abs(dx), std::abs(dx));
        player.SetPosition(playerPosition);
        dx = tornadoPosition.x - playerPosition.x;
        distance = std::sqrt(dx * dx + dz * dz);
    }

    if (distance < radiusAtHeight + player.GetRadius())
    {
        if (player.GetVelocity().y < LIFT_SPEED_THRESHOLD)
        {
            player.AddImpulse({0.0f, LIFT_IMPULSE, 0.0f});
        }

        if (player.GetInvincibleTime() <= 0.0f)
        {
            player.ApplyDamage(DAMAGE, INVINCIBLE_DURATION);
        }
    }
}

// 当たり判定などのデバッグ描画処理
void EnemyBoss::TornadoState::DrawDebugPrimitive(ShapeRenderer* renderer) const
{
    if (!active || !renderer)
    {
        return;
    }

    for (int index = 0; index <= DEBUG_SEGMENTS; ++index)
    {
        const float y = TORNADO_HEIGHT * static_cast<float>(index) / DEBUG_SEGMENTS;
        renderer->DrawSphere({tornadoPosition.x, tornadoPosition.y + y, tornadoPosition.z},
                             GetRadiusAtHeight(y), {0.1f, 0.8f, 1.0f, 1.0f});
    }
}

// 状態に入るときの処理
void EnemyBoss::ChargeAttackState::OnEnter()
{
    normalColor = owner->color;
    phase = Phase::Ready;
    phaseTime = 0.0f;
    owner->velocity.x = 0.0f;
    owner->velocity.z = 0.0f;
    const float direction = owner->position.x >= owner->arenaCenterX ? -1.0f : 1.0f;
    owner->rotation.y = DirectX::XMConvertToRadians(direction > 0.0f ? 90.0f : -90.0f);
    owner->PlayAnimation(9, false);

    if (!chargeEffect)
    {
        chargeEffect = std::make_unique<Effect>("Data/Effect/ChargeEffect.efkefc");
    }

    chargeHandle =
        chargeEffect->Play({owner->position.x, owner->position.y + owner->height * 0.5f, owner->position.z},
                           CHARGE_EFFECT_SCALE);
    Audio::Instance().Play("SE_BOSS_DASH_CHARGE");
}

// 状態を抜けるときの処理
void EnemyBoss::ChargeAttackState::OnExit()
{
    owner->color = normalColor;

    if (chargeEffect)
    {
        chargeEffect->Stop(chargeHandle);
    }

    chargeHandle = -1;
    Audio::Instance().Stop("SE_BOSS_DASH_CHARGE");
    Audio::Instance().Stop("SE_BOSS_MOVE");
}

// 現在の状態の更新処理
void EnemyBoss::ChargeAttackState::OnUpdate(float elapsedTime)
{
    phaseTime += (std::max)(elapsedTime, 0.0f);

    switch (phase)
    {
    case Phase::Ready:
    {
        const float progress = (std::clamp)(phaseTime / CHARGE_DURATION, 0.0f, 1.0f);

        const float pulse =
            0.5f + 0.5f * std::sin(DirectX::XM_2PI *
                                   (3.0f * phaseTime + 2.5f * phaseTime * phaseTime / CHARGE_DURATION));
        const float glow = (0.25f + 0.75f * progress) * pulse;
        owner->color = {normalColor.x + (1.8f - normalColor.x) * glow,
                        normalColor.y + (1.6f - normalColor.y) * glow,
                        normalColor.z + (0.8f - normalColor.z) * glow, normalColor.w};

        if (chargeEffect)
        {
            chargeEffect->SetPosition(
                chargeHandle,
                {owner->position.x, owner->position.y + owner->height * 0.5f, owner->position.z});
            const float scale = CHARGE_EFFECT_SCALE * (0.7f + 0.3f * progress);
            chargeEffect->SetScale(chargeHandle, {scale, scale, scale});
        }

        if (phaseTime >= CHARGE_DURATION)
        {
            OnExit();
            Audio::Instance().Play("SE_BOSS_MOVE");
            chargeStartX = owner->position.x;
            const float margin =
                (std::min)(owner->radius + BACKSIDE_CLEARANCE, (owner->arenaMaxX - owner->arenaMinX) * 0.5f);
            const bool moveRight = chargeStartX < owner->arenaCenterX;
            chargeTargetX = moveRight ? owner->arenaMaxX - margin : owner->arenaMinX + margin;
            owner->rotation.y = DirectX::XMConvertToRadians(moveRight ? 90.0f : -90.0f);
            owner->velocity.x = 0.0f;
            owner->velocity.z = 0.0f;
            owner->PlayAnimation(7, true);
            phase = Phase::Charge;
            phaseTime = 0.0f;
        }

        break;
    }

    case Phase::Charge:
    {
        const float progress = (std::clamp)(phaseTime / CHARGE_MOVE_DURATION, 0.0f, 1.0f);
        const float previousX = owner->position.x;
        const float nextX = chargeStartX + (chargeTargetX - chargeStartX) * progress;

        if (owner->player && !owner->player->IsDead())
        {
            const auto& playerPosition = owner->player->GetPosition();
            const float closestX =
                (std::clamp)(playerPosition.x, (std::min)(previousX, nextX), (std::max)(previousX, nextX));
            const float dx = playerPosition.x - closestX;
            const float dz = playerPosition.z - owner->position.z;
            const float combinedRadius = owner->radius + owner->player->GetRadius();
            const bool overlapsHeight = playerPosition.y < owner->position.y + owner->height &&
                                        playerPosition.y + owner->player->GetHeight() > owner->position.y;

            if (overlapsHeight && dx * dx + dz * dz < combinedRadius * combinedRadius)
            {
                owner->player->ApplyDamage(1, 1.2f);
            }
        }

        owner->position.x = nextX;
        owner->velocity.x = 0.0f;
        owner->velocity.z = 0.0f;

        if (progress >= 1.0f)
        {
            owner->position.x = chargeTargetX;
            Audio::Instance().Stop("SE_BOSS_MOVE");

            owner->rotation.y =
                DirectX::XMConvertToRadians(chargeTargetX >= owner->arenaCenterX ? -90.0f : 90.0f);
            owner->PlayAnimation(0, true);
            phase = Phase::Complete;
            phaseTime = 0.0f;
        }

        break;
    }

    case Phase::Complete:
        if (phaseTime >= RIGOR_DURATION)
        {
            owner->nextIdleDuration = 0.0f;
            owner->SetState(StateId::Idle);
        }

        break;
    }
}
