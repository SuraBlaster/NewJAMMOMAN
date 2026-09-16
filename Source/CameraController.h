#pragma once

#include <DirectXMath.h>
#include <imgui.h> 

// カメラコントローラー
class CameraController
{
public:
	// カメラモードの定義
	enum class CameraMode
	{
		Free,       // 自由視点カメラ
		Follow2D    // 2D追従カメラ
	};

	// 更新処理
	void Update(
		float elapsedTime,
		const DirectX::XMFLOAT3& playerPos,
		const DirectX::XMFLOAT3& playerVelocity);

	// メニューバーの描画
	void DrawMenuBar();

	// ターゲット位置設定
	void SetTarget(const DirectX::XMFLOAT3& target) { this->target = target; }

	void UpdateFixed2D(float elapsedTime, const DirectX::XMFLOAT3& fixedFocus);

	void Zoom(float wheelDelta)
	{
		range -= wheelDelta * range * 0.1f;
		if (range < 2.0f) range = 2.0f;
		if (range > 30.0f) range = 30.0f;
	}

	// モードの取得と設定
	CameraMode GetMode() const { return mode; }
	void SetMode(CameraMode newMode) { mode = newMode; }

private:
	CameraMode				mode = CameraMode::Free;

	DirectX::XMFLOAT3		target = { 0, 0, 0 };	// 注視点
	DirectX::XMFLOAT3		angle = { DirectX::XMConvertToRadians(45), 0, 0 };	// 回転角度
	float					roll_speed = DirectX::XMConvertToRadians(90); // 回転速度
	float					range = 10.0f; 	// 距離
	float					max_angle_x = DirectX::XMConvertToRadians(45);
	float					min_angle_x = DirectX::XMConvertToRadians(-45);
	float					interpolation_speed = 3.0f; // 補間速度

	// Follow2D look-ahead settings. Keep looking in the last movement direction
	// while stopped, and smoothly cross to the other side on a direction change.
	float					look_ahead_distance = 2.0f;
	float					look_ahead_lerp_speed = 3.0f;
	float					look_ahead_velocity_threshold = 0.1f;
	float					last_move_direction_x = 0.0f;
	float					current_look_ahead_x = 0.0f;

	// 固定2Dカメラの注視点からの距離。
	float fixed_2d_distance = 10.0f;
};
