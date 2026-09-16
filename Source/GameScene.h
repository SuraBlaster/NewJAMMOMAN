#pragma once

#include <memory>
#include <DirectXMath.h>
#include <optional>
#include "Scene.h"
#include "Player.h"
#include "Stage.h"
#include "CameraController.h"
#include "FileTypeManager.h"
#include "StageData.h"

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

private:
	// 1つの落下復帰エリアとXY座標を比較し、範囲内にいるかを返す。
	bool IsInsideFallRespawnZone(const DirectX::XMFLOAT3& position, const FallRespawnZoneData& zone) const;

	bool IsInsideBossEncounterZone(const DirectX::XMFLOAT3& position,const BossEncounterZoneData& zone) const;

	// プレイヤーのXY座標がボス前通路のカメラエリア内にあるか判定する。
	bool IsInsideBossApproachCameraZone(const DirectX::XMFLOAT3& position, const BossApproachCameraZoneData& zone) const;

	// プレイヤーが入っているボス前通路のカメラエリアを返す。
	// どのエリアにも入っていない場合はnullptrを返す。
	const BossApproachCameraZoneData* FindBossApproachCameraZone(const DirectX::XMFLOAT3& position) const;

	// プレイヤーが入っているボス戦エリアを返す。
	// どのエリアにも入っていない場合はnullptrを返す。
	const BossEncounterZoneData* FindBossEncounterZone(const DirectX::XMFLOAT3& position) const;

	// 空中のプレイヤーが落下復帰エリアへ入った場合、最後の安全位置へ戻す。
	void HandleFallRespawn();

	void TryStartBossEncounter();

private:
	TileTypeManager tileTypeManager;
	StageData stageData;
	std::optional<EnemySpawnData> pendingBossSpawnData;
	std::unique_ptr<Player> player;
	std::unique_ptr<Stage> stage;
	CameraController cameraController;
	bool isPaused = false;

private:
	// プレイヤーが最後に接地していた位置。落下時の復帰先として使用する。
	DirectX::XMFLOAT3 lastSafePosition = {};
};
