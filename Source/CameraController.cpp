#include "InputManager.h"
#include "CameraController.h"
#include "Camera.h"
#include "Player.h"

#include <cmath>

// 更新処理
void CameraController::Update(float elapsedTime, const DirectX::XMFLOAT3& playerPos,
                              const DirectX::XMFLOAT3& playerVelocity,
                              const std::vector<CameraLimitZoneData>& cameraLimitZones)
{
    Camera& camera = Camera::Instance();

    if (mode == CameraMode::Free)
    {
        currentLookAheadX = 0.0f;
        pendingMoveDirectionX = 0.0f;
        directionChangeTimer = 0.0f;

        InputManager& gamePad = InputManager::Instance();
        float ax = gamePad.GetAxisRX();
        float ay = gamePad.GetAxisRY();
        // カメラの回転速度
        float speed = rollSpeed * elapsedTime;

        // スティックの入力値に合わせてX軸とY軸を回転
        angle.x += ay * speed;
        angle.y += ax * speed;

        // X軸のカメラ回転を制限
        if (angle.x < minAngleX)
        {
            angle.x = minAngleX;
        }

        if (angle.x > maxAngleX)
        {
            angle.x = maxAngleX;
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
        DirectX::XMMATRIX transform = DirectX::XMMatrixRotationRollPitchYaw(angle.x, angle.y, angle.z);

        // 回転行列から前方向ベクトルを取り出す
        DirectX::XMVECTOR frontVector = transform.r[2];
        DirectX::XMFLOAT3 front;
        DirectX::XMStoreFloat3(&front, frontVector);

        // 注視点から後ろベクトル方向に一定距離離れたカメラ視点を求める
        DirectX::XMFLOAT3 eye;
        eye.x = target.x - front.x * range;
        eye.y = target.y - front.y * range;
        eye.z = target.z - front.z * range;

        // 補間処理
        float t = interpolationSpeed * elapsedTime;
        DirectX::XMVECTOR eyeVector = DirectX::XMLoadFloat3(&camera.GetEye());
        DirectX::XMVECTOR focusVector = DirectX::XMLoadFloat3(&camera.GetFocus());
        DirectX::XMVECTOR finalEye = DirectX::XMLoadFloat3(&eye);
        DirectX::XMVECTOR finalFocus = DirectX::XMLoadFloat3(&target);
        DirectX::XMStoreFloat3(&eye, DirectX::XMVectorLerp(eyeVector, finalEye, t));
        DirectX::XMStoreFloat3(&target, DirectX::XMVectorLerp(focusVector, finalFocus, t));

        // カメラの視点と注視点を設定
        camera.SetLookAt(eye, target, DirectX::XMFLOAT3(0, 1, 0));
    }
    else if (mode == CameraMode::Follow2D)
    {
        const float safeElapsedTime = elapsedTime > 0.0f ? elapsedTime : 0.0f;

        if (std::abs(playerVelocity.x) > lookAheadVelocityThreshold)
        {
            const float movementDirection = playerVelocity.x > 0.0f ? 1.0f : -1.0f;

            if (lastMoveDirectionX == 0.0f)
            {
                lastMoveDirectionX = movementDirection;
                pendingMoveDirectionX = 0.0f;
                directionChangeTimer = 0.0f;
            }
            else if (movementDirection == lastMoveDirectionX)
            {
                pendingMoveDirectionX = 0.0f;
                directionChangeTimer = 0.0f;
            }
            else
            {
                if (pendingMoveDirectionX != movementDirection)
                {
                    pendingMoveDirectionX = movementDirection;
                    directionChangeTimer = 0.0f;
                }

                directionChangeTimer += safeElapsedTime;

                if (directionChangeTimer >= directionChangeConfirmTime)
                {
                    lastMoveDirectionX = pendingMoveDirectionX;
                    pendingMoveDirectionX = 0.0f;
                    directionChangeTimer = 0.0f;
                }
            }
        }
        else
        {

            pendingMoveDirectionX = 0.0f;
            directionChangeTimer = 0.0f;
        }

        const float desiredLookAheadX = lastMoveDirectionX * lookAheadDistance;
        const float blend = 1.0f - std::exp(-lookAheadLerpSpeed * safeElapsedTime);
        currentLookAheadX += (desiredLookAheadX - currentLookAheadX) * blend;

        const float verticalBlend = 1.0f - std::exp(-verticalFollowLerpSpeed * safeElapsedTime);
        const float currentFocusY = camera.GetFocus().y;
        const float cameraY = currentFocusY + (playerPos.y - currentFocusY) * verticalBlend;

        const float cameraX = playerPos.x + currentLookAheadX;
        DirectX::XMFLOAT3 focus = {cameraX, cameraY, playerPos.z};
        DirectX::XMFLOAT3 eye = {cameraX, cameraY + verticalEyeOffset, playerPos.z - 10.0f};

        const float travelX = playerPos.x - previousFollowPosition.x;
        const float travelY = playerPos.y - previousFollowPosition.y;
        const bool resetSection =
            !followInitialized || safeElapsedTime >= 1.0f || travelX * travelX + travelY * travelY > 400.0f;
        const bool sectionChanged =
            cameraSectionState.Update(cameraLimitZones, playerPos.x, playerPos.y, resetSection);
        cameraBoundsRegion.SetBounds(cameraSectionState.GetBounds(cameraLimitZones));
        const float originalX = focus.x;
        const float originalY = focus.y;

        if (sectionChanged || resetSection)
        {
            focus.y = playerPos.y;
        }

        if (!cameraSectionState.GetSection().empty())
        {
            cameraBoundsRegion.ConstrainSectionFollow(focus.x, focus.y, camera.GetFocus().x,
                                                      camera.GetFocus().y, playerPos.x, playerPos.y,
                                                      sectionChanged || resetSection, safeElapsedTime);
        }
        else
        {
            cameraBoundsRegion.ConstrainKeepingPlayerVisible(focus.x, focus.y, playerPos.x, playerPos.y);
        }

        followInitialized = true;
        previousFollowPosition = playerPos;
        eye.x += focus.x - originalX;
        eye.y += focus.y - originalY;
        camera.SetLookAt(eye, focus, DirectX::XMFLOAT3(0, 1, 0));
    }
}

// ImGuiメニューの描画
void CameraController::DrawMenuBar()
{
#ifdef _DEBUG
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

#endif
}

// UpdateFixed2D の更新処理
void CameraController::UpdateFixed2D(float elapsedTime, const DirectX::XMFLOAT3& fixedFocus)
{
    Camera& camera = Camera::Instance();

    // 注視点の正面から一定距離だけ後方にカメラを配置する
    const DirectX::XMFLOAT3 desiredEye = {
        fixedFocus.x,
        fixedFocus.y,
        fixedFocus.z - fixed2dDistance,
    };

    // 負の経過時間が補間計算に入らないようにする
    const float safeElapsedTime = elapsedTime > 0.0f ? elapsedTime : 0.0f;

    // フレームレートに左右されにくい補間率を計算する
    const float blend = 1.0f - std::exp(-interpolationSpeed * safeElapsedTime);

    const DirectX::XMVECTOR currentEye = DirectX::XMLoadFloat3(&camera.GetEye());

    const DirectX::XMVECTOR currentFocus = DirectX::XMLoadFloat3(&camera.GetFocus());

    const DirectX::XMVECTOR targetEye = DirectX::XMLoadFloat3(&desiredEye);

    const DirectX::XMVECTOR targetFocus = DirectX::XMLoadFloat3(&fixedFocus);

    DirectX::XMFLOAT3 interpolatedEye;
    DirectX::XMFLOAT3 interpolatedFocus;

    DirectX::XMStoreFloat3(&interpolatedEye, DirectX::XMVectorLerp(currentEye, targetEye, blend));

    DirectX::XMStoreFloat3(&interpolatedFocus, DirectX::XMVectorLerp(currentFocus, targetFocus, blend));

    const DirectX::XMFLOAT3 worldUp = {0.0f, 1.0f, 0.0f};

    camera.SetLookAt(interpolatedEye, interpolatedFocus, worldUp);
}
