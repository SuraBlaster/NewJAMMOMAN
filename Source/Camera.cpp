#include "Camera.h"
#include <cstdlib>

// コンストラクタ
Camera::Camera()
{
	// カメラ設定
	SetPerspectiveFov(
		DirectX::XMConvertToRadians(45),	// 画角
		1280.0f / 720.0f,					// 画面アスペクト比
		0.1f,								// ニアクリップ
		1000.0f								// ファークリップ
	);
	SetLookAt(
		{ 0, 0, -5 },		// 視点
		{ 0, 0, 0 },		// 注視点
		{ 0, 1, 0 }			// 上ベクトル
	);
}

// 指定方向を向く
void Camera::SetLookAt(const DirectX::XMFLOAT3& eye, const DirectX::XMFLOAT3& focus, const DirectX::XMFLOAT3& up)
{
	DirectX::XMFLOAT3 finalEye = {
		eye.x + shakeOffset.x,
		eye.y + shakeOffset.y,
		eye.z + shakeOffset.z
	};

	DirectX::XMFLOAT3 finalFocus = {
		focus.x + shakeOffset.x,
		focus.y + shakeOffset.y,
		focus.z + shakeOffset.z
	};

	// 視点、注視点、上方向からビュー行列を作成
	DirectX::XMVECTOR eyeVector = DirectX::XMLoadFloat3(&finalEye);
	DirectX::XMVECTOR focusVector = DirectX::XMLoadFloat3(&finalFocus);
	DirectX::XMVECTOR upVector = DirectX::XMLoadFloat3(&up);
	DirectX::XMMATRIX viewMatrix = DirectX::XMMatrixLookAtLH(eyeVector, focusVector, upVector);
	DirectX::XMStoreFloat4x4(&view, viewMatrix);

	// ビューを逆行列化し、ワールド行列に戻す
	DirectX::XMMATRIX worldMatrix = DirectX::XMMatrixInverse(nullptr, viewMatrix);
	DirectX::XMFLOAT4X4 world;
	DirectX::XMStoreFloat4x4(&world, worldMatrix);

	// カメラの方向を取り出す
	this->right.x = world._11;
	this->right.y = world._12;
	this->right.z = world._13;

	this->up.x = world._21;
	this->up.y = world._22;
	this->up.z = world._23;

	this->front.x = world._31;
	this->front.y = world._32;
	this->front.z = world._33;

	// 視点、注視点を保存
	this->eye = eye;
	this->focus = focus;
}

// パースペクティブ設定
void Camera::SetPerspectiveFov(float fovY, float aspect, float nearZ, float farZ)
{
	// 画角、画面比率、クリップ距離からプロジェクション行列を作成
	DirectX::XMMATRIX projectionMatrix = DirectX::XMMatrixPerspectiveFovLH(fovY, aspect, nearZ, farZ);

	//奥行きが気にならないようにする(orthoWeightの値を大きくすると効果あり)
	DirectX::XMMATRIX ortho = DirectX::XMMatrixOrthographicLH(1920 * 0.01f, 1080 * 0.01f, nearZ, farZ);
	orthoWeight = 0.8f;
	perspectiveWeight = 1.0f - orthoWeight;

	DirectX::XMMATRIX s = DirectX::XMMatrixSet(orthoWeight, 0, 0, 0, 0, orthoWeight, 0, 0, 0, 0, orthoWeight, 0, 0, 0, 0, orthoWeight);
	DirectX::XMMATRIX a = DirectX::XMMatrixSet(perspectiveWeight, 0, 0, 0, 0, perspectiveWeight, 0, 0, 0, 0, perspectiveWeight, 0, 0, 0, 0, perspectiveWeight);
	DirectX::XMStoreFloat4x4(&projection, projectionMatrix * a + ortho * s);
}

void Camera::StartShake(float magnitude, float duration)
{
	this->shakeMagnitude = magnitude;
	this->shakeTimer = duration;
}

void Camera::SetLookAtWithShake(const DirectX::XMFLOAT3& eye, const DirectX::XMFLOAT3& focus, const DirectX::XMFLOAT3& up)
{
	// SetLookAt applies the shake once and preserves the unshaken position.
	SetLookAt(eye, focus, up);
}

void Camera::ShakeUpdate(float elapsedTime)
{
	if (shakeTimer > 0.0f)
	{
		shakeTimer -= elapsedTime;

		// -1.0f ～ 1.0f のランダム値 * 強度
		float rX = ((float)rand() / RAND_MAX * 2.0f - 1.0f) * shakeMagnitude;
		float rY = ((float)rand() / RAND_MAX * 2.0f - 1.0f) * shakeMagnitude;
		float rZ = ((float)rand() / RAND_MAX * 2.0f - 1.0f) * shakeMagnitude;

		shakeOffset = { rX, rY, rZ };

		if (shakeTimer <= 0.0f)
		{
			shakeTimer = 0.0f;
			shakeOffset = { 0.0f, 0.0f, 0.0f };
		}
	}
}
