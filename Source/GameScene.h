#pragma once

#include <memory>
#include <DirectXMath.h>
#include <optional>
#include "Scene.h"
#include "Sprite.h"
#include "HealthGaugeTrail.h"
class EnemyBoss;
#include "Player.h"
#include "Stage.h"
#include "CameraController.h"
#include "TileTypeManager.h"
#include "StageData.h"
#include "BackgroundEditor.h"
#include "BackgroundFog.h"
#include "BackgroundCity.h"

class GameScene : public Scene
{
public:
	GameScene();
	~GameScene() override;

	// 更新処理
	void Update(float elapsedTime) override;

	// 描画処理
	void Render(float elapsedTime) override;

	// GUI描画処理
	void DrawGUI() override;
    bool IsBackgroundEditing() const override { return backgroundEditor && backgroundEditor->IsEditing(); }

private:
	// 1つの落下復帰エリアとXY座標を比較し、範囲内にいるかを返す。
	bool IsInsideFallRespawnZone(const DirectX::XMFLOAT3& position, const FallRespawnZoneData& zone) const;

	bool IsInsideBossEncounterZone(const DirectX::XMFLOAT3& position,const BossEncounterZoneData& zone) const;

	// プレイヤーのXY座標がボス前通路のカメラエリア内にあるか判定する。
	bool IsInsideBossApproachCameraZone(const DirectX::XMFLOAT3& position, const BossApproachCameraZoneData& zone) const;

	bool IsInsideCameraLimitZone(const DirectX::XMFLOAT3& position, const CameraLimitZoneData& zone) const;

	// プレイヤーが入っているボス前通路のカメラエリアを返す。
	// どのエリアにも入っていない場合はnullptrを返す。
	const BossApproachCameraZoneData* FindBossApproachCameraZone(const DirectX::XMFLOAT3& position) const;

	// プレイヤーが入っているボス戦エリアを返す。
	// どのエリアにも入っていない場合はnullptrを返す。
	const BossEncounterZoneData* FindBossEncounterZone(const DirectX::XMFLOAT3& position) const;

	// プレイヤーが入っているエリアを返す。
	const CameraLimitZoneData* FindCameraLimitZone(const DirectX::XMFLOAT3& position) const;

	// 空中のプレイヤーが落下復帰エリアへ入った場合、最後の安全位置へ戻す。
	void HandleFallRespawn();

	void TryStartBossEncounter();
	void RenderHealthGauges();

private:
	TileTypeManager tileTypeManager;
	StageData stageData;
	std::optional<EnemySpawnData> pendingBossSpawnData;
	std::unique_ptr<Player> player;
	std::unique_ptr<Stage> stage;
	std::unique_ptr<BackgroundEditor> backgroundEditor;
    std::unique_ptr<BackgroundFog> backgroundFog;
    std::unique_ptr<BackgroundCity> backgroundCity;
	CameraController cameraController;
	bool isPaused = false;
	std::unique_ptr<Sprite> playerGauge;
	std::unique_ptr<Sprite> bossGauge;
	Microsoft::WRL::ComPtr<ID3D11Buffer> gaugeConstants;
	std::weak_ptr<EnemyBoss> encounterBoss;
	HealthGaugeTrail playerHealthTrail;
	HealthGaugeTrail bossHealthTrail;
	bool bossGaugeVisible = false;
	float bossGaugeIntroTime = 0.0f;
	static constexpr float bossGaugeIntroDuration = 1.5f;

	std::array<AABB, 3> testTerrainBoxes = {
		// 床：上面がY=0
		AABB{
			{ -6.0f, -1.0f, -2.0f },
			{  6.0f,  0.0f,  2.0f }
		},

		// 右の壁：左面がX=3
		AABB{
			{ 3.0f, 0.0f, -2.0f },
			{ 4.0f, 4.0f,  2.0f }
		},

		// 天井：下面がY=2.4
		AABB{
			{ -3.0f, 2.4f, -2.0f },
			{  3.0f, 3.4f,  2.0f }
		}
	};

private:
	// プレイヤーが最後に接地していた位置。落下時の復帰先として使用する。
	DirectX::XMFLOAT3 lastSafePosition = {};
};
