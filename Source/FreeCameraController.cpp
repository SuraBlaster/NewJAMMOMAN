#include <imgui.h>
#include "FreeCameraController.h"

// カメラからコントローラーへパラメータを同期する
void FreeCameraController::SyncCameraToController(const Camera& camera)
{
	eye = camera.GetEye();
	focus = camera.GetFocus();
	up = camera.GetUp();
	right = camera.GetRight();

	// 視点から注視点までの距離を算出
	DirectX::XMVECTOR eyeVector = DirectX::XMLoadFloat3(&eye);
	DirectX::XMVECTOR focusVector = DirectX::XMLoadFloat3(&focus);
	DirectX::XMVECTOR vec = DirectX::XMVectorSubtract(focusVector, eyeVector);
	DirectX::XMVECTOR distanceVector = DirectX::XMVector3Length(vec);
	DirectX::XMStoreFloat(&distance, distanceVector);

	// 回転角度を算出
	const DirectX::XMFLOAT3& front = camera.GetFront();
	angleX = ::asinf(-front.y);
	if (up.y < 0)
	{
		if (front.y > 0)
		{
			angleX = -DirectX::XM_PI - angleX;
		}
		else
		{
			angleX = DirectX::XM_PI - angleX;
		}
		angleY = ::atan2f(front.x, front.z);
	}
	else
	{
		angleY = ::atan2f(-front.x, -front.z);
	}

}

// コントローラーからカメラへパラメータを同期する
void FreeCameraController::SyncControllerToCamera(Camera& camera)
{
	camera.SetLookAt(eye, focus, up);
}

// 更新処理
void FreeCameraController::Update()
{
	// デバッグウインドウ操作中は処理しない
	if (ImGui::IsWindowFocused(ImGuiFocusedFlags_AnyWindow))
	{
		return;
	}

	// IMGUIのマウス入力値を使ってカメラ操作する
	ImGuiIO io = ImGui::GetIO();

	// マウスカーソルの移動量を求める
	float moveX = io.MouseDelta.x * 0.02f;
	float moveY = io.MouseDelta.y * 0.02f;

	// マウス左ボタン押下中
	if (io.MouseDown[ImGuiMouseButton_Right])
	{
		// Y軸回転
		angleY += moveX * 0.5f;
		if (angleY > DirectX::XM_PI)
		{
			angleY -= DirectX::XM_2PI;
		}
		else if (angleY < -DirectX::XM_PI)
		{
			angleY += DirectX::XM_2PI;
		}
		// X軸回転
		angleX += moveY * 0.5f;
		if (angleX > DirectX::XM_PI)
		{
			angleX -= DirectX::XM_2PI;
		}
		else if (angleX < -DirectX::XM_PI)
		{
			angleX += DirectX::XM_2PI;
		}
	}
	// マウス中ボタン押下中
	else if (io.MouseDown[ImGuiMouseButton_Middle])
	{
		// 平行移動
		float s = distance * 0.035f;
		float x = moveX * s;
		float y = moveY * s;

		focus.x -= right.x * x;
		focus.y -= right.y * x;
		focus.z -= right.z * x;

		focus.x += up.x * y;
		focus.y += up.y * y;
		focus.z += up.z * y;
	}
	// マウス右ボタン押下中
	else if (io.MouseDown[ImGuiMouseButton_Left] && io.MouseDown[ImGuiMouseButton_Right])
	{
		// ズーム
		distance += (-moveY - moveX) * distance * 0.1f;
	}
	// マウスホイール
	else if (io.MouseWheel != 0)
	{
		// ズーム
		distance -= io.MouseWheel * distance * 0.1f;
	}

	float sx = ::sinf(angleX);
	float cx = ::cosf(angleX);
	float sy = ::sinf(angleY);
	float cy = ::cosf(angleY);

	// カメラの方向を算出
	DirectX::XMVECTOR frontVector = DirectX::XMVectorSet(-cx * sy, -sx, -cx * cy, 0.0f);
	DirectX::XMVECTOR rightVector = DirectX::XMVectorSet(cy, 0, -sy, 0.0f);
	DirectX::XMVECTOR upVector = DirectX::XMVector3Cross(rightVector, frontVector);
	// カメラの視点＆注視点を算出
	DirectX::XMVECTOR focusVector = DirectX::XMLoadFloat3(&focus);
	DirectX::XMVECTOR distanceVector = DirectX::XMVectorSet(distance, distance, distance, 0.0f);
	DirectX::XMVECTOR eyeVector = DirectX::XMVectorSubtract(focusVector, DirectX::XMVectorMultiply(frontVector, distanceVector));
	// ビュー行列からワールド行列を算出
	DirectX::XMMATRIX view = DirectX::XMMatrixLookAtLH(eyeVector, focusVector, upVector);
	DirectX::XMMATRIX world = DirectX::XMMatrixTranspose(view);
	// ワールド行列から方向を算出
	rightVector = DirectX::XMVector3TransformNormal(DirectX::XMVectorSet(1, 0, 0, 0), world);
	upVector = DirectX::XMVector3TransformNormal(DirectX::XMVectorSet(0, 1, 0, 0), world);
	// 結果を格納
	DirectX::XMStoreFloat3(&eye, eyeVector);
	DirectX::XMStoreFloat3(&up, upVector);
	DirectX::XMStoreFloat3(&right, rightVector);
}
