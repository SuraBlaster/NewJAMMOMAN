#include "ModelManager.h"
#include "Player.h"
#include "EnemyBoss.h"
#include "Audio/Audio.h"
#include "SceneLoading.h"
#include "SceneClear.h"
#include "GameScene.h"
#include "Camera.h"
#include "InputManager.h"
#include "Graphics.h"
#include "CollisionManager.h"
#include "Collision.h"
#include "EnemyManager.h"
#include "SetStage.h"
#include "ShapeRenderer.h"
#include <ImGui.h>
#include "HitStopManager.h"


namespace
{
	constexpr float GROUND_ATTACK_PLAYBACK_SPEED = 1.3f;
	constexpr float AIR_ATTACK_PLAYBACK_SPEED = 2.0f;
	constexpr float AIR_COMBO_CANCEL_START = 0.30f;

	bool BuildCapsuleTransform(
		const DirectX::XMFLOAT3& start,
		const DirectX::XMFLOAT3& end,
		DirectX::XMFLOAT4X4& transform,
		float& height)
	{
		const DirectX::XMVECTOR startVector = DirectX::XMLoadFloat3(&start);
		const DirectX::XMVECTOR endVector = DirectX::XMLoadFloat3(&end);
		const DirectX::XMVECTOR difference = DirectX::XMVectorSubtract(endVector, startVector);
		height = DirectX::XMVectorGetX(DirectX::XMVector3Length(difference));
		if (height <= 0.0001f) return false;

		const DirectX::XMVECTOR yAxis = DirectX::XMVectorScale(difference, 1.0f / height);
		const float upDot = std::abs(DirectX::XMVectorGetY(yAxis));
		const DirectX::XMVECTOR reference = upDot > 0.99f
			? DirectX::XMVectorSet(1, 0, 0, 0)
			: DirectX::XMVectorSet(0, 1, 0, 0);
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

Player::Player(ID3D11Device* device)
{
	const char* filename = "./Data/Model/Jammo/Jammo_Player.gltf";
	model = ModelManager::Instance().CreateInstance(device, filename);
	animator = std::make_unique<Animator>(model.get(), 2);

	SetStage::outputData data;
	data.filename = model->GetFileName();
	data.id = 57;
	SetStage::JsonOutput("test.json", data);

	// 上半身マスクの生成
	upper_body_mask.BuildLayerMaskFromRoot(model.get(), "mixamorig:Spine1");


	position.z = 0.0f;
	rotation.y = DirectX::XMConvertToRadians(90);

	scale.x = scale.y = scale.z = 0.0f;

	health = max_health = 20;

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

Player::~Player()
{
    states[static_cast<size_t>(StateId::Clear)]->OnExit();
    states[static_cast<size_t>(StateId::Death)]->OnExit();
}

void Player::Update(float elapsed_time)
{
    if (IsDead())
    {
        if (current_state != StateId::Death && next_state != StateId::Death)
            SetState(StateId::Death);
    }
    else if (EnemyBoss::GetDefeat() && !IsClearing()) SetState(StateId::Clear);
    UpdateStateMachine(elapsed_time);
    if (IsClearing() || current_state == StateId::Death)
    {
        animator->Update(elapsed_time);
        Character::UpdateTransform();
        return;
    }
    UpdateVelocity(elapsed_time);
    UpdateUpperBody(elapsed_time);
    animator->Update(elapsed_time);
    UpdateInvincibleTimer(elapsed_time);
    Character::UpdateTransform();
    UpdateSwordTrail(elapsed_time);
}
void Player::DrawGUI()
{
	ImGui::Begin("Player");
	{
		ImGui::Text(debugstatename[static_cast<int>(current_state)].c_str());
		ImGui::InputFloat3("position", &position.x);
		ImGui::InputFloat3("velocity", &velocity.x);
		ImGui::InputInt("health", & health);
		ImGui::Checkbox("Show sword collision", &show_sword_collision);
		ImGui::Checkbox("Show body AABB", &show_body_aabb);
		ImGui::SliderFloat("Body half width", &body_half_width, 0.05f, 2.0f);
		ImGui::SliderFloat("Body half depth", &body_half_depth, 0.05f, 2.0f);
		ImGui::SliderFloat("Body height", &body_height, 0.1f, 4.0f);
		ImGui::Text(
			"Grounded: %s",
			is_ground ? "true" : "false");

		ImGui::Text(
			"Body overlap: %s",
			has_body_overlap ? "true" : "false");
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
}

void Player::DrawDebugPrimitive(ShapeRenderer* shapeRenderer) const
{
	if (!shapeRenderer)
	{
		return;
	}

	// 身体のAABBを描画
	if (show_body_aabb)
	{
		const AABB box = GetBodyAABB();

		shapeRenderer->DrawBox(
			box.GetCenter(),
			DirectX::XMFLOAT3{ 0.0f, 0.0f, 0.0f },
			box.GetHalfSize(),
			DirectX::XMFLOAT4{ 0.0f, 1.0f, 0.0f, 1.0f }
		);
	}

	if (!shapeRenderer || !show_sword_collision
		|| !is_blade_active || !has_sword_collision_debug_data)
	{
		return;
	}

	for (int step = 0; step <= SWORD_SWEEP_STEPS; ++step)
	{
		const float t = step / static_cast<float>(SWORD_SWEEP_STEPS);
		const DirectX::XMFLOAT3 root = {
			debug_previous_blade_root.x + (debug_blade_root.x - debug_previous_blade_root.x) * t,
			debug_previous_blade_root.y + (debug_blade_root.y - debug_previous_blade_root.y) * t,
			debug_previous_blade_root.z + (debug_blade_root.z - debug_previous_blade_root.z) * t
		};
		const DirectX::XMFLOAT3 tip = {
			debug_previous_blade_tip.x + (debug_blade_tip.x - debug_previous_blade_tip.x) * t,
			debug_previous_blade_tip.y + (debug_blade_tip.y - debug_previous_blade_tip.y) * t,
			debug_previous_blade_tip.z + (debug_blade_tip.z - debug_previous_blade_tip.z) * t
		};

		DirectX::XMFLOAT4X4 capsuleTransform;
		float capsuleHeight;
		if (!BuildCapsuleTransform(root, tip, capsuleTransform, capsuleHeight)) continue;

		const bool currentCapsule = step == SWORD_SWEEP_STEPS;
		const DirectX::XMFLOAT4 color = currentCapsule
			? DirectX::XMFLOAT4{ 0.1f, 1.0f, 0.2f, 1.0f }
			: DirectX::XMFLOAT4{ 1.0f, 0.65f, 0.1f, 1.0f };
		shapeRenderer->DrawCapsule(
			capsuleTransform,
			SWORD_COLLISION_RADIUS,
			capsuleHeight,
			color);
	}
}

void Player::SetBladeActive(bool active)
{
	if (active && !is_blade_active)
	{
		sword_hit_enemies.clear();
	}
	is_blade_active = active;
}

void Player::SetTrailActive(bool active)
{
	is_trail_active = active;
}

AABB Player::GetBodyAABB() const
{
	AABB box;

	box.min = {
		position.x - body_half_width,
		position.y,
		position.z - body_half_depth
	};

	box.max = {
		position.x + body_half_width,
		position.y + body_height,
		position.z + body_half_depth
	};

	return box;
}

void Player::RenderTrail(PrimitiveRenderer* primitiveRenderer)
{
	if (!is_trail_active) return;

	const int segmentCount = MAX_POLYGON - 1;
	const int division = 20;
	const float trailHalfDepth = 0.10f;
	const float handleClearance = 0.12f;
	const DirectX::XMFLOAT3 rootColor = { 1.00f, 1.00f, 0.92f };
	const DirectX::XMFLOAT3 tipColor = { 0.08f, 1.00f, 0.38f };

	auto addTriangle = [&](const DirectX::XMFLOAT3& a, const DirectX::XMFLOAT4& colorA, const DirectX::XMFLOAT2& uvA,
		const DirectX::XMFLOAT3& b, const DirectX::XMFLOAT4& colorB, const DirectX::XMFLOAT2& uvB,
		const DirectX::XMFLOAT3& c, const DirectX::XMFLOAT4& colorC, const DirectX::XMFLOAT2& uvC)
	{
		primitiveRenderer->AddVertex(a, colorA, uvA);
		primitiveRenderer->AddVertex(b, colorB, uvB);
		primitiveRenderer->AddVertex(c, colorC, uvC);
	};

	auto addQuad = [&](const DirectX::XMFLOAT3& a, const DirectX::XMFLOAT4& colorA, const DirectX::XMFLOAT2& uvA,
		const DirectX::XMFLOAT3& b, const DirectX::XMFLOAT4& colorB, const DirectX::XMFLOAT2& uvB,
		const DirectX::XMFLOAT3& c, const DirectX::XMFLOAT4& colorC, const DirectX::XMFLOAT2& uvC,
		const DirectX::XMFLOAT3& d, const DirectX::XMFLOAT4& colorD, const DirectX::XMFLOAT2& uvD)
	{
		addTriangle(a, colorA, uvA, b, colorB, uvB, c, colorC, uvC);
		addTriangle(a, colorA, uvA, c, colorC, uvC, d, colorD, uvD);
	};

	bool hasPreviousSlice = false;

	for (int i = 0; i < segmentCount; i++)
	{
		const int idx0 = (std::max)(0, i - 1);
		const int idx1 = i;
		const int idx2 = (std::min)(MAX_POLYGON - 1, i + 1);
		const int idx3 = (std::min)(MAX_POLYGON - 1, i + 2);

		const DirectX::XMVECTOR root0 = DirectX::XMLoadFloat3(&trailPositions[0][idx0]);
		const DirectX::XMVECTOR root1 = DirectX::XMLoadFloat3(&trailPositions[0][idx1]);
		const DirectX::XMVECTOR root2 = DirectX::XMLoadFloat3(&trailPositions[0][idx2]);
		const DirectX::XMVECTOR root3 = DirectX::XMLoadFloat3(&trailPositions[0][idx3]);
		const DirectX::XMVECTOR tip0 = DirectX::XMLoadFloat3(&trailPositions[1][idx0]);
		const DirectX::XMVECTOR tip1 = DirectX::XMLoadFloat3(&trailPositions[1][idx1]);
		const DirectX::XMVECTOR tip2 = DirectX::XMLoadFloat3(&trailPositions[1][idx2]);
		const DirectX::XMVECTOR tip3 = DirectX::XMLoadFloat3(&trailPositions[1][idx3]);

		const int endJ = (i == segmentCount - 1) ? division : division - 1;
		for (int j = 0; j <= endJ; j++)
		{
			const float t = j / static_cast<float>(division);
			const float age = (i + t) / static_cast<float>(segmentCount);

			DirectX::XMVECTOR root = DirectX::XMVectorCatmullRom(root0, root1, root2, root3, t);
			DirectX::XMVECTOR tip = DirectX::XMVectorCatmullRom(tip0, tip1, tip2, tip3, t);

			const float fadeMul = 1.0f;
			const float fadeShape = 1.0f;
			const float taper = powf(1.0f - age, 1.15f) * fadeShape;
			const DirectX::XMVECTOR mid = DirectX::XMVectorScale(DirectX::XMVectorAdd(root, tip), 0.5f);
			root = DirectX::XMVectorLerp(mid, root, taper);
			tip = DirectX::XMVectorLerp(mid, tip, taper);
			root = DirectX::XMVectorLerp(root, tip, handleClearance);

			const float baseAlpha = powf(1.0f - age, 1.65f);
			const float alpha = baseAlpha * fadeMul * fadeMul * fadeMul;
			const float greenShift = age * 0.35f;
			const DirectX::XMFLOAT4 rootDrawColor =
			{
				rootColor.x * (1.0f - greenShift) + tipColor.x * greenShift,
				rootColor.y * (1.0f - greenShift) + tipColor.y * greenShift,
				rootColor.z * (1.0f - greenShift) + tipColor.z * greenShift,
				alpha * 0.15f
			};
			const DirectX::XMFLOAT4 tipDrawColor = { tipColor.x, tipColor.y, tipColor.z, alpha };
			DirectX::XMFLOAT4 rootSideColor = rootDrawColor;
			DirectX::XMFLOAT4 tipSideColor = tipDrawColor;
			rootSideColor.w *= 0.40f;
			tipSideColor.w *= 0.40f;

			DirectX::XMVECTOR sweepDirection = DirectX::XMVectorAdd(
				DirectX::XMVectorSubtract(root2, root1),
				DirectX::XMVectorSubtract(tip2, tip1));
			if (DirectX::XMVectorGetX(DirectX::XMVector3LengthSq(sweepDirection)) < 0.0001f)
			{
				sweepDirection = DirectX::XMVectorSet(0.0f, 1.0f, 0.0f, 0.0f);
			}
			sweepDirection = DirectX::XMVector3Normalize(sweepDirection);

			DirectX::XMFLOAT3 rootFront, tipFront;
			DirectX::XMStoreFloat3(&rootFront, root);
			DirectX::XMStoreFloat3(&tipFront, tip);

			if (!hasPreviousSlice)
			{
				const DirectX::XMVECTOR leadingPoint = DirectX::XMVectorSubtract(
					DirectX::XMVectorScale(DirectX::XMVectorAdd(root, tip), 0.5f),
					DirectX::XMVectorScale(sweepDirection, 0.45f));
				DirectX::XMFLOAT3 apex;
				DirectX::XMStoreFloat3(&apex, leadingPoint);
				primitiveRenderer->AddVertex(apex, tipDrawColor, { 0.5f, -0.15f });
			}

			primitiveRenderer->AddVertex(rootFront, rootDrawColor, { 0.0f, age });
			primitiveRenderer->AddVertex(tipFront, tipDrawColor, { 1.0f, age });

			hasPreviousSlice = true;
		}
	}

}

void Player::UpdateVelocity(float elapsed_time)
{
	if (wall_kick_lock_timer > 0.0f)
	{
		wall_kick_lock_timer -= elapsed_time;
	}
	else
	{
		// 加速処理
		float input_move_length = sqrtf(input_move_x * input_move_x + input_move_z * input_move_z);
		if (input_move_length > 0)
		{
			float vec_x = input_move_x / input_move_length;
			float vec_z = input_move_z / input_move_length;

			// ダッシュジャンプ中の場合は加速度を上げる
			float current_acceleration = this->acceleration;
			if (is_dash_jump) current_acceleration *= 1.5f;

			// 逆入力判定
			bool is_reversing = (velocity.x * vec_x < 0.0f);

			if (is_reversing)
			{
				// 慣性の反転
				velocity.x = vec_x * std::abs(velocity.x);
			}
			else
			{
				// 同方向への移動は通常の加速処理
				float frame_acceleration = current_acceleration * elapsed_time;
				if (!is_ground) frame_acceleration *= air_control;

				velocity.x += vec_x * frame_acceleration;
				velocity.z += vec_z * frame_acceleration;
			}

			// ダッシュジャンプ中の場合は最大速度上限も上げる
			float current_move_speed = move_speed;
			if (is_dash_jump) current_move_speed *= 1.5f;

			// 最大速度制限
			float velocity_length = sqrtf(velocity.x * velocity.x + velocity.z * velocity.z);
			if (velocity_length > current_move_speed)
			{
				velocity.x = (velocity.x / velocity_length) * current_move_speed;
				velocity.z = (velocity.z / velocity_length) * current_move_speed;
			}
		}
		else
		{
			// 減速処理
			float deceleration = this->deceleration * elapsed_time;
			if (!is_ground) deceleration *= air_control;

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
	GravityChange(elapsed_time);

	MoveAndCollide(elapsed_time);
}

void Player::UpdateStateMachine(float elapsed_time)
{
	// ステートの切り替え
	if (next_state != StateId::None)
	{
		if (current_state != StateId::None)
		{
			states[static_cast<size_t>(current_state)]->OnExit();
		}
		current_state = next_state;
		next_state = StateId::None;
		if (current_state != StateId::None)
		{
			states[static_cast<size_t>(current_state)]->OnEnter();
		}
	}
	// ステートの更新
	if (current_state != StateId::None)
	{
		states[static_cast<size_t>(current_state)]->OnUpdate(elapsed_time);
	}
}

// 常に呼ばれる「上半身レイヤーの管理」
void Player::UpdateUpperBody(float elapsed_time)
{
	// Layer 1（上半身）のアニメーションが終了したらフラグを折る
	if (is_upper_body_active && !animator->IsPlaying(1))
	{
		is_upper_body_active = false;
	}

	// 上半身レイヤーのウェイトを滑らかに増減（フェードイン・フェードアウト）
	float fade_speed = 10.0f;
	if (is_upper_body_active)
	{
		upper_body_weight = (std::min)(1.0f, upper_body_weight + fade_speed * elapsed_time);
	}
	else {
		upper_body_weight = (std::max)(0.0f, upper_body_weight - fade_speed * elapsed_time);
	}

	// Animator に常に状態を反映
	animator->SetLayerState(1, upper_body_weight, AnimationBlendMode::Override, &upper_body_mask);
}

void Player::UpdateDash()
{
	if (!IsGround())
	{
		velocity.y = 0.0f;
	}

	DirectX::XMFLOAT3 dir = { sinf(rotation.y), 0.0f, cosf(rotation.y) };
	DirectX::XMStoreFloat3(&dir, DirectX::XMVector3Normalize(DirectX::XMLoadFloat3(&dir)));

	velocity.x = dir.x * 10;
	velocity.z = dir.z * 10;
}

void Player::UpdateMotionTrail(float elapsedTime)
{
	trailRecordTimer -= elapsedTime;
	if (trailRecordTimer <= 0.0f)
	{
		trailRecordTimer = trail_interval;
		TrailData data;
		data.trailModel = std::make_shared<Model>(*model);
		data.position = position;
		data.angle = rotation;
		data.scale = scale;
		data.alpha = 0.5f;
		data.drawColor = { 1.0f,0.0f,0.0f,1.0f };

		trails.push_back(data);
		if (trails.size() > max_trails) trails.pop_front();
	}
}

void Player::UpdateSwordTrail(float elapsed_time)
{
	// プレイヤーの右手ノードを取得
	int handNodeIndex = model->GetNodeIndex("mixamorig:RightHandThumb2");
	if (handNodeIndex == -1) return;

	const Model::Node& handNode = model->GetNodes().at(handNodeIndex);

	// Zセイバーをイメージしたレーザーの長さ
	DirectX::XMVECTOR RootOffset = DirectX::XMVectorSet(0.0f, 0.0f, 0.15f, 0.0f);
	DirectX::XMVECTOR TipOffset = DirectX::XMVectorSet(0.0f, 0.0f, 5.35f, 0.0f);

	const DirectX::XMMATRIX handWorld = DirectX::XMLoadFloat4x4(&handNode.worldTransform);
	const DirectX::XMMATRIX swordLocal =
		DirectX::XMMatrixRotationRollPitchYaw(
			DirectX::XMConvertToRadians(-23.0f),
			DirectX::XMConvertToRadians(-109.0f),
			DirectX::XMConvertToRadians(-42.0f)) *
		DirectX::XMMatrixTranslation(0.5f, 0.1f, 0.020f);
	const DirectX::XMMATRIX W = swordLocal * handWorld;
	DirectX::XMVECTOR Root = DirectX::XMVector3Transform(RootOffset, W);
	DirectX::XMVECTOR Tip = DirectX::XMVector3Transform(TipOffset, W);
	DirectX::XMFLOAT3 current_root, current_tip;
	DirectX::XMStoreFloat3(&current_root, Root);
	DirectX::XMStoreFloat3(&current_tip, Tip);

	if (is_blade_active)
	{
		trailFadeTimer = trailFadeDuration;
		// 攻撃が開始された最初のフレームですべての座標を現在の位置で初期化する
		if (!prev_blade_active || (is_trail_active && !prev_trail_active))
		{
			for (int i = 0; i < MAX_POLYGON; i++)
			{
				trailPositions[0][i] = current_root;
				trailPositions[1][i] = current_tip;
			}
		}
		else
		{
			// アクティブ中は頂点座標を1フレーム分ずらして最新を更新する
			for (int i = 0; i < 2; i++)
			{
				for (int j = MAX_POLYGON - 1; j > 0; j--)
				{
					trailPositions[i][j] = trailPositions[i][j - 1];
				}
			}
			trailPositions[0][0] = current_root;
			trailPositions[1][0] = current_tip;
		}

		CheckSwordCollision(
			current_root,
			current_tip,
			trailPositions[0][1],
			trailPositions[1][1]);

		debug_blade_root = current_root;
		debug_blade_tip = current_tip;
		debug_previous_blade_root = trailPositions[0][1];
		debug_previous_blade_tip = trailPositions[1][1];
		has_sword_collision_debug_data = true;
	}
	else if (trailFadeTimer > 0.0f)
	{
		trailFadeTimer -= elapsed_time;
	}

	prev_blade_active = is_blade_active;
	prev_trail_active = is_trail_active;
}

void Player::CheckSwordCollision(
	const DirectX::XMFLOAT3& bladeRoot,
	const DirectX::XMFLOAT3& bladeTip,
	const DirectX::XMFLOAT3& previousRoot,
	const DirectX::XMFLOAT3& previousTip)
{
	constexpr int swordDamage = 1;

	EnemyManager& enemyManager = EnemyManager::Instance();
	for (size_t enemyIndex = 0; enemyIndex < enemyManager.GetEnemyCount(); ++enemyIndex)
	{
		const std::shared_ptr<Enemy>& enemy = enemyManager.GetEnemy(enemyIndex);
		if (!enemy || enemy->IsDestroyRequested()
			|| sword_hit_enemies.count(enemy.get()) > 0)
		{
			continue;
		}

		bool hit = false;
		DirectX::XMFLOAT3 contactPosition;
		for (int step = 0; step <= SWORD_SWEEP_STEPS && !hit; ++step)
		{
			const float t = step / static_cast<float>(SWORD_SWEEP_STEPS);
			const DirectX::XMFLOAT3 root = {
				previousRoot.x + (bladeRoot.x - previousRoot.x) * t,
				previousRoot.y + (bladeRoot.y - previousRoot.y) * t,
				previousRoot.z + (bladeRoot.z - previousRoot.z) * t
			};
			const DirectX::XMFLOAT3 tip = {
				previousTip.x + (bladeTip.x - previousTip.x) * t,
				previousTip.y + (bladeTip.y - previousTip.y) * t,
				previousTip.z + (bladeTip.z - previousTip.z) * t
			};
			hit = Collision::IntersectCapsuleVsCylinder(
				root, tip, SWORD_COLLISION_RADIUS,
				enemy->GetPosition(), enemy->GetRadius(), enemy->GetHeight(),
				contactPosition);
		}

		if (hit && enemy->ApplyDamage(swordDamage))
		{
			sword_hit_enemies.insert(enemy.get());
		}
	}
}


// 移動入力処理
bool Player::InputMove()
{
	InputManager& game_pad = InputManager::Instance();
	float axis_x = game_pad.GetAxisLX();

	input_move_x = axis_x;
	input_move_z = 0.0f;

	// 入力がある場合
	if (std::abs(axis_x) > 0.1f)
	{
		rotation.y = DirectX::XMConvertToRadians(axis_x < 0.0f ? -90.0f : 90.0f);
		return true;
	}

	return false;
}

// ジャンプ入力処理
bool Player::InputJump()
{
	const InputManager& game_pad = InputManager::Instance();
	if (!(game_pad.GetButtonDown() & InputManager::BTN_A)) return false;
	if (!IsGround() && current_state != StateId::WallSlide) return false;

	is_dash_jump = (game_pad.GetButton() & InputManager::BTN_B) != 0;
	velocity.y = jump_speed * (is_dash_jump ? 1.2f : 1.0f);
	return true;
}

bool Player::InputShot()
{
	InputManager& game_pad = InputManager::Instance();
	if (game_pad.GetButtonDown() & InputManager::BTN_X)
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
	InputManager& game_pad = InputManager::Instance();
	float axis_x = game_pad.GetAxisLX();

	if (std::abs(axis_x) < 0.1f) return false;

	return (axis_x * wallNormal.x) < 0.0f;
}

// 左スティックが壁と反対の方向へ傾けられているか判定
bool Player::InputAwayFromWall()
{
	InputManager& game_pad = InputManager::Instance();
	float axis_x = game_pad.GetAxisLX();

	if (std::abs(axis_x) < 0.1f) return false;

	return (axis_x * wallNormal.x) > 0.0f;
}

void Player::HoldJump()
{
	InputManager& game_pad = InputManager::Instance();

	const bool is_holding_jump = (game_pad.GetButton() & InputManager::BTN_A) != 0;

	// ボタンを離しており、かつ現在の上昇速度が規定値を上回っている場合
	if (!is_holding_jump && velocity.y > cut_jump_velocity)
	{
		// 上昇速度を強制的にカット速度に下げる
		velocity.y = cut_jump_velocity;
	}
}

void Player::GravityChange(float elapsed_time)
{
	if (wall_climb)
	{
		constexpr float wallSlideGravity = 2.0f;
		constexpr float maxWallSlideSpeed = 2.0f;

		gravity = wallSlideGravity;
		velocity.y -= gravity * elapsed_time;

		if (velocity.y < -maxWallSlideSpeed)
		{
			velocity.y = -maxWallSlideSpeed;
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

	velocity.y -= gravity * elapsed_time;

	const float maxFallSpeed = -gravity;

	if (velocity.y < maxFallSpeed)
	{
		velocity.y = maxFallSpeed;
	}
}

bool Player::WallJudgement(float elapsed_time)
{
	const DirectX::XMFLOAT3 previousNormal = wallNormal;
	wallNormal = {};

	// 接地中と、キック直後は壁につかまらない。
	if (is_ground || wall_kick_lock_timer > 0.0f)
	{
		return false;
	}

	constexpr float checkDistance = 0.003f;

	CollisionManager& manager =
		CollisionManager::Instance();

	const AABB body = GetBodyAABB();

	auto checkWall = [&](float direction,
		DirectX::XMFLOAT3& outNormal)
		{
			TerrainSweepHit hit;

			if (!manager.CheckWall(body, direction * checkDistance, hit)) return false;

            for (const DirectX::XMFLOAT3& normal : hit.normals)
			{
				// 調べた方向と向かい合う側面だけを採用する。
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
		// 両側が近い場合は、入力方向を優先する。
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
			// 入力がなければ、直前につかまっていた側を優先。
			wallNormal = previousNormal.x > 0.0f
				? leftNormal
				: rightNormal;
		}
	}
	else
	{
		wallNormal = rightWall ? rightNormal : leftNormal;
	}

	return true;
}

void Player::MoveAndCollide(float elapsed_time)
{
	if (elapsed_time <= 0.0f)
	{
		return;
	}

	CollisionManager& manager =
		CollisionManager::Instance();

	constexpr int maxIterations = 4;

	// 接触直前で止めるための小さい距離。
	constexpr float skin = 0.001f;

	// 静止時の接地確認に使う距離。
	constexpr float groundCheckDistance = 0.003f;

	has_body_overlap = false;
	is_ground = false;

	DirectX::XMFLOAT3 remaining = {
		velocity.x * elapsed_time,
		velocity.y * elapsed_time,
		velocity.z * elapsed_time
	};

	// 面へ向かう成分だけ取り除く。
	auto removeInwardComponent = [](
		DirectX::XMFLOAT3& value,
		const DirectX::XMFLOAT3& normal)
		{
			const float inward =
				value.x * normal.x +
				value.y * normal.y +
				value.z * normal.z;

			if (inward < 0.0f)
			{
				value.x -= inward * normal.x;
				value.y -= inward * normal.y;
				value.z -= inward * normal.z;
			}
		};

	for (int iteration = 0;
		iteration < maxIterations;
		++iteration)
	{
		TerrainSweepHit hit;

		const SweepStatus status = manager.SweepTerrain(
			GetBodyAABB(),
			remaining,
			hit);

		if (status == SweepStatus::InitialOverlap)
		{
			// 今回は自動で押し戻さず、状態を表示して止める。
			has_body_overlap = true;
			velocity = { 0.0f, 0.0f, 0.0f };
			return;
		}

		if (status == SweepStatus::NoHit)
		{
			position.x += remaining.x;
			position.y += remaining.y;
			position.z += remaining.z;
			break;
		}

		// 接触面との間に、小さい隙間を残す。
		float backoffTime = 0.0f;

		for (const DirectX::XMFLOAT3& normal : hit.normals)
		{
			const float approach =
				-(remaining.x * normal.x +
					remaining.y * normal.y +
					remaining.z * normal.z);

			if (approach > 0.0f)
			{
				backoffTime = (std::max)(
					backoffTime,
					skin / approach);
			}
		}

		const float travelTime = (std::max)(
			0.0f,
			hit.time - backoffTime);

		position.x += remaining.x * travelTime;
		position.y += remaining.y * travelTime;
		position.z += remaining.z * travelTime;

		// 実際に進んだ割合を除いた移動量。
		const float remainingRatio = 1.0f - travelTime;

		remaining.x *= remainingRatio;
		remaining.y *= remainingRatio;
		remaining.z *= remainingRatio;

		for (const DirectX::XMFLOAT3& normal : hit.normals)
		{
			removeInwardComponent(remaining, normal);
			removeInwardComponent(velocity, normal);
		}

		if (remaining.x == 0.0f &&
			remaining.y == 0.0f &&
			remaining.z == 0.0f)
		{
			break;
		}
	}

	// 最終位置で接地を確認する。
	// 上昇中は床へ吸着・接地させない。
	if (velocity.y <= 0.0f)
	{
		TerrainSweepHit groundHit;

		const SweepStatus groundStatus =
			manager.SweepTerrain(
				GetBodyAABB(),
				DirectX::XMFLOAT3{
					0.0f, -groundCheckDistance, 0.0f
				},
				groundHit);

		if (groundStatus == SweepStatus::InitialOverlap)
		{
			has_body_overlap = true;
			velocity = { 0.0f, 0.0f, 0.0f };
			return;
		}

		if (groundStatus == SweepStatus::Hit)
		{
			for (const DirectX::XMFLOAT3& normal
				: groundHit.normals)
			{
				if (normal.y > 0.5f)
				{
					is_ground = true;
					velocity.y = 0.0f;
					break;
				}
			}
		}
	}
}

void Player::SetState(StateId state_id)
{
	next_state = state_id;
}


void Player::IdleState::OnEnter()
{
	owner->animator->Play(0, "Idle_Seq_0", true);
}

void Player::IdleState::OnUpdate(float elapsed_time)
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
	owner->InputShot();
}

void Player::MoveState::OnEnter()
{
	owner->animator->Play(0, "Run_Fast_Loop_Seq_0", true, 0.1f);
}

void Player::MoveState::OnUpdate(float elapsed_time)
{
	if (!owner->InputMove())
	{
		owner->SetState(StateId::Idle);
		owner->velocity.x = 0.0f;
		return;
	}
	if (owner->InputJump())
	{
		owner->SetState(StateId::Jump);
		return;
	}
	if (owner->InputDash() && owner->IsGround())
	{
		owner->SetState(StateId::Dash);
		return;
	}

	owner->InputShot();
}

void Player::JumpState::OnEnter()
{
	owner->animator->Play(0, "Jump_Start_0_Seq_0", false, 0.1f, 0.2f);
}

void Player::JumpState::OnUpdate(float elapsed_time)
{
	owner->InputMove();

	owner->HoldJump();

	if (owner->is_dash_jump)
	{
		owner->UpdateMotionTrail(elapsed_time);
	}

	if (owner->animator->GetProgress(0))
	{
		owner->SetState(StateId::JumpFall);
	}
}

void Player::JumpFallState::OnEnter()
{
	owner->animator->Play(0, "Jump_Loop_0_Seq_0", true, 0.05f);
}

void Player::JumpFallState::OnUpdate(float elapsed_time)
{
	bool move = owner->InputMove();

	owner->HoldJump();

	if (owner->WallJudgement(elapsed_time) && owner->InputTowardWall())
	{
		owner->is_dash_jump = false;
		owner->SetState(StateId::WallSlide);
		return;
	}

	if (owner->is_dash_jump)
	{
		owner->UpdateMotionTrail(elapsed_time);
	}
	

	if (owner->IsGround())
	{
		owner->trails.clear();
		
		if (move)
		{
			owner->is_dash_jump = false;
			owner->SetState(StateId::Move);
		}
		else
		{
			owner->is_dash_jump = false;
			owner->SetState(StateId::Idle);
		}
	}
	else
	{
		if (owner->InputShot())
		{
			owner->trails.clear();
		};
	}
}

void Player::DashState::OnEnter()
{
	owner->animator->Play(0, "ComboAttack0204Seq", false, 0.1f, 0.0f, 0.5f);
	owner->dash_timer = owner->dash_duration;

	owner->velocity.x = 0.0f;
	owner->input_move_x = 0.0f;
}

void Player::DashState::OnUpdate(float elapsed_time)
{
	owner->UpdateDash();
	owner->dash_timer -= elapsed_time;
	owner->UpdateMotionTrail(elapsed_time);

	InputManager& game_pad = InputManager::Instance();

	if ((game_pad.GetButtonUp() & InputManager::BTN_B) || owner->dash_timer <= 0.0f)
	{
		owner->trails.clear();
		owner->velocity.x = 0.0f;
		owner->velocity.z = 0.0f;
		owner->SetState(StateId::Idle);
		return;
	}

	if (owner->InputJump())
	{
		owner->is_dash_jump = true;

		if (!owner->InputMove())
		{
			owner->velocity.x = 0.0f;
			owner->velocity.z = 0.0f;
		}
		owner->SetState(StateId::Jump);
	}
}

void Player::WallSlideState::OnEnter()
{
	owner->animator->Play(0, "WallSlide_R", false, 0.1f, 0.0f, 0.3f);
	owner->wall_climb = true;
	owner->trails.clear();
	owner->velocity.y = 0;
}

void Player::WallSlideState::OnUpdate(float elapsed_time)
{
	if (owner->IsGround())
	{
		owner->wall_climb = false;
		owner->SetState(StateId::Idle);
		return;
	}

	if (!owner->WallJudgement(elapsed_time))
	{
		owner->wall_climb = false;
		owner->SetState(StateId::JumpFall);
		return;
	}

	if (owner->InputAwayFromWall())
	{
		owner->wall_climb = false;
		owner->SetState(StateId::JumpFall);
		return;
	}

	if (owner->InputJump())
	{
		owner->wall_climb = false;

		owner->velocity.x = owner->wallNormal.x * kick_power;
		owner->wall_kick_lock_timer = owner->wall_kick_lock_duration;
		owner->rotation.y = atan2f(owner->wallNormal.x, owner->wallNormal.z);

		owner->SetState(StateId::WallKick);
		return;
	}
}

void Player::WallSlideState::OnExit()
{
	owner->wall_climb = false;
}

void Player::WallKickState::OnEnter()
{
	owner->animator->Play(0, "Jump_Start_0_Seq_0", false);
}

void Player::WallKickState::OnUpdate(float elapsed_time)
{
	if (owner->is_dash_jump)
	{
		owner->UpdateMotionTrail(elapsed_time);
	}

	bool move = false;

	if (owner->wall_kick_lock_timer <= 0.0f)
	{
		move = owner->InputMove();

		if (owner->WallJudgement(elapsed_time) && owner->InputTowardWall())
		{
			owner->is_dash_jump = false;
			owner->SetState(StateId::WallSlide);
			return;
		}
	}

	if (owner->IsGround())
	{
		owner->is_dash_jump = false;
		owner->trails.clear();
		if (owner->InputMove())
		{
			owner->is_dash_jump = false;
			owner->SetState(StateId::Move);
		}
		else
		{
			owner->is_dash_jump = false;
			owner->SetState(StateId::Idle);
		}
	}
}

void Player::AttackState::OnEnter()
{
	owner->combo_step = 1;
	owner->has_next_combo_input = false;

	owner->is_upper_body_active = false;
	owner->animator->Play(0, "ComboAttack0301Seq", false, 0.1f);
	owner->animator->SetLayerSpeed(
		0,
		owner->IsGround() ? GROUND_ATTACK_PLAYBACK_SPEED : AIR_ATTACK_PLAYBACK_SPEED);

	// 攻撃開始時に移動をリセット
	owner->velocity.x = 0.0f;
	owner->velocity.z = 0.0f;
	owner->input_move_x = 0.0f;
	owner->input_move_z = 0.0f;

	owner->SetBladeActive(true);
	owner->SetTrailActive(false);

	owner->is_air_attack = !owner->IsGround();
}

void Player::AttackState::OnUpdate(float elapsed_time)
{
	InputManager& game_pad = InputManager::Instance();
	bool is_grounded = owner->IsGround();

	if (owner->is_air_attack && is_grounded)
	{
		owner->SetBladeActive(false);
		owner->SetTrailActive(false);
		owner->animator->SetLayerSpeed(0, 1.0f);
		owner->SetState(StateId::Idle);
		return;
	}

	owner->is_air_attack = !is_grounded;

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
			if (is_grounded)
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
	if (owner->combo_step >= 1 && owner->combo_step <= 3)
	{
		if (game_pad.GetButtonDown() & InputManager::BTN_X)
		{
			owner->has_next_combo_input = true;
		}
	}

	bool is_playing = owner->animator->IsPlaying(0);

	// 振り抜き中だけをトレイルとして表示
	const bool isComboSwing = owner->combo_step >= 1 && owner->combo_step <= 3
		&& progress >= 0.05f && progress < 0.30f;
	owner->SetTrailActive(isComboSwing);

	// 攻撃モーション
	if (owner->combo_step >= 1 && owner->combo_step <= 3)
	{
		const float combo_cancel_start = is_grounded ? 0.2f : AIR_COMBO_CANCEL_START;
		const float combo_cancel_end = is_grounded ? 0.65f : 0.80f;

		if (owner->has_next_combo_input && progress >= combo_cancel_start && progress <= combo_cancel_end)
		{
			owner->has_next_combo_input = false;

			if (is_grounded)
			{
				if (owner->combo_step == 1)
				{
					owner->combo_step = 2;
					owner->animator->Play(0, "ComboAttack0302Seq", false, 0.03f);
				}
				else if (owner->combo_step == 2)
				{
					owner->combo_step = 3;
					owner->animator->Play(0, "ComboAttack0401Seq", false, 0.03f);
				}
			}
			else
			{
				owner->combo_step = 1;
				owner->animator->Play(0, "ComboAttack0301Seq", false, 0.03f);
			}
		}
		else if (progress >= 0.45f || !is_playing)
		{
			// 次の入力がなければ納刀を挟まずに直接Idleへ戻す（硬直カット）[cite: 1]
			owner->SetBladeActive(false);
			owner->SetTrailActive(false);
			owner->animator->SetLayerSpeed(0, 1.0f);

			// 移動入力があればそのままMove、無ければIdle[cite: 1]
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
    clear_audio_played = exit_started = scene_requested = false;
    EnemyBoss::SetDefeat(false);
    owner->velocity = { 0.0f, 0.0f, 0.0f };
    owner->invincible_time = scene_change_time;
    owner->input_move_x = owner->input_move_z = 0.0f;
    owner->SetBladeActive(false);
    owner->SetTrailActive(false);
    owner->prev_blade_active = owner->prev_trail_active = false;
    owner->trails.clear();
    owner->is_upper_body_active = false;
    owner->upper_body_weight = 0.0f;
    owner->animator->SetLayerState(1, 0.0f, AnimationBlendMode::Override);
    owner->animator->Play(0, "Idle_Seq_0", true);
    Audio::Instance().Stop("SE_PLAYER_DAMAGE_SPARK");
    Audio::Instance().Stop("SE_PLAYER_RUN");
    Audio::Instance().Stop("SE_PLAYER_CHARGE");
}

void Player::ClearState::OnUpdate(float elapsed_time)
{
    const float step = (std::max)(elapsed_time, 0.0f);
    timer += step;
    if (!exit_started)
    {
        owner->velocity.x = owner->velocity.z = 0.0f;
        owner->velocity.y -= owner->gravity * step;
        owner->MoveAndCollide(step);
    }
    if (timer >= clear_audio_time && !clear_audio_played)
    {
        clear_audio_played = true;
        Audio::Instance().Play("SE_CLEAR");
    }
    if (timer >= exit_effect_time && !exit_started)
    {
        exit_started = true;
        if (!exit_effect) exit_effect = std::make_unique<Effect>("Data/Effect/ExitEffect.efkefc");
        exit_handle = exit_effect->Play(owner->position);
        Audio::Instance().Play("SE_PLAYER_RETURN");
        owner->scale = { 0.0f, 0.0f, 0.0f };
        owner->velocity = { 0.0f, 0.0f, 0.0f };
    }
    if (timer >= scene_change_time && !scene_requested)
    {
        scene_requested = true;
        SceneManager::Instance().ChangeScene([]() {
            return std::make_shared<SceneLoading>([]() { return std::make_shared<SceneClear>(); });
        });
    }
}

void Player::ClearState::OnExit()
{
    if (exit_effect && exit_handle >= 0) exit_effect->Stop(exit_handle);
    exit_handle = -1;
}

void Player::OnDamaged()
{
	SetState(StateId::Damage);
	velocity = { 0.0f, 0.0f, 0.0f };
	input_move_x = 0.0f;
	input_move_z = 0.0f;
	SetBladeActive(false);
	SetTrailActive(false);
	trails.clear();
}

void Player::OnDead()
{
    SetState(StateId::Death);
    velocity = { 0.0f, 0.0f, 0.0f };
    SetBladeActive(false);
    SetTrailActive(false);
}

void Player::DeathState::OnEnter()
{
    OnExit();
    active = true;
    scene_requested = false;
    timer = 0.0f;
    owner->scale = { 0.0f, 0.0f, 0.0f };
    owner->velocity = { 0.0f, 0.0f, 0.0f };
    owner->input_move_x = owner->input_move_z = 0.0f;
    owner->SetBladeActive(false);
    owner->SetTrailActive(false);
    owner->prev_blade_active = owner->prev_trail_active = false;
    owner->trails.clear();
    owner->is_upper_body_active = false;
    owner->upper_body_weight = 0.0f;
    owner->animator->SetLayerState(1, 0.0f, AnimationBlendMode::Override);
    if (!death_effect) death_effect = std::make_unique<Effect>("Data/Effect/Death.efkefc");
    death_handle = death_effect->Play(owner->position, effect_scale);
    Audio::Instance().StopAll();
    Audio::Instance().Play("SE_DEATH_GENERIC");
    InputManager::Instance().GetGamePad().Vibrate(vibration_power, vibration_power, vibration_duration);
}

void Player::DamageState::OnEnter()
{
	Camera::Instance().StartShake(0.3f, 0.15f);
	HitStopManager::Instance().Request(0.1f);
	owner->animator->Play(0, "Hit_Combat_F_Seq_0", false);
}

void Player::DamageState::OnUpdate(float elapsed_time)
{
	if (!owner->animator->IsPlaying(0))
	{
		owner->SetState(StateId::Idle);
	}
}

void Player::DeathState::OnUpdate(float elapsed_time)
{
    if (!active || scene_requested) return;
    timer += (std::max)(elapsed_time, 0.0f);
    if (timer >= retry_delay)
    {
        scene_requested = true;
        OnExit();
        SceneManager::Instance().ChangeScene([]() {
            return std::make_shared<SceneLoading>([]() { return std::make_shared<GameScene>(); });
        });
    }
}

void Player::DeathState::OnExit()
{
    if (death_effect && death_handle >= 0) death_effect->Stop(death_handle);
    death_handle = -1;
    if (active) InputManager::Instance().GetGamePad().StopVibration();
    active = false;
}

void Player::SpawnState::OnEnter()
{
    // Effekseer uses the immediate context; load on the main update thread.
    if (!owner->spawnEffect)
        owner->spawnEffect = std::make_unique<Effect>("Data/Effect/SpawnEffect.efkefc");
	owner->animator->Play(0, "Spawn", false);
	owner->spawnHandle = owner->spawnEffect->Play(owner->position);
}

void Player::SpawnState::OnUpdate(float elapsed_time)
{
	spawnTimer += elapsed_time;

	if (!owner->animator->IsPlaying(0) && spawnTimer > SPAWN_END_TIME)
	{
		owner->SetState(StateId::Idle);
		//Audio::Instance().Play("SE_PLAYER_AFTER_APPEAR");
	}
	else if (spawnTimer > SPAWN_ANIM_START_TIME && !hasSpawned)
	{
		owner->scale = { 0.01f, 0.01f, 0.01f };
		hasSpawned = true;
	}
}
