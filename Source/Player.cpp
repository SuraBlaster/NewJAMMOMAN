#include "ModelManager.h"
#include "Player.h"
#include "EnemyBoss.h"
#include "Audio/Audio.h"
#include "Camera.h"
#include "InputManager.h"
#include "Graphics.h"
#include "CollisionManager.h"
#include "Collision.h"
#include "EnemyManager.h"
#include "SetStage.h"
#include "ShapeRenderer.h"
#include <ImGui.h>

namespace
{

    bool BuildCapsuleTransform(const DirectX::XMFLOAT3& start, const DirectX::XMFLOAT3& end,
                               DirectX::XMFLOAT4X4& transform, float& height)
    {
        const DirectX::XMVECTOR startVector = DirectX::XMLoadFloat3(&start);
        const DirectX::XMVECTOR endVector = DirectX::XMLoadFloat3(&end);
        const DirectX::XMVECTOR difference = DirectX::XMVectorSubtract(endVector, startVector);
        height = DirectX::XMVectorGetX(DirectX::XMVector3Length(difference));

        if (height <= 0.0001f)
        {
            return false;
        }

        const DirectX::XMVECTOR yAxis = DirectX::XMVectorScale(difference, 1.0f / height);
        const float upDot = std::abs(DirectX::XMVectorGetY(yAxis));
        const DirectX::XMVECTOR reference =
            upDot > 0.99f ? DirectX::XMVectorSet(1, 0, 0, 0) : DirectX::XMVectorSet(0, 1, 0, 0);
        const DirectX::XMVECTOR xAxis =
            DirectX::XMVector3Normalize(DirectX::XMVector3Cross(reference, yAxis));
        const DirectX::XMVECTOR zAxis = DirectX::XMVector3Cross(yAxis, xAxis);
        const DirectX::XMVECTOR center =
            DirectX::XMVectorScale(DirectX::XMVectorAdd(startVector, endVector), 0.5f);

        DirectX::XMMATRIX world;
        world.r[0] = DirectX::XMVectorSetW(xAxis, 0.0f);
        world.r[1] = DirectX::XMVectorSetW(yAxis, 0.0f);
        world.r[2] = DirectX::XMVectorSetW(zAxis, 0.0f);
        world.r[3] = DirectX::XMVectorSetW(center, 1.0f);
        DirectX::XMStoreFloat4x4(&transform, world);
        return true;
    }
}

// コンストラクタ
Player::Player(ID3D11Device* device)
{
    const char* filename = "./Data/Model/Jammo/Jammo_Player.gltf";
    model = ModelManager::Instance().CreateInstance(device, filename);
    animator = std::make_unique<Animator>(model.get(), 2);

    // 上半身マスクの生成
    upperBodyMask.BuildLayerMaskFromRoot(model.get(), "mixamorig:Spine1");

    position.z = 0.0f;
    height = 2.0f;
    rotation.y = DirectX::XMConvertToRadians(90);

    scale.x = scale.y = scale.z = 0.0f;

    health = maxHealth = 20;

    useGravity = true;

    // ステートの生成
    states[static_cast<size_t>(StateId::Idle)] = std::make_unique<IdleState>(this);
    states[static_cast<size_t>(StateId::Move)] = std::make_unique<MoveState>(this);
    states[static_cast<size_t>(StateId::Attack)] = std::make_unique<AttackState>(this);
    states[static_cast<size_t>(StateId::Jump)] = std::make_unique<JumpState>(this);
    states[static_cast<size_t>(StateId::JumpFall)] = std::make_unique<JumpFallState>(this);
    states[static_cast<size_t>(StateId::Dash)] = std::make_unique<DashState>(this);
    states[static_cast<size_t>(StateId::WallSlide)] = std::make_unique<WallSlideState>(this);
    states[static_cast<size_t>(StateId::WallKick)] = std::make_unique<WallKickState>(this);
    states[static_cast<size_t>(StateId::Clear)] = std::make_unique<ClearState>(this);
    states[static_cast<size_t>(StateId::Damage)] = std::make_unique<DamageState>(this);
    states[static_cast<size_t>(StateId::Death)] = std::make_unique<DeathState>(this);
    states[static_cast<size_t>(StateId::Spawn)] = std::make_unique<SpawnState>(this);

    SetState(StateId::Spawn);
}

// デストラクタ
Player::~Player()
{
    states[static_cast<size_t>(StateId::Clear)]->OnExit();
    states[static_cast<size_t>(StateId::Death)]->OnExit();
}

// 更新処理
void Player::Update(float elapsedTime)
{
    if (IsDead())
    {
        if (currentState != StateId::Death && nextState != StateId::Death)
        {
            SetState(StateId::Death);
        }
    }
    else if (EnemyBoss::GetDefeat() && !IsClearing())
    {
        SetState(StateId::Clear);
    }

    UpdateJump(elapsedTime);

    UpdateStateMachine(elapsedTime);

    if (IsClearing() || currentState == StateId::Death)
    {
        animator->Update(elapsedTime);
        Character::UpdateTransform();
        return;
    }

    UpdateVelocity(elapsedTime);
    UpdateUpperBody(elapsedTime);
    animator->Update(elapsedTime);
    UpdateInvincibleTimer(elapsedTime);
    Character::UpdateTransform();
    UpdateSwordTrail(elapsedTime);
}

void Player::PrepareForBossIntro()
{
    if (currentState != StateId::None)
    {
        states[static_cast<size_t>(currentState)]->OnExit();
    }

    velocity = {0.0f, 0.0f, 0.0f};
    gravity = 10.0f;
    dashTimer = 0.0f;
    isDashJump = false;
    isAirAttack = false;
    wallClimb = false;
    wallKickLockTimer = 0.0f;
    inputMoveX = inputMoveZ = 0.0f;
    hasNextComboInput = false;
    SetBladeActive(false);
    SetTrailActive(false);
    prevBladeActive = prevTrailActive = false;
    hasSwordCollisionDebugData = false;
    trail.ClearMotion();
    trail.FadeSword(1.0f);
    animator->SetLayerSpeed(0, 1.0f);
    currentState = StateId::Idle;
    nextState = StateId::None;
    states[static_cast<size_t>(currentState)]->OnEnter();
    animator->Update(0.0f);
    Character::UpdateTransform();
}

#ifdef _DEBUG
void Player::DebugWarp(const DirectX::XMFLOAT3& destination, float facingDirection)
{
    PrepareForBossIntro();
    useGravity = true;
    health = maxHealth;
    invincibleTime = 0.0f;
    hasBodyOverlap = false;

    if (damagedEffect && damagedHandle >= 0)
    {
        damagedEffect->Stop(damagedHandle);
    }

    damagedHandle = -1;
    position = destination;
    rotation.y = DirectX::XMConvertToRadians(facingDirection > 0.0f ? 90.0f : -90.0f);
    isGround = true;
    Character::UpdateTransform();
}
#endif

// デバッグ画面の描画処理
void Player::DrawGUI()
{
#ifdef _DEBUG
    ImGui::Begin("Player");
    {
        ImGui::Text(debugStateNames[static_cast<int>(currentState)].c_str());
        ImGui::InputFloat3("position", &position.x);
        ImGui::InputFloat3("velocity", &velocity.x);
        ImGui::InputInt("health", &health);
#ifdef _DEBUG
        ImGui::Text("Gravity enabled: %s", useGravity ? "true" : "false");
        ImGui::Text("Gravity: %.3f | Wall slide: %s", gravity, wallClimb ? "true" : "false");
#endif
        ImGui::Checkbox("Show sword collision", &showSwordCollision);
        ImGui::Checkbox("Show body AABB", &showBodyAabb);
        ImGui::SliderFloat("Body half width", &bodyHalfWidth, 0.05f, 2.0f);
        ImGui::SliderFloat("Body half depth", &bodyHalfDepth, 0.05f, 2.0f);
        ImGui::SliderFloat("Body height", &bodyHeight, 0.1f, 4.0f);
        ImGui::Text("Grounded: %s", isGround ? "true" : "false");

        ImGui::Text("Body overlap: %s", hasBodyOverlap ? "true" : "false");
    }

    ImGui::End();

    if (ImGui::BeginMainMenuBar())
    {
        if (ImGui::BeginMenu("How To Use"))
        {
            ImGui::Text("X:Attack");
            ImGui::Text("Space:Jump or WallKick");
            ImGui::Text("Shift:Dash");

            ImGui::EndMenu();
        }

        ImGui::EndMainMenuBar();
    }

#endif
}

// 当たり判定などのデバッグ描画処理
void Player::DrawDebugPrimitive(ShapeRenderer* shapeRenderer) const
{
    if (!shapeRenderer)
    {
        return;
    }

    // 身体のAABBを描画
    if (showBodyAabb)
    {
        const AABB box = GetBodyAABB();

        shapeRenderer->DrawBox(box.GetCenter(), DirectX::XMFLOAT3{0.0f, 0.0f, 0.0f}, box.GetHalfSize(),
                               DirectX::XMFLOAT4{0.0f, 1.0f, 0.0f, 1.0f});
    }

    if (!shapeRenderer || !showSwordCollision || !isBladeActive || !hasSwordCollisionDebugData)
    {
        return;
    }

    for (int step = 0; step <= SWORD_SWEEP_STEPS; ++step)
    {
        const float t = step / static_cast<float>(SWORD_SWEEP_STEPS);
        const DirectX::XMFLOAT3 root = {
            debugPreviousBladeRoot.x + (debugBladeRoot.x - debugPreviousBladeRoot.x) * t,
            debugPreviousBladeRoot.y + (debugBladeRoot.y - debugPreviousBladeRoot.y) * t,
            debugPreviousBladeRoot.z + (debugBladeRoot.z - debugPreviousBladeRoot.z) * t};
        const DirectX::XMFLOAT3 tip = {
            debugPreviousBladeTip.x + (debugBladeTip.x - debugPreviousBladeTip.x) * t,
            debugPreviousBladeTip.y + (debugBladeTip.y - debugPreviousBladeTip.y) * t,
            debugPreviousBladeTip.z + (debugBladeTip.z - debugPreviousBladeTip.z) * t};

        DirectX::XMFLOAT4X4 capsuleTransform;
        float capsuleHeight;

        if (!BuildCapsuleTransform(root, tip, capsuleTransform, capsuleHeight))
        {
            continue;
        }

        const bool currentCapsule = step == SWORD_SWEEP_STEPS;
        const DirectX::XMFLOAT4 color = currentCapsule ? DirectX::XMFLOAT4{0.1f, 1.0f, 0.2f, 1.0f}
                                                       : DirectX::XMFLOAT4{1.0f, 0.65f, 0.1f, 1.0f};
        shapeRenderer->DrawCapsule(capsuleTransform, SWORD_COLLISION_RADIUS, capsuleHeight, color);
    }
}

void Player::SetBladeActive(bool active)
{
    if (active && !isBladeActive)
    {
        swordHitEnemies.clear();
    }

    isBladeActive = active;
}

void Player::SetTrailActive(bool active)
{
    isTrailActive = active;
}

AABB Player::GetBodyAABB() const
{
    AABB box;

    box.min = {position.x - bodyHalfWidth, position.y, position.z - bodyHalfDepth};

    box.max = {position.x + bodyHalfWidth, position.y + bodyHeight, position.z + bodyHalfDepth};

    return box;
}

// RenderTrail の描画処理
void Player::RenderTrail(PrimitiveRenderer* primitiveRenderer)
{
    if (isTrailActive)
    {
        trail.RenderSword(primitiveRenderer);
    }
}

// 移動速度の更新処理
void Player::UpdateVelocity(float elapsedTime)
{
    if (wallKickLockTimer > 0.0f)
    {
        wallKickLockTimer -= elapsedTime;
    }
    else
    {
        // 加速処理
        float inputMoveLength = sqrtf(inputMoveX * inputMoveX + inputMoveZ * inputMoveZ);

        if (inputMoveLength > 0)
        {
            float vecX = inputMoveX / inputMoveLength;
            float vecZ = inputMoveZ / inputMoveLength;

            // ダッシュジャンプ中の場合は加速度を上げる
            float currentAcceleration = this->acceleration;

            if (isDashJump)
            {
                currentAcceleration *= 1.5f;
            }

            // 逆入力判定
            bool isReversing = (velocity.x * vecX < 0.0f);

            if (isReversing)
            {
                // 慣性の反転
                velocity.x = vecX * std::abs(velocity.x);
            }
            else
            {
                // 同方向への移動は通常の加速処理
                float frameAcceleration = currentAcceleration * elapsedTime;

                if (!isGround)
                {
                    frameAcceleration *= airControl;
                }

                velocity.x += vecX * frameAcceleration;
                velocity.z += vecZ * frameAcceleration;
            }

            // ダッシュジャンプ中の場合は最大速度上限も上げる
            float currentMoveSpeed = moveSpeed;

            if (isDashJump)
            {
                currentMoveSpeed *= 1.5f;
            }

            // 最大速度制限
            float velocityLength = sqrtf(velocity.x * velocity.x + velocity.z * velocity.z);

            if (velocityLength > currentMoveSpeed)
            {
                velocity.x = (velocity.x / velocityLength) * currentMoveSpeed;
                velocity.z = (velocity.z / velocityLength) * currentMoveSpeed;
            }
        }
        else
        {
            // 減速処理
            float deceleration = this->deceleration * elapsedTime;

            if (!isGround)
            {
                deceleration *= airControl;
            }

            float velocityLength = sqrtf(velocity.x * velocity.x + velocity.z * velocity.z);

            if (velocityLength > deceleration)
            {
                velocity.x -= (velocity.x / velocityLength) * deceleration;
                velocity.z -= (velocity.z / velocityLength) * deceleration;
            }
            else
            {
                velocity.x = 0.0f;
                velocity.z = 0.0f;
            }
        }
    }

    // 重力処理
    GravityChange(elapsedTime);

    MoveAndCollide(elapsedTime);
}

// 状態遷移と現在の状態の更新処理
void Player::UpdateStateMachine(float elapsedTime)
{
    // ステートの切り替え
    if (nextState != StateId::None)
    {
        if (currentState != StateId::None)
        {
            states[static_cast<size_t>(currentState)]->OnExit();
        }

        currentState = nextState;
        nextState = StateId::None;

        if (currentState != StateId::None)
        {
            states[static_cast<size_t>(currentState)]->OnEnter();
        }
    }

    // ステートの更新
    if (currentState != StateId::None)
    {
        states[static_cast<size_t>(currentState)]->OnUpdate(elapsedTime);
    }
}

// 常に呼ばれる「上半身レイヤーの管理」
void Player::UpdateUpperBody(float elapsedTime)
{
    // Layer 1（上半身）のアニメーションが終了したらフラグを折る
    if (isUpperBodyActive && !animator->IsPlaying(1))
    {
        isUpperBodyActive = false;
    }

    // 上半身レイヤーのウェイトを滑らかに増減（フェードイン・フェードアウト）
    float fadeSpeed = 10.0f;

    if (isUpperBodyActive)
    {
        upperBodyWeight = (std::min)(1.0f, upperBodyWeight + fadeSpeed * elapsedTime);
    }
    else
    {
        upperBodyWeight = (std::max)(0.0f, upperBodyWeight - fadeSpeed * elapsedTime);
    }

    // Animator に常に状態を反映
    animator->SetLayerState(1, upperBodyWeight, AnimationBlendMode::Override, &upperBodyMask);
}

// UpdateDash の更新処理
void Player::UpdateDash()
{
    if (!IsGround())
    {
        useGravity = false;
        velocity.y = 0.0f;
    }
    else
    {
        useGravity = true;
    }

    DirectX::XMFLOAT3 dir = {sinf(rotation.y), 0.0f, cosf(rotation.y)};
    DirectX::XMStoreFloat3(&dir, DirectX::XMVector3Normalize(DirectX::XMLoadFloat3(&dir)));

    velocity.x = dir.x * 10;
    velocity.z = dir.z * 10;
}

void Player::UpdateJump(float elapsedTime)
{
    if (IsGround())
    {
        coyoteTimer = coyoteTime;
    }
    else
    {
        coyoteTimer -= elapsedTime;
    }
}

// 残像の更新処理
void Player::UpdateMotionTrail(float elapsedTime)
{
    trail.RecordMotion(elapsedTime, *model, position, rotation, scale);
}

// 剣の軌跡と当たり判定の更新処理
void Player::UpdateSwordTrail(float elapsedTime)
{
    // プレイヤーの右手ノードを取得
    int handNodeIndex = model->GetNodeIndex("mixamorig:RightHandThumb2");

    if (handNodeIndex == -1)
    {
        return;
    }

    const Model::Node& handNode = model->GetNodes().at(handNodeIndex);

    // Zセイバーをイメージしたレーザーの長さ
    DirectX::XMVECTOR rootOffset = DirectX::XMVectorSet(0.0f, 0.0f, 0.15f, 0.0f);
    DirectX::XMVECTOR tipOffset = DirectX::XMVectorSet(0.0f, 0.0f, 5.35f, 0.0f);

    const DirectX::XMMATRIX handWorld = DirectX::XMLoadFloat4x4(&handNode.worldTransform);
    const DirectX::XMMATRIX swordLocal =
        DirectX::XMMatrixRotationRollPitchYaw(DirectX::XMConvertToRadians(-23.0f),
                                              DirectX::XMConvertToRadians(-109.0f),
                                              DirectX::XMConvertToRadians(-42.0f)) *
        DirectX::XMMatrixTranslation(0.5f, 0.1f, 0.020f);
    const DirectX::XMMATRIX wMatrix = swordLocal * handWorld;
    DirectX::XMVECTOR rootVector = DirectX::XMVector3Transform(rootOffset, wMatrix);
    DirectX::XMVECTOR tipVector = DirectX::XMVector3Transform(tipOffset, wMatrix);
    DirectX::XMFLOAT3 currentRoot, currentTip;
    DirectX::XMStoreFloat3(&currentRoot, rootVector);
    DirectX::XMStoreFloat3(&currentTip, tipVector);

    if (isBladeActive)
    {
        trail.RecordSword(currentRoot, currentTip, !prevBladeActive || (isTrailActive && !prevTrailActive));

        CheckSwordCollision(currentRoot, currentTip, trail.GetPreviousRoot(), trail.GetPreviousTip());

        debugBladeRoot = currentRoot;
        debugBladeTip = currentTip;
        debugPreviousBladeRoot = trail.GetPreviousRoot();
        debugPreviousBladeTip = trail.GetPreviousTip();
        hasSwordCollisionDebugData = true;
    }
    else
    {
        trail.FadeSword(elapsedTime);
    }

    prevBladeActive = isBladeActive;
    prevTrailActive = isTrailActive;
}

void Player::CheckSwordCollision(const DirectX::XMFLOAT3& bladeRoot, const DirectX::XMFLOAT3& bladeTip,
                                 const DirectX::XMFLOAT3& previousRoot, const DirectX::XMFLOAT3& previousTip)
{
    constexpr int SWORD_DAMAGE = 1;

    EnemyManager& enemyManager = EnemyManager::Instance();

    for (size_t enemyIndex = 0; enemyIndex < enemyManager.GetEnemyCount(); ++enemyIndex)
    {
        const std::shared_ptr<Enemy>& enemy = enemyManager.GetEnemy(enemyIndex);

        if (!enemy || enemy->IsDestroyRequested() || swordHitEnemies.count(enemy.get()) > 0)
        {
            continue;
        }

        bool hit = false;
        DirectX::XMFLOAT3 contactPosition;

        for (int step = 0; step <= SWORD_SWEEP_STEPS && !hit; ++step)
        {
            const float t = step / static_cast<float>(SWORD_SWEEP_STEPS);
            const DirectX::XMFLOAT3 root = {previousRoot.x + (bladeRoot.x - previousRoot.x) * t,
                                            previousRoot.y + (bladeRoot.y - previousRoot.y) * t,
                                            previousRoot.z + (bladeRoot.z - previousRoot.z) * t};
            const DirectX::XMFLOAT3 tip = {previousTip.x + (bladeTip.x - previousTip.x) * t,
                                           previousTip.y + (bladeTip.y - previousTip.y) * t,
                                           previousTip.z + (bladeTip.z - previousTip.z) * t};
            hit = Collision::IntersectCapsuleVsCylinder(root, tip, SWORD_COLLISION_RADIUS,
                                                        enemy->GetPosition(), enemy->GetRadius(),
                                                        enemy->GetHeight(), contactPosition);
        }

        if (hit && enemy->ApplyDamage(SWORD_DAMAGE))
        {
            swordHitEnemies.insert(enemy.get());
            Audio::Instance().Play("SE_PLAYER_SLASH");
            const auto effectPlacement = enemy->GetCombatEffectPlacement();

            Camera::Instance().StartShake(0.1f, 0.2f);

            if (enemy->IsDead())
            {
                if (!explosionEffect)
                {
                    explosionEffect = std::make_unique<Effect>("Data/Effect/Fire.efkefc");
                }

                explosionHandle =
                    explosionEffect->Play(effectPlacement.position, effectPlacement.size * 0.25f);
                InputManager::Instance().GetGamePad().Vibrate(0.3f, 0.3f, 0.3f);
            }
            else
            {
                if (!slashEffect)
                {
                    slashEffect = std::make_unique<Effect>("Data/Effect/SlashEffect.efkefc");
                }

                slashHandle = slashEffect->Play(effectPlacement.position, effectPlacement.size);
                InputManager::Instance().GetGamePad().Vibrate(0.1f, 0.1f, 0.3f);
            }
        }
    }
}

// 移動入力処理
bool Player::InputMove()
{
    InputManager& gamePad = InputManager::Instance();
    float axisX = gamePad.GetAxisLX();

    inputMoveX = axisX;
    inputMoveZ = 0.0f;

    // 入力がある場合
    if (std::abs(axisX) > 0.1f)
    {
        rotation.y = DirectX::XMConvertToRadians(axisX < 0.0f ? -90.0f : 90.0f);
        return true;
    }

    return false;
}

// ジャンプ入力処理
bool Player::InputJump()
{
    const InputManager& gamePad = InputManager::Instance();

    if (!(gamePad.GetButtonDown() & InputManager::BTN_A))
    {
        return false;
    }

    if (!IsGround() && currentState != StateId::WallSlide && coyoteTimer <= 0.0f)
    {
        return false;
    }

    isDashJump = (gamePad.GetButton() & InputManager::BTN_B) != 0;
    coyoteTimer = 0.0f;
    velocity.y = jumpSpeed * (isDashJump ? 1.2f : 1.0f);
    return true;
}

bool Player::InputShot()
{
    InputManager& gamePad = InputManager::Instance();

    if (gamePad.GetButtonDown() & InputManager::BTN_X)
    {
        SetState(StateId::Attack);
        return true;
    }

    return false;
}

bool Player::InputDash()
{
    return (InputManager::Instance().GetButtonDown() & InputManager::BTN_B) != 0;
}

// スティックの入力方向とwallNormalの内積が負のとき壁へ向いていると判定する
bool Player::InputTowardWall()
{
    InputManager& gamePad = InputManager::Instance();
    float axisX = gamePad.GetAxisLX();

    if (std::abs(axisX) < 0.1f)
    {
        return false;
    }

    return (axisX * wallNormal.x) < 0.0f;
}

// 左スティックが壁と反対の方向へ傾けられているか判定
bool Player::InputAwayFromWall()
{
    InputManager& gamePad = InputManager::Instance();
    float axisX = gamePad.GetAxisLX();

    if (std::abs(axisX) < 0.1f)
    {
        return false;
    }

    return (axisX * wallNormal.x) > 0.0f;
}

void Player::HoldJump()
{
    InputManager& gamePad = InputManager::Instance();

    const bool isHoldingJump = (gamePad.GetButton() & InputManager::BTN_A) != 0;

    // ボタンを離しており、かつ現在の上昇速度が規定値を上回っている場合
    if (!isHoldingJump && velocity.y > cutJumpVelocity)
    {
        // 上昇速度を強制的にカット速度に下げる
        velocity.y = cutJumpVelocity;
    }
}

void Player::GravityChange(float elapsedTime)
{
    if (wallClimb)
    {
        constexpr float WALL_SLIDE_GRAVITY = 2.0f;
        constexpr float MAX_WALL_SLIDE_SPEED = 2.0f;

        gravity = WALL_SLIDE_GRAVITY;
        velocity.y -= gravity * elapsedTime;

        if (velocity.y < -MAX_WALL_SLIDE_SPEED)
        {
            velocity.y = -MAX_WALL_SLIDE_SPEED;
        }

        return;
    }

    if (velocity.y < 0.0f)
    {
        gravity = 33.75f;
    }
    else
    {
        gravity = 22.5f;
    }

    if (!useGravity)
    {
        return;
    }

    velocity.y -= gravity * elapsedTime;

    const float maxFallSpeed = -gravity;

    if (velocity.y < maxFallSpeed)
    {
        velocity.y = maxFallSpeed;
    }
}

bool Player::WallJudgement(float elapsedTime)
{
    const DirectX::XMFLOAT3 previousNormal = wallNormal;
    wallNormal = {};

    // 接地中と、キック直後は壁につかまらない
    if (isGround || wallKickLockTimer > 0.0f)
    {
        return false;
    }

    constexpr float CHECK_DISTANCE = 0.003f;

    CollisionManager& manager = CollisionManager::Instance();

    const AABB body = GetBodyAABB();

    auto checkWall = [&](float direction, DirectX::XMFLOAT3& outNormal)
    {
        TerrainSweepHit hit;

        if (!manager.CheckWall(body, direction * CHECK_DISTANCE, hit))
        {
            return false;
        }

        for (const DirectX::XMFLOAT3& normal : hit.normals)
        {
            // 調べた方向と向かい合う側面だけを採用する
            if (normal.x * direction < -0.5f)
            {
                outNormal = normal;
                return true;
            }
        }

        return false;
    };

    DirectX::XMFLOAT3 rightNormal = {};
    DirectX::XMFLOAT3 leftNormal = {};

    const bool rightWall = checkWall(1.0f, rightNormal);
    const bool leftWall = checkWall(-1.0f, leftNormal);

    if (!rightWall && !leftWall)
    {
        return false;
    }

    if (rightWall && leftWall)
    {
        // 両側が近い場合は、入力方向を優先する
        const float axisX = InputManager::Instance().GetAxisLX();

        if (axisX > 0.1f)
        {
            wallNormal = rightNormal;
        }
        else if (axisX < -0.1f)
        {
            wallNormal = leftNormal;
        }
        else
        {
            // 入力がなければ、直前につかまっていた側を優先
            wallNormal = previousNormal.x > 0.0f ? leftNormal : rightNormal;
        }
    }
    else
    {
        wallNormal = rightWall ? rightNormal : leftNormal;
    }

    return true;
}

void Player::MoveAndCollide(float elapsedTime)
{
    if (elapsedTime <= 0.0f)
    {
        return;
    }

    CollisionManager& manager = CollisionManager::Instance();

    constexpr int MAX_ITERATIONS = 4;

    // 接触直前で止めるための小さい距離
    constexpr float SKIN = 0.001f;

    // 静止時の接地確認に使う距離
    constexpr float GROUND_CHECK_DISTANCE = 0.003f;

    hasBodyOverlap = false;
    isGround = false;

    DirectX::XMFLOAT3 remaining = {velocity.x * elapsedTime, velocity.y * elapsedTime,
                                   velocity.z * elapsedTime};

    // 面へ向かう成分だけ取り除く
    auto removeInwardComponent = [](DirectX::XMFLOAT3& value, const DirectX::XMFLOAT3& normal)
    {
        const float inward = value.x * normal.x + value.y * normal.y + value.z * normal.z;

        if (inward < 0.0f)
        {
            value.x -= inward * normal.x;
            value.y -= inward * normal.y;
            value.z -= inward * normal.z;
        }
    };

    for (int iteration = 0; iteration < MAX_ITERATIONS; ++iteration)
    {
        TerrainSweepHit hit;

        const SweepStatus status = manager.SweepTerrain(GetBodyAABB(), remaining, hit);

        if (status == SweepStatus::InitialOverlap)
        {
            // 今回は自動で押し戻さず、状態を表示して止める
            hasBodyOverlap = true;
            velocity = {0.0f, 0.0f, 0.0f};
            return;
        }

        if (status == SweepStatus::NoHit)
        {
            position.x += remaining.x;
            position.y += remaining.y;
            position.z += remaining.z;
            break;
        }

        // 接触面との間に、小さい隙間を残す
        float backoffTime = 0.0f;

        for (const DirectX::XMFLOAT3& normal : hit.normals)
        {
            const float approach =
                -(remaining.x * normal.x + remaining.y * normal.y + remaining.z * normal.z);

            if (approach > 0.0f)
            {
                backoffTime = (std::max)(backoffTime, SKIN / approach);
            }
        }

        const float travelTime = (std::max)(0.0f, hit.time - backoffTime);

        position.x += remaining.x * travelTime;
        position.y += remaining.y * travelTime;
        position.z += remaining.z * travelTime;

        // 実際に進んだ割合を除いた移動量
        const float remainingRatio = 1.0f - travelTime;

        remaining.x *= remainingRatio;
        remaining.y *= remainingRatio;
        remaining.z *= remainingRatio;

        for (const DirectX::XMFLOAT3& normal : hit.normals)
        {
            removeInwardComponent(remaining, normal);
            removeInwardComponent(velocity, normal);
        }

        if (remaining.x == 0.0f && remaining.y == 0.0f && remaining.z == 0.0f)
        {
            break;
        }
    }

    // 最終位置で接地を確認する
    // 上昇中は床へ吸着・接地させない
    if (velocity.y <= 0.0f)
    {
        TerrainSweepHit groundHit;

        const SweepStatus groundStatus = manager.SweepTerrain(
            GetBodyAABB(), DirectX::XMFLOAT3{0.0f, -GROUND_CHECK_DISTANCE, 0.0f}, groundHit);

        if (groundStatus == SweepStatus::InitialOverlap)
        {
            hasBodyOverlap = true;
            velocity = {0.0f, 0.0f, 0.0f};
            return;
        }

        if (groundStatus == SweepStatus::Hit)
        {
            for (const DirectX::XMFLOAT3& normal : groundHit.normals)
            {
                if (normal.y > 0.5f)
                {
                    isGround = true;
                    velocity.y = 0.0f;
                    break;
                }
            }
        }
    }
}

void Player::SetState(StateId stateId)
{
    nextState = stateId;
}

void Player::OnDamaged()
{
    SetState(StateId::Damage);
    velocity = {0.0f, 0.0f, 0.0f};
    inputMoveX = 0.0f;
    inputMoveZ = 0.0f;
    SetBladeActive(false);
    SetTrailActive(false);
    trail.ClearMotion();
}

void Player::OnDead()
{
    SetState(StateId::Death);
    velocity = {0.0f, 0.0f, 0.0f};
    SetBladeActive(false);
    SetTrailActive(false);
}

// RenderMotionTrail の描画処理
void Player::RenderMotionTrail(ModelRenderer* modelRenderer) const
{
    trail.RenderMotion(modelRenderer);
}
