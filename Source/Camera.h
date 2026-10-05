#pragma once

#include <DirectXMath.h>

// カメラ
class Camera
{
private:
	Camera();
	~Camera() = default;

public:
	// インスタンス取得
	static Camera& Instance()
	{
		static Camera instance;
		return instance;
	}

	// 指定方向を向く
	void SetLookAt(const DirectX::XMFLOAT3& eye, const DirectX::XMFLOAT3& focus, const DirectX::XMFLOAT3& up);

	// パースペクティブ設定
	void SetPerspectiveFov(float fovY, float aspect, float nearZ, float farZ);

	// カメラシェイク開始
	void StartShake(float magnitude, float duration);

	// シェイクのオフセットを加算するSetLookAt
	void SetLookAtWithShake(const DirectX::XMFLOAT3& eye, const DirectX::XMFLOAT3& focus, const DirectX::XMFLOAT3& up);

	// シェイク更新
	void ShakeUpdate(float elapsedTime);

	// ビュー行列取得
	const DirectX::XMFLOAT4X4& GetView() const { return view; }

	// プロジェクション行列取得
	const DirectX::XMFLOAT4X4& GetProjection() const { return projection; }

	// 視点取得
	const DirectX::XMFLOAT3& GetEye() const { return eye; }

	// 注視点取得
	const DirectX::XMFLOAT3& GetFocus() const { return focus; }

	// 上方向取得
	const DirectX::XMFLOAT3& GetUp() const { return up; }

	// 前方向取得
	const DirectX::XMFLOAT3& GetFront() const { return front; }

	// 右方向取得
	const DirectX::XMFLOAT3& GetRight() const { return right; }

private:
	DirectX::XMFLOAT4X4		view;
	DirectX::XMFLOAT4X4		projection;

	DirectX::XMFLOAT3		eye;
	DirectX::XMFLOAT3		focus;

	DirectX::XMFLOAT3		up;
	DirectX::XMFLOAT3		front;
	DirectX::XMFLOAT3		right;

private:
	float orthoWeight;
	float perspectiveWeight;

	float shakeTimer = 0.0f;
	float shakeMagnitude = 0.0f;
	DirectX::XMFLOAT3 shakeOffset = { 0.0f, 0.0f, 0.0f };
};
