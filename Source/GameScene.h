#pragma once

#include <DirectXMath.h>
#include <memory>
#include <optional>
#include <vector>
#include <array>
#include "Scene.h"
#include "CameraController.h"
#include "Player.h"
#include "BossGate.h"
#include "Stage.h"
#include "StageData.h"
#include "TileTypeManager.h"
#include "BackgroundEditor.h"
#include "BackgroundCity.h"
#include "BackgroundFog.h"
#include "HealthGaugeUI.h"

class GameScene : public Scene
{
public:
    // コンストラクタ
    explicit GameScene(std::optional<DirectX::XMFLOAT3> savePosition = std::nullopt);

    // デストラクタ
    ~GameScene() override;

    // 初期化処理
    void Initialize() override;

    // 更新処理
    void Update(float elapsedTime) override;

    // 描画処理
    void Render(float elapsedTime) override;

    // GUI描画処理
    void DrawGUI() override;

    bool UsesEscapeKey() const override
    {
        return true;
    }

    bool IsGameplayPaused() const override
    {
        return instructionVisible || isPaused;
    }

    bool IsBackgroundEditing() const override
    {
        return backgroundEditor && backgroundEditor->IsEditing();
    }

private:
    // 1つの落下復帰エリアとXY座標を比較し、範囲内にいるかを返す
    bool IsInsideFallRespawnZone(const DirectX::XMFLOAT3& position, const FallRespawnZoneData& zone) const;

    bool IsInsideBossEncounterZone(const DirectX::XMFLOAT3& position,
                                   const BossEncounterZoneData& zone) const;

    // プレイヤーのXY座標がボス前通路のカメラエリア内にあるか判定する
    bool IsInsideBossApproachCameraZone(const DirectX::XMFLOAT3& position,
                                        const BossApproachCameraZoneData& zone) const;

    bool IsInsideCameraLimitZone(const DirectX::XMFLOAT3& position, const CameraLimitZoneData& zone) const;

    // プレイヤーが入っているボス前通路のカメラエリアを返す
    const BossApproachCameraZoneData* FindBossApproachCameraZone(const DirectX::XMFLOAT3& position) const;

    // プレイヤーが入っているボス戦エリアを返す
    const BossEncounterZoneData* FindBossEncounterZone(const DirectX::XMFLOAT3& position) const;

    // プレイヤーが入っているエリアを返す
    const CameraLimitZoneData* FindCameraLimitZone(const DirectX::XMFLOAT3& position) const;

    // 空中のプレイヤーが落下復帰エリアへ入った場合、最後の安全位置へ戻す
    void HandleFallRespawn();

    void MakeSafetyMargin();

    void EnqueueInstructions();
    void RenderInstructions();
    std::unique_ptr<Sprite> instructionSprite;
    std::unique_ptr<Sprite> instructionGuideSprite;
    bool instructionVisible = false;
    float instructionGuideAlpha = 1.0f;

    void EnqueueReady();
    void RenderReady();
    static constexpr float READY_DURATION = 2.2f;
    std::unique_ptr<Sprite> readySprite;
    bool readyPlaying = false;
    float readyTime = 0.0f;

    void TryStartBossEncounter();
    void UpdateBossWarning(float elapsedTime);
    void EnqueueBossWarning();
    void RenderBossWarning();
    static constexpr float WARNING_DURATION = 3.2f;
    std::array<std::unique_ptr<Sprite>, 7> warningLetters;
    bool warningPlaying = false;
    float warningTime = 0.0f;

    // UpdateBossGates の更新処理
    void UpdateBossGates(float elapsedTime);
#ifdef _DEBUG
    void DebugWarpBeforeBoss();
    void DebugWarpToLateStage(float targetX, float targetFloorY);
    bool debugWarpFailed = false;
#endif

    // カメラの更新処理
    void UpdateCamera(float elapsedTime);

private:
    TileTypeManager tileTypeManager;
    StageData stageData;
    std::optional<EnemySpawnData> pendingBossSpawnData;
    std::optional<BossEncounterZoneData> activeBossArena;
    std::weak_ptr<EnemyBoss> introBoss;
    std::vector<BossGate> bossGates;
    std::vector<std::shared_ptr<Model>> bossGateModels;
    std::unique_ptr<Player> player;
    std::unique_ptr<Stage> stage;
    std::unique_ptr<BackgroundEditor> backgroundEditor;
    std::unique_ptr<BackgroundFog> backgroundFog;
    std::unique_ptr<BackgroundCity> backgroundCity;
    CameraController cameraController;
    // 0: hidden, 1: before merging, 2: after merging, 3: both
    int terrainAabbDisplay = 3;
    bool isPaused = false;
    std::unique_ptr<HealthGaugeUI> healthGaugeUI;

private:
    // プレイヤーが最後に接地していた位置。落下時の復帰先として使用する
    DirectX::XMFLOAT3 lastSafePosition = {};
};
