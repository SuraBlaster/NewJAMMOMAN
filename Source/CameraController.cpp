#include "InputManager.h"
#include "CameraController.h"
#include "Camera.h"
#include "Player.h"

#include <cmath>

// 更新処理
void CameraController::Update(
	float elapsed_time,
	const DirectX::XMFLOAT3& playerPos,
	const DirectX::XMFLOAT3& playerVelocity,
	const CameraLimitZoneData* cameraLimitZoneData)
{
	Camera& camera = Camera::Instance();

	if (mode == CameraMode::Free)
	{
		current_look_ahead_x = 0.0f;
		pending_move_direction_x = 0.0f;
		direction_change_timer = 0.0f;

		InputManager& game_pad = InputManager::Instance();
		float ax = game_pad.GetAxisRX();
		float ay = game_pad.GetAxisRY();
		// カメラの回転速度
		float speed = roll_speed * elapsed_time;

		// スティックの入力値に合わせてX軸とY軸を回転
		angle.x += ay * speed;
		angle.y += ax * speed;

		// X軸のカメラ回転を制限
		if (angle.x < min_angle_x)
		{
			angle.x = min_angle_x;
		}
		if (angle.x > max_angle_x)
		{
			angle.x = max_angle_x;
		}

		// Y軸の回転値を-3.14～3.14に収まるようにする
		if (angle.y < -DirectX::XM_PI)
		{
			angle.y += DirectX::XM_2PI;
		}
		if (angle.y > DirectX::XM_PI)
		{
			angle.y -= DirectX::XM_2PI;
		}

		// カメラ回転値を回転行列に変換
		DirectX::XMMATRIX Transform = DirectX::XMMatrixRotationRollPitchYaw(angle.x, angle.y, angle.z);

		// 回転行列から前方向ベクトルを取り出す
		DirectX::XMVECTOR Front = Transform.r[2];
		DirectX::XMFLOAT3 front;
		DirectX::XMStoreFloat3(&front, Front);

		// 注視点から後ろベクトル方向に一定距離離れたカメラ視点を求める
		DirectX::XMFLOAT3 eye;
		eye.x = target.x - front.x * range;
		eye.y = target.y - front.y * range;
		eye.z = target.z - front.z * range;

		// 補間処理
		float t = interpolation_speed * elapsed_time;
		DirectX::XMVECTOR Eye = DirectX::XMLoadFloat3(&camera.GetEye());
		DirectX::XMVECTOR Focus = DirectX::XMLoadFloat3(&camera.GetFocus());
		DirectX::XMVECTOR FinalEye = DirectX::XMLoadFloat3(&eye);
		DirectX::XMVECTOR FinalFocus = DirectX::XMLoadFloat3(&target);
		DirectX::XMStoreFloat3(&eye, DirectX::XMVectorLerp(Eye, FinalEye, t));
		DirectX::XMStoreFloat3(&target, DirectX::XMVectorLerp(Focus, FinalFocus, t));

		// カメラの視点と注視点を設定
		camera.SetLookAt(eye, target, DirectX::XMFLOAT3(0, 1, 0));
	}
	else if (mode == CameraMode::Follow2D)
	{
		const float safeElapsedTime = elapsed_time > 0.0f ? elapsed_time : 0.0f;

		// A brief opposite impulse, such as the start of a wall kick, must continue
		// for a short time before it changes the camera's look-ahead direction.
		if (std::abs(playerVelocity.x) > look_ahead_velocity_threshold)
		{
			const float movementDirection =
				playerVelocity.x > 0.0f ? 1.0f : -1.0f;

			if (last_move_direction_x == 0.0f)
			{
				last_move_direction_x = movementDirection;
				pending_move_direction_x = 0.0f;
				direction_change_timer = 0.0f;
			}
			else if (movementDirection == last_move_direction_x)
			{
				pending_move_direction_x = 0.0f;
				direction_change_timer = 0.0f;
			}
			else
			{
				if (pending_move_direction_x != movementDirection)
				{
					pending_move_direction_x = movementDirection;
					direction_change_timer = 0.0f;
				}

				direction_change_timer += safeElapsedTime;
				if (direction_change_timer >= direction_change_confirm_time)
				{
					last_move_direction_x = pending_move_direction_x;
					pending_move_direction_x = 0.0f;
					direction_change_timer = 0.0f;
				}
			}
		}
		else
		{
			// The opposite direction must be continuous to be confirmed.
			pending_move_direction_x = 0.0f;
			direction_change_timer = 0.0f;
		}

		const float desiredLookAheadX =
			last_move_direction_x * look_ahead_distance;
		const float blend = 1.0f - std::exp(
			-look_ahead_lerp_speed * safeElapsedTime);
		current_look_ahead_x +=
			(desiredLookAheadX - current_look_ahead_x) * blend;

		// Smoothly move the whole camera vertically with the player. Using the
		// current focus also makes transitions from a fixed camera zone seamless.
		const float verticalBlend = 1.0f - std::exp(
			-vertical_follow_lerp_speed * safeElapsedTime);
		const float currentFocusY = camera.GetFocus().y;
		const float cameraY = currentFocusY +
			(playerPos.y - currentFocusY) * verticalBlend;

		const float cameraX = playerPos.x + current_look_ahead_x;
		DirectX::XMFLOAT3 focus = { cameraX, cameraY, playerPos.z };
		DirectX::XMFLOAT3 eye =
		{
			cameraX,
			cameraY + vertical_eye_offset,
			playerPos.z - 10.0f
		};

		if (cameraLimitZoneData == nullptr)
		{
			// カメラの視点と注視点を設定
			camera.SetLookAt(eye, focus, DirectX::XMFLOAT3(0, 1, 0));
		}
		else
		{
			correction_value = {};

			fixed_minX = cameraLimitZoneData->minX + 8.176f;
			fixed_maxX = cameraLimitZoneData->maxX - 8.176f;
			fixed_minY = cameraLimitZoneData->minY + 4.622f;
			fixed_maxY = cameraLimitZoneData->maxY - 4.338f;

			// 範囲内に補正
			if (focus.x < fixed_minX)
			{
				correction_value.x = fixed_minX - focus.x;
				focus.x = fixed_minX;
			}

			if (focus.x > fixed_maxX)
			{
				correction_value.x = fixed_maxX - focus.x;
				focus.x = fixed_maxX;
			}

			if (focus.y < fixed_minY)
			{
				correction_value.y = fixed_minY - focus.y;
				focus.y = fixed_minY;
			}

			if (focus.y > fixed_maxY)
			{
				correction_value.y = fixed_maxY - focus.y;
				focus.y = fixed_maxY;
			}

			eye = { eye.x + correction_value.x ,eye.y + correction_value.y,eye.z };

			// カメラの視点と注視点を設定
			camera.SetLookAt(eye, focus, DirectX::XMFLOAT3(0, 1, 0));

		}
	}
}

// ImGuiメニューの描画
void CameraController::DrawMenuBar()
{
	if (ImGui::BeginMainMenuBar())
	{
		if (ImGui::BeginMenu("Camera"))
		{
			bool freeSelected = (mode == CameraMode::Free);
			bool followSelected = (mode == CameraMode::Follow2D);

			if (ImGui::MenuItem("Free Camera", nullptr, freeSelected))
			{
				mode = CameraMode::Free;
			}
			if (ImGui::MenuItem("2D Follow Camera", nullptr, followSelected))
			{
				mode = CameraMode::Follow2D;
			}

			ImGui::EndMenu();
		}

		ImGui::EndMainMenuBar();
	}
}

void CameraController::UpdateFixed2D(float elapsedTime, const DirectX::XMFLOAT3& fixedFocus)
{
	Camera& camera = Camera::Instance();

	// 注視点の正面から一定距離だけ後方にカメラを配置する。
	const DirectX::XMFLOAT3 desiredEye =
	{
		fixedFocus.x,
		fixedFocus.y,
		fixedFocus.z - fixed_2d_distance,
	};

	// 負の経過時間が補間計算に入らないようにする。
	const float safeElapsedTime =
		elapsedTime > 0.0f ? elapsedTime : 0.0f;

	// フレームレートに左右されにくい補間率を計算する。
	const float blend =
		1.0f - std::exp(-interpolation_speed * safeElapsedTime);

	const DirectX::XMVECTOR currentEye =
		DirectX::XMLoadFloat3(&camera.GetEye());

	const DirectX::XMVECTOR currentFocus =
		DirectX::XMLoadFloat3(&camera.GetFocus());

	const DirectX::XMVECTOR targetEye =
		DirectX::XMLoadFloat3(&desiredEye);

	const DirectX::XMVECTOR targetFocus =
		DirectX::XMLoadFloat3(&fixedFocus);

	DirectX::XMFLOAT3 interpolatedEye;
	DirectX::XMFLOAT3 interpolatedFocus;

	DirectX::XMStoreFloat3(
		&interpolatedEye,
		DirectX::XMVectorLerp(currentEye, targetEye, blend));

	DirectX::XMStoreFloat3(
		&interpolatedFocus,
		DirectX::XMVectorLerp(currentFocus, targetFocus, blend));

	const DirectX::XMFLOAT3 worldUp = { 0.0f, 1.0f, 0.0f };

	camera.SetLookAt(
		interpolatedEye,
		interpolatedFocus,
		worldUp);
}
