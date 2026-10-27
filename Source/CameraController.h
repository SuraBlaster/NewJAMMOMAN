#pragma once

#include <DirectXMath.h>
#include <imgui.h>
#include "StageData.h"
#include "CameraBoundsRegion.h"
#include "CameraSectionState.h"

// カメラコントローラー
class CameraController
{
  public:

    // カメラモードの定義
    enum class CameraMode
    {
        Free,    // 自由視点カメラ
        Follow2D // 2D追従カメラ
    };

    // 更新処理
    void Update(float elapsedTime, const DirectX::XMFLOAT3& playerPos,
                const DirectX::XMFLOAT3& playerVelocity,
                const std::vector<CameraLimitZoneData>& cameraLimitZones = {});

    // メニューバーの描画
    void DrawMenuBar();

    // ターゲット位置設定
    void SetTarget(const DirectX::XMFLOAT3& target)
    {
        this->target = target;
    }

    // UpdateFixed2D の更新処理
    void UpdateFixed2D(float elapsedTime, const DirectX::XMFLOAT3& fixedFocus);

    void Zoom(float wheelDelta)
    {
        range -= wheelDelta * range * 0.1f;

        if (range < 2.0f)
        {
            range = 2.0f;
        }

        if (range > 30.0f)
        {
            range = 30.0f;
        }
    }

    // モードの取得と設定
    CameraMode GetMode() const
    {
        return mode;
    }

    void SetMode(CameraMode newMode)
    {
        mode = newMode;
    }

  private:

    CameraMode mode = CameraMode::Follow2D;

    DirectX::XMFLOAT3 target = {0, 0, 0};                              // 注視点
    DirectX::XMFLOAT3 angle = {DirectX::XMConvertToRadians(45), 0, 0}; // 回転角度
    float rollSpeed = DirectX::XMConvertToRadians(90);                 // 回転速度
    float range = 10.0f;                                               // 距離
    float maxAngleX = DirectX::XMConvertToRadians(45);
    float minAngleX = DirectX::XMConvertToRadians(-45);
    float interpolationSpeed = 3.0f; // 補間速度

    float lookAheadDistance = 2.0f;
    float lookAheadLerpSpeed = 3.0f;
    float lookAheadVelocityThreshold = 0.1f;
    float directionChangeConfirmTime = 0.2f;
    float lastMoveDirectionX = 0.0f;
    float pendingMoveDirectionX = 0.0f;
    float directionChangeTimer = 0.0f;
    float currentLookAheadX = 0.0f;

    float verticalFollowLerpSpeed = 3.0f;
    float verticalEyeOffset = 1.0f;

    // 固定2Dカメラの注視点からの距離
    float fixed2dDistance = 10.0f;

    // 補正値
    CameraBoundsRegion cameraBoundsRegion;
    CameraSectionState cameraSectionState;
    bool followInitialized = false;
    DirectX::XMFLOAT3 previousFollowPosition{};
};
