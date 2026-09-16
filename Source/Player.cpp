#include "Player.h"
#include "Camera.h"
#include "GamePad.h"
#include "Graphics.h"
#include "CollisionManager.h"
#include "Collision.h"
#include "EnemyManager.h"
#include "SetStage.h"
#include <ImGui.h>

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
	model = std::make_shared<Model>(device, filename);
	animator = std::make_unique<Animator>(model.get(), 2);

	SetStage::outputData data;
	data.filename = model->GetFileName();
	data.id = 57;
	SetStage::JsonOutput("test.json", data);

	// 上半身マスクの生成
	upper_body_mask.BuildLayerMaskFromRoot(model.get(), "mixamorig:Spine1");

	position.z = 0.0f;
	rotation.y = DirectX::XMConvertToRadians(90);

	scale.x = scale.y = scale.z = 0.01f;

	// ステートの生成
	states[static_cast<size_t>(StateId::Idle)] = std::make_unique<IdleState>(this);
	states[static_cast<size_t>(StateId::Move)] = std::make_unique<MoveState>(this);
	states[static_cast<size_t>(StateId::Attack)] = std::make_unique<AttackState>(this);
	states[static_cast<size_t>(StateId::Jump)] = std::make_unique<JumpState>(this);
	states[static_cast<size_t>(StateId::JumpFall)] = std::make_unique<JumpFallState>(this);
	states[static_cast<size_t>(StateId::Dash)] = std::make_unique<DashState>(this);
	states[static_cast<size_t>(StateId::WallSlide)] = std::make_unique<WallSlideState>(this);
	states[static_cast<size_t>(StateId::WallKick)] = std::make_unique<WallKickState>(this);
	SetState(StateId::Idle);

	
}

void Player::Update(float elapsed_time)
{
	UpdateStateMachine(elapsed_time);

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
		ImGui::Checkbox("Show sword collision", &show_sword_collision);
	}
	ImGui::End();

	if (ImGui::BeginMainMenuBar())
	{
		if (ImGui::BeginMenu("How To Use"))
		{
			ImGui::Text("X:Attack");
			ImGui::Text("A:Jump or WallKick");
			ImGui::Text("B or LT:Dash");

			ImGui::EndMenu();
		}

		ImGui::EndMainMenuBar();
	}
}

void Player::DrawDebugPrimitive(ShapeRenderer* shapeRenderer) const
{
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
			bool is_reversing = (!is_ground && (velocity.x * vec_x < 0.0f));

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

	// 移動量
	float move_x = velocity.x * elapsed_time;
	float move_y = velocity.y * elapsed_time;
	float move_z = velocity.z * elapsed_time;

	// 水平移動処理
	float move_xz_length = sqrtf(move_x * move_x + move_z * move_z);
	if (move_xz_length > 0)
	{
		// キャラクターの半径（当たり判定の太さ）に合わせて調整してください
		float margin = 0.4f;

		// 移動方向を正規化
		float move_dx = move_x / move_xz_length;
		float move_dz = move_z / move_xz_length;

		// レイの始点
		DirectX::XMFLOAT3 s = {
			position.x,
			position.y + 0.5f,
			position.z
		};

		// 実際の移動予定位置（終点）
		DirectX::XMFLOAT3 e = {
			position.x + move_x,
			position.y + 0.5f,
			position.z + move_z
		};

		// めり込み防止のため、レイをマージン分だけ長く飛ばして壁を「早めに」検知する
		DirectX::XMFLOAT3 ray_e = {
			position.x + move_dx * (move_xz_length + margin),
			position.y + 0.5f,
			position.z + move_dz * (move_xz_length + margin)
		};

		HitResult hit_result;
		// 延長した ray_e でレイキャストを行う
		if (CollisionManager::Instance().Raycast(s, ray_e, hit_result))
		{
			DirectX::XMVECTOR P = DirectX::XMLoadFloat3(&hit_result.position);
			DirectX::XMVECTOR E = DirectX::XMLoadFloat3(&e); // 実際の移動先
			DirectX::XMVECTOR N = DirectX::XMLoadFloat3(&hit_result.normal);

			// 移動予定位置(E) から 壁の交点(P) へのベクトル
			DirectX::XMVECTOR EP = DirectX::XMVectorSubtract(E, P);

			// E が壁平面からどれだけ離れているか（法線方向への射影）
			float dist_to_plane = DirectX::XMVectorGetX(DirectX::XMVector3Dot(EP, N));

			DirectX::XMFLOAT3 q;

			// 実際の移動先 E が、壁からマージンの内側に入ろうとしている場合のみ押し出す
			if (dist_to_plane < margin)
			{
				// 押し出し量 = 保ちたいマージン - 現在の壁との距離
				float push_amount = margin - dist_to_plane;

				// 壁に沿うように押し出した位置 Q を求める
				DirectX::XMVECTOR Q = DirectX::XMVectorAdd(E, DirectX::XMVectorScale(N, push_amount));
				DirectX::XMStoreFloat3(&q, Q);
			}
			else
			{
				// マージンより手前で止まる移動なら、そのまま移動させる
				q = e;
			}

			// 壁際で壁ずり後の位置がめり込んでいないかレイキャストでチェックする
			if (CollisionManager::Instance().Raycast(s, q, hit_result))
			{
				// めり込んでいた場合はプレイヤーの位置に今回レイキャストした交点を設定する
				P = DirectX::XMLoadFloat3(&hit_result.position);
				DirectX::XMVECTOR S = DirectX::XMLoadFloat3(&s);
				DirectX::XMVECTOR PS = DirectX::XMVectorSubtract(S, P);
				DirectX::XMVECTOR V = DirectX::XMVector3Normalize(PS);
				P = DirectX::XMVectorAdd(P, DirectX::XMVectorScale(V, 0.001f));
				DirectX::XMStoreFloat3(&q, P);
			}

			position.x = q.x;
			position.z = q.z;
		}
		else
		{
			// 壁に当たらなかったので普通に移動
			position.x += move_x;
			position.z += move_z;
		}
	}

	// 上下移動処理
	DirectX::XMFLOAT3 start = { position.x, position.y + 1, position.z };
	DirectX::XMFLOAT3 end = { position.x, position.y + move_y, position.z };
	HitResult hit_result;
	if (CollisionManager::Instance().Raycast(start, end, hit_result))
	{
		position.y = hit_result.position.y;
		velocity.y = 0.0f;
		is_ground = true;
	}
	else
	{
		position.y += velocity.y * elapsed_time;
		is_ground = false;
	}
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
	GamePad& game_pad = GamePad::Instance();
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
	GamePad& game_pad = GamePad::Instance();

	if ((game_pad.GetButtonDown() & GamePad::BTN_A) && (game_pad.GetButton() & GamePad::BTN_B) && current_state == StateId::WallSlide
		|| (game_pad.GetButtonDown() & GamePad::BTN_A) && (game_pad.GetButton() & GamePad::BTN_LEFT_TRIGGER) && current_state == StateId::WallSlide)
	{
		velocity.y = jump_speed * 1.2f;
		is_dash_jump = true;
		return true;
	}

	if ((game_pad.GetButtonDown() & GamePad::BTN_A) && (game_pad.GetButton() & GamePad::BTN_B)
		|| (game_pad.GetButtonDown() & GamePad::BTN_A) && (game_pad.GetButton() & GamePad::BTN_LEFT_TRIGGER))
	{
		velocity.y = jump_speed * 1.2f;
		is_dash_jump = true;
		return true;
	}

	if ((game_pad.GetButtonDown() & GamePad::BTN_A) && IsGround())
	{
		velocity.y = jump_speed;
		return true;
	}

	return false;
}

bool Player::InputShot()
{
	GamePad& game_pad = GamePad::Instance();
	if (game_pad.GetButtonDown() & GamePad::BTN_X)
	{
		SetState(StateId::Attack);
		return true;
	}
	return false;
}

bool Player::InputDash()
{
	GamePad& game_pad = GamePad::Instance();
	if (game_pad.GetButtonDown() & GamePad::BTN_B || game_pad.GetButtonDown() & GamePad::BTN_LEFT_TRIGGER)
	{
		return true;
	}

	return false;
}

// スティックの入力方向とwallNormalの内積が負のとき壁へ向いていると判定する
bool Player::InputTowardWall()
{
	GamePad& game_pad = GamePad::Instance();
	float axis_x = game_pad.GetAxisLX();

	if (std::abs(axis_x) < 0.1f) return false;

	return (axis_x * wallNormal.x) < 0.0f;
}

// 左スティックが壁と反対の方向へ傾けられているか判定
bool Player::InputAwayFromWall()
{
	GamePad& game_pad = GamePad::Instance();
	float axis_x = game_pad.GetAxisLX();

	if (std::abs(axis_x) < 0.1f) return false;

	return (axis_x * wallNormal.x) > 0.0f;
}

void Player::HoldJump()
{
	GamePad& game_pad = GamePad::Instance();

	bool is_holding_jump = (game_pad.GetButton() & GamePad::BTN_A);

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
		gravity = 2.0f;
		velocity.y -= 2.0f;
	}
	else if (velocity.y < 0.0f)
	{
		gravity = 33.75f;
	}
	else
	{
		gravity = 22.5f;
	}

	velocity.y -= gravity * elapsed_time;

	const float max_fall_speed = -gravity;
	if (velocity.y < max_fall_speed)
	{
		velocity.y = max_fall_speed;
	}
}

bool Player::WallJudgement(float elapsed_time)
{
	wallNormal = {};

	DirectX::XMFLOAT3 startPos = { position.x, position.y + 0.5f, position.z };
	DirectX::XMVECTOR S = DirectX::XMLoadFloat3(&startPos);

	// 前方ベクトルを正規化
	DirectX::XMFLOAT3 forward = { transform._31, transform._32, transform._33 };
	DirectX::XMVECTOR rayDir = DirectX::XMVector3Normalize(DirectX::XMLoadFloat3(&forward));

	// velocity から1秒間の速度(長さ)を取得
	DirectX::XMFLOAT2 XZLength = { velocity.x, velocity.z };
	DirectX::XMVECTOR Vec = DirectX::XMLoadFloat2(&XZLength);
	float speed = DirectX::XMVectorGetX(DirectX::XMVector2Length(Vec));

	// 速度 × 経過時間 で「今フレームの実際の移動量」を算出
	float moveDistance = speed * elapsed_time;

	// 実際の移動量 ＋ キャラクターの半径をチェック距離とする
	float checkDistance = moveDistance + 0.5f;

	// 終点の計算
	DirectX::XMVECTOR E = DirectX::XMVectorAdd(S, DirectX::XMVectorScale(rayDir, checkDistance));

	DirectX::XMFLOAT3 s;
	DirectX::XMFLOAT3 e;
	DirectX::XMStoreFloat3(&s, S);
	DirectX::XMStoreFloat3(&e, E);

	HitResult hit_result;

	// レイキャストを実行
	if (CollisionManager::Instance().Raycast(s, e, hit_result))
	{
		// 取得した法線で、その面が「壁」かどうかを判定する
		if (hit_result.normal.y > -0.3f && hit_result.normal.y < 0.7f)
		{
			wallNormal = hit_result.normal;
			return true;
		}
	}
	return false;
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

	GamePad& game_pad = GamePad::Instance();

	if (game_pad.GetButtonUp() & GamePad::BTN_B || game_pad.GetButtonUp() & GamePad::BTN_LEFT_TRIGGER)
	{
		owner->trails.clear();
		owner->velocity.x = 0.0f;
		owner->velocity.z = 0.0f;
		owner->SetState(StateId::Idle);
		return;
	}

	if (owner->dash_timer <= 0.0f)
	{
		owner->velocity.x = 0.0f;
		owner->velocity.z = 0.0f;

		owner->trails.clear();

		owner->SetState(StateId::Idle);
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

	GamePad& game_pad = GamePad::Instance();
	if (game_pad.GetButtonDown() & GamePad::BTN_A)
	{
		owner->wall_climb = false;

		if (game_pad.GetButton() & GamePad::BTN_B || game_pad.GetButton() & GamePad::BTN_LEFT_TRIGGER)
		{
			owner->velocity.y = owner->jump_speed * 1.2f;
			owner->is_dash_jump = true;
		}
		else
		{
			owner->velocity.y = owner->jump_speed;
			owner->is_dash_jump = false;
		}

		owner->velocity.x = owner->wallNormal.x * kick_power;
		owner->wall_kick_lock_timer = owner->wall_kick_lock_duration;
		owner->rotation.y = atan2f(owner->wallNormal.x, owner->wallNormal.z);

		owner->SetState(StateId::WallKick);
		return;
	}
	if (owner->IsGround())
	{
		owner->wall_climb = false;
		owner->SetState(StateId::Idle);
		return;
	}
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

		if (owner->WallJudgement(elapsed_time))
		{
			owner->is_dash_jump = false;
			owner->SetState(StateId::WallSlide);
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
	GamePad& game_pad = GamePad::Instance();
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
		if (game_pad.GetButtonDown() & GamePad::BTN_X)
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

