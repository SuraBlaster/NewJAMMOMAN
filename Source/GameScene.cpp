#include <imgui.h>
#include <iostream>
#include <stdexcept>
#include <StageLoader.h>
#include <cassert>
#include <cmath>
#include <algorithm>
#include <array>
#include "GameScene.h"
#include "EnemyFactory.h"
#include "Graphics.h"
#include "Camera.h"
#include "Collision.h"
#include "EnemyManager.h"
#include "EnemyBoss.h"
#include "HitStopManager.h"
#include "Audio/Audio.h"
#include "LoadingProfile.h"
#include "Light.h"
#include "ViewVolume.h"
#include "ModelManager.h"


GameScene::GameScene()
{
    LoadingProfile::Scope profile("GameScene");
	ID3D11Device* device = Graphics::Instance().GetDevice();
	backgroundEditor = std::make_unique<BackgroundEditor>(device);
    backgroundFog = std::make_unique<BackgroundFog>(device);
    backgroundCity = std::make_unique<BackgroundCity>(device);
	healthGaugeUI = std::make_unique<HealthGaugeUI>(device);

	// JSON読み込み
	if (!tileTypeManager.LoadDefinitions("Data/Stage/TileTypes.json"))
	{
		std::cerr << "エラー: タイル定義の読み込みに失敗しました。" << std::endl;
		throw std::runtime_error("Failed to load TileTypes.json.");
	}

	if (!StageLoader::Load(
		"Data/Stage/LaboratoryCourse.stage.json",
		tileTypeManager,
		stageData))
	{
		std::cerr
			<< "エラー: ステージデータの読み込みに失敗しました。"
			<< std::endl;

		throw std::runtime_error(
			"Failed to load LaboratoryCourse.stage.json.");
	}

	// 3Dオブジェクト
	EnemyBoss::SetDefeat(false);
	profile.Step("stage_json_and_camera");
	player = std::make_unique<Player>(device);
    profile.Step("player");
	// 最初の接地記録より前に落下しても戻れるよう、生成位置を初期復帰位置にする。
	lastSafePosition = player->GetPosition();
	stage = std::make_unique<Stage>(
		device,
		stageData,
		tileTypeManager);
    profile.Step("stage");

	

	// Tiledから変換された配置を順番に生成し、EnemyManagerへ所有権を渡す。
	for (const EnemySpawnData& spawnData : stageData.enemies)
	{
		if (spawnData.enemyType == EnemyType::Boss)
		{
			// 1つのステージに複数のボスが設定されていた場合は、設定ミスとして扱う。
			if (pendingBossSpawnData.has_value())
			{
				throw std::runtime_error(
					"Multiple boss spawn data entries were found.");
			}

			pendingBossSpawnData = spawnData;
			continue;
		}

		auto spawnedEnemy = EnemyFactory::CreateFromSpawnData(
			device,
			player.get(),
			spawnData);

		// Loader通過後に生成できない場合は実装側の対応漏れとして扱う。
		if (!spawnedEnemy)
		{
			throw std::runtime_error("Failed to create enemy from stage data.");
		}
		else
		{
			// Register後はEnemyManagerがshared_ptrを保持し、更新・描画・破棄を管理する。
			EnemyManager::Instance().Register(spawnedEnemy);
		}
	}

	profile.Step("enemies");
    CollisionManager& collisionManager =
		CollisionManager::Instance();

	collisionManager.ClearTerrainAABBs();

	for (const AABB& box : stage->GetTerrainAABBs())
	{
		collisionManager.RegisterTerrainAABB(box);
	}
    bossGates = BossGate::FromObjects(stageData.objects);
    for (const auto& gate : bossGates)
    {
        bossGateModels.push_back(ModelManager::Instance().CreateInstance(
            device, "Data/Model/Cube/Cube.gltf"));
    }
    UpdateBossGates(0.0f);
    profile.Step("collision_registration");

}

void GameScene::Initialize()
{
    DirectionalLight stageLight;
    LightManager::Instance().SetDirectionalLight(stageLight);
    const float screenWidth = Graphics::Instance().GetScreenWidth();
    const float screenHeight = Graphics::Instance().GetScreenHeight();
	Camera& camera = Camera::Instance();
	camera.SetPerspectiveFov(
		DirectX::XMConvertToRadians(45),	// 画角
		screenWidth / screenHeight,			// 画面アスペクト比
		0.1f,								// ニアクリップ
		1000.0f								// ファークリップ
	);
	camera.SetLookAtWithShake(
		{ 8, 3, 0 },		// 視点
		{ 0, 2, 0 },		// 注視点
		{ 0, 1, 0 }			// 上ベクトル
	);

    LoadingProfile::gameReady = true;

	Audio::Instance().Play("BGM_STAGE");
}

GameScene::~GameScene()
{
	// EnemyWave は Player を非所有ポインタで参照するため、Player の破棄前に解放する。
	EnemyManager::Instance().Clear();

	CollisionManager::Instance().ClearTerrainAABBs();
}

// 更新処理
void GameScene::Update(float elapsedTime)
{
    backgroundFog->Update(isPaused ? 0.0f : elapsedTime);
    backgroundCity->Update(isPaused ? 0.0f : elapsedTime);
    if (backgroundEditor->Update(elapsedTime)) return;
	HitStopManager::Instance().Update(elapsedTime);

	float scaleElapsedTime = elapsedTime * HitStopManager::Instance().GetTimeScale();

	if (!isPaused)
	{
		UpdateBossGates(scaleElapsedTime);
        player->Update(scaleElapsedTime);
        // Detect a complete crossing after movement, including a dash this frame.
        UpdateBossGates(0.0f);

		// 接地中の位置だけを記録し、空中では最後の安全な足場位置を保持する。
		if (player->IsGround())
		{
			lastSafePosition = player->GetPosition();
		}

		// 移動と接地判定の更新後に、Tiledで指定した落下範囲との重なりを調べる。
		if (!player->IsClearing() && !player->IsDead()) HandleFallRespawn();

		// プレイヤーがボス戦開始エリアへ入った場合、保留中のボスを生成する。
		if (!player->IsClearing() && !player->IsDead()) TryStartBossEncounter();

		if (!player->IsDead()) EnemyManager::Instance().Update(scaleElapsedTime);

		healthGaugeUI->Update(*player, elapsedTime);
	}

	if (!isPaused)
	{
		Camera::Instance().ShakeUpdate(scaleElapsedTime);
	}

	UpdateCamera(elapsedTime);
}

void GameScene::Render(float elapsedTime)
{
    Graphics::Instance().Clear(0.055f, 0.075f, 0.10f, 1.0f);
    if (IsBackgroundEditing())
    {
        auto& graphics = Graphics::Instance();
        graphics.Clear(0.06f, 0.08f, 0.10f, 1.0f);
        backgroundCity->Render();
        RenderContext rc;
        rc.deviceContext = graphics.GetDeviceContext();
        rc.renderState = graphics.GetRenderState();
        rc.camera = &Camera::Instance();
        rc.lightManager = &LightManager::Instance();
        rc.deviceContext->OMSetBlendState(rc.renderState->GetBlendState(BlendState::Opaque), nullptr, 0xFFFFFFFF);
        backgroundEditor->Render(graphics.GetModelRenderer());
        graphics.GetModelRenderer()->Render(rc);
        backgroundFog->Render();
        return;
    }
    backgroundCity->Render();
	healthGaugeUI->EnqueueRender();
	ID3D11DeviceContext* dc = Graphics::Instance().GetDeviceContext();
	RenderState* renderState = Graphics::Instance().GetRenderState();
	PrimitiveRenderer* primitiveRenderer = Graphics::Instance().GetPrimitiveRenderer();
	ShapeRenderer* shapeRenderer = Graphics::Instance().GetShapeRenderer();
	ModelRenderer* modelRenderer = Graphics::Instance().GetModelRenderer();

	Camera& camera = Camera::Instance();
	LightManager& lightManager = LightManager::Instance();
    const ViewVolume worldFrustum(camera.GetView(), camera.GetProjection());
    const auto terrainVisible = [&](const AABB& box) {
        return worldFrustum.Intersects(DirectX::BoundingBox(box.GetCenter(), box.GetHalfSize()));
    };

	// レンダーステート設定
	dc->OMSetBlendState(renderState->GetBlendState(BlendState::Opaque), nullptr, 0xFFFFFFFF);
	dc->OMSetDepthStencilState(renderState->GetDepthStencilState(DepthState::TestAndWrite), 0);
	dc->RSSetState(renderState->GetRasterizerState(RasterizerState::SolidCullNone));

	// モデル描画をキューに積む
	for (const auto& stageObject : stage->GetStageObjects())
	{
        AABB bounds;
        if (stageObject->GetCollisionAABB(bounds) && !terrainVisible(bounds)) continue;
		modelRenderer->Draw(
			ShaderId::Model,
			stageObject->GetModel(),
			{ 1.0f, 1.0f, 1.0f, 1.0f },
			1.0f);
	}
	for (std::size_t i = 0; i < bossGates.size(); ++i)
    {
        const auto& gate = bossGates[i];
        const auto& bounds = gate.GetBounds();
        const float visibleHeight = (bounds.max.y - bounds.min.y) * (1.0f - gate.GetOpenness());
        if (visibleHeight <= 0.0f) continue;
        auto center = bounds.GetCenter();
        auto half = bounds.GetHalfSize();
        // Draw only the shutter below its original top edge.
        // The lower edge rises during opening; the part above the doorway disappears.
        half.y = visibleHeight * 0.5f;
        center.y = bounds.max.y - half.y;
        DirectX::XMFLOAT4X4 world;
        DirectX::XMStoreFloat4x4(&world,
            DirectX::XMMatrixScaling(half.x * 2.0f, half.y * 2.0f, half.z * 2.0f) *
            DirectX::XMMatrixTranslation(center.x, center.y, center.z));
        bossGateModels[i]->UpdateTransform(world);
        modelRenderer->Draw(ShaderId::Model, bossGateModels[i],
            { 0.25f, 0.55f, 0.65f, 1.0f }, 1.0f);
    }
    modelRenderer->Draw(ShaderId::Model, player->GetModel(), { 1,1,1,1 }, 1.0f);
	EnemyManager::Instance().Render(modelRenderer);

	player->RenderMotionTrail(modelRenderer);

	// ここで不透明モデル(ステージ・プレイヤー・残像)を実際に描画し切る
	RenderContext rc;
	rc.deviceContext = dc;
	rc.renderState = renderState;
	rc.camera = &camera;
	rc.lightManager = &lightManager;
	backgroundEditor->Render(modelRenderer);
	modelRenderer->Render(rc);
    backgroundFog->Render();

	// 実際に衝突判定へ登録した箱を表示する。
	for (const AABB& box : stage->GetTerrainAABBs())
	{
        if (!terrainVisible(box)) continue;
		shapeRenderer->DrawBox(
			box.GetCenter(),
			DirectX::XMFLOAT3{ 0.0f, 0.0f, 0.0f },
			box.GetHalfSize(),
			DirectX::XMFLOAT4{ 0.0f, 0.8f, 1.0f, 1.0f }
		);
	}

	for (const auto& stageObject : stage->GetStageObjects())
	{
		AABB box;

		if (!stageObject->GetCollisionAABB(box) || !terrainVisible(box))
		{
			continue;
		}

		shapeRenderer->DrawBox(
			box.GetCenter(),
			DirectX::XMFLOAT3{ 0.0f, 0.0f, 0.0f },
			box.GetHalfSize(),
			DirectX::XMFLOAT4{ 1.0f, 0.8f, 0.0f, 1.0f }
		);
	}
	// 実際の刀判定と同じ掃引カプセルをデバッグ表示する。
	player->DrawDebugPrimitive(shapeRenderer);
	
	EnemyManager::Instance().DrawPrimitive(shapeRenderer);
#ifdef _DEBUG
	EnemyManager::Instance().DrawDebugPrimitive(shapeRenderer);
#endif
	dc->OMSetBlendState(renderState->GetBlendState(BlendState::Opaque), nullptr, 0xFFFFFFFF);
	dc->OMSetDepthStencilState(renderState->GetDepthStencilState(DepthState::TestOnly), 0);
	dc->RSSetState(renderState->GetRasterizerState(RasterizerState::SolidCullNone));
	shapeRenderer->Render(dc, camera.GetView(), camera.GetProjection());

	// 剣の軌跡の頂点をバッファに積む
	player->RenderTrail(primitiveRenderer);

	// ソードトレイル用ステートを明示
	dc->OMSetBlendState(
		renderState->GetBlendState(BlendState::Additive),
		nullptr,
		0xFFFFFFFF
	);
	dc->OMSetDepthStencilState(
		renderState->GetDepthStencilState(DepthState::TestOnly),
		0
	);

	// modelRendererによるステート変更を上書きする
	dc->RSSetState(
		renderState->GetRasterizerState(RasterizerState::SolidCullNone)
	);

	primitiveRenderer->Render(
		dc,
		camera.GetView(),
		camera.GetProjection(),
		D3D11_PRIMITIVE_TOPOLOGY_TRIANGLESTRIP
	);

	// ステートを戻す（次のフレーム/後続処理のため）
	dc->OMSetBlendState(renderState->GetBlendState(BlendState::Opaque), nullptr, 0xFFFFFFFF);
	dc->OMSetDepthStencilState(renderState->GetDepthStencilState(DepthState::TestAndWrite), 0);
}

// GUI描画処理
void GameScene::DrawGUI()
{
    backgroundFog->DrawGUI();
    backgroundCity->DrawGUI();
    if (backgroundEditor->IsEditing()) return;
	if (ImGui::Begin("Debug"))
	{
		ImGui::Checkbox(u8"ゲーム時間を停止", &isPaused);
		ImGui::Text("Enemy count: %zu", EnemyManager::Instance().GetEnemyCount());
	}
	ImGui::End();

	player->DrawGUI();

	cameraController.DrawMenuBar();
}

// XY座標が長方形の4境界すべてに収まっている場合だけ、範囲内と判定する。
// 2.5Dの共通レーンを使うため、Z座標は比較しない。
bool GameScene::IsInsideFallRespawnZone(const DirectX::XMFLOAT3& position, const FallRespawnZoneData& zone) const
{
	return position.x >= zone.minX &&
		position.x <= zone.maxX &&
		position.y >= zone.minY &&
		position.y <= zone.maxY;
}

// プレイヤーがボス前通路のカメラエリア内にいるか判定する。
// 2.5DゲームなのでZ座標は判定に使用しない。
bool GameScene::IsInsideBossApproachCameraZone(const DirectX::XMFLOAT3& position, const BossApproachCameraZoneData& zone) const
{
	return position.x >= zone.minX &&
		position.x <= zone.maxX &&
		position.y >= zone.minY &&
		position.y <= zone.maxY;
}

bool GameScene::IsInsideCameraLimitZone(const DirectX::XMFLOAT3& position, const CameraLimitZoneData& zone) const
{
	return position.x >= zone.minX &&
		position.x <= zone.maxX &&
		position.y >= zone.minY &&
		position.y <= zone.maxY;
}

bool GameScene::IsInsideBossEncounterZone(const DirectX::XMFLOAT3& position, const BossEncounterZoneData& zone) const
{
	return position.x >= zone.minX &&
		position.x <= zone.maxX &&
		position.y >= zone.minY &&
		position.y <= zone.maxY;
}

const BossApproachCameraZoneData* GameScene::FindBossApproachCameraZone(const DirectX::XMFLOAT3& position) const
{
	for (const BossApproachCameraZoneData& zone : stageData.bossApproachCameraZones)
	{
		if (IsInsideBossApproachCameraZone(position, zone))
		{
			return &zone;
		}
	}

	return nullptr;
}

const BossEncounterZoneData* GameScene::FindBossEncounterZone(const DirectX::XMFLOAT3& position) const
{
	for (const BossEncounterZoneData& zone :
		stageData.bossEncounterZones)
	{
		if (IsInsideBossEncounterZone(position, zone))
		{
			return &zone;
		}
	}

	return nullptr;
}

const CameraLimitZoneData* GameScene::FindCameraLimitZone(const DirectX::XMFLOAT3& position) const
{
	for (const CameraLimitZoneData& zone : stageData.cameraLimitZones)
	{
		if (IsInsideCameraLimitZone(position, zone))
		{
			return &zone;
		}
	}

	return nullptr;
}

// ステージ内の落下復帰エリアを検索し、該当した最初の1件で復帰処理を行う。
void GameScene::HandleFallRespawn()
{
	// 地面に立っている場合は落下中ではないため、復帰させない。
	if (player->IsGround())
	{
		return;
	}

	// ループ中に同じ現在位置を繰り返し取得しないよう、定数参照として保持する。
	const DirectX::XMFLOAT3& playerPosition = player->GetPosition();

	// Tiledから読み込んだ各長方形と、プレイヤーの足元座標を順番に比較する。
	for (const FallRespawnZoneData& zone : stageData.fallRespawnZones)
	{
		if (!IsInsideFallRespawnZone(playerPosition, zone))
		{
			continue;
		}

		// 最後に接地していた安全な位置へ戻す。
		player->SetPosition(lastSafePosition);

		// 落下速度や横方向の慣性を持ち越さない。
		player->SetVelocity({ 0.0f, 0.0f, 0.0f });

		// 同時に複数の範囲を処理する必要はないため、復帰後は直ちに終了する。
		return;
	}
}

void GameScene::TryStartBossEncounter()
{
	// 保留中のボスがいなければ、未配置または既に生成済みなので何もしない。
	if (!pendingBossSpawnData.has_value())
	{
		return;
	}

	const DirectX::XMFLOAT3& playerPosition = player->GetPosition();

	for (const BossEncounterZoneData& zone : stageData.bossEncounterZones)
	{
		if (!IsInsideBossEncounterZone(playerPosition, zone) ||
            player->GetBodyAABB().min.x < zone.minX + 0.05f ||
            player->GetBodyAABB().max.x > zone.maxX - 0.05f)
		{
			continue;
		}

		auto spawnedBoss = EnemyFactory::CreateFromSpawnData(
			Graphics::Instance().GetDevice(),
			player.get(),
			pendingBossSpawnData.value());

		if (!spawnedBoss)
		{
			throw std::runtime_error(
				"Failed to create boss from stage data.");
		}

		// Share the exact encounter bounds used by the fixed boss camera.
		const auto boss = std::dynamic_pointer_cast<EnemyBoss>(spawnedBoss);
		if (!boss)
		{
			throw std::runtime_error("Boss encounter spawned a non-boss enemy.");
		}
		boss->SetArenaBounds(zone.minX, zone.maxX);
		healthGaugeUI->ShowBoss(boss);

		EnemyManager::Instance().Register(spawnedBoss);

		// 配置情報を空にして、次のフレーム以降に同じボスが生成されるのを防ぐ。
		activeBossArena = zone;
        lastSafePosition = playerPosition;
        pendingBossSpawnData.reset();
		return;
	}
}



void GameScene::UpdateBossGates(float elapsedTime)
{
    if (bossGates.empty()) return;
    auto& collisionManager = CollisionManager::Instance();
    collisionManager.ClearTerrainAABBs();
    for (const auto& box : stage->GetTerrainAABBs())
        collisionManager.RegisterTerrainAABB(box);
    const auto body = player->GetBodyAABB();
    for (auto& gate : bossGates)
    {
        const bool wasLocked = gate.IsLocked();
        gate.Update(elapsedTime, body);
        // A fall must never restore the player to the entry side of a sealed gate.
        if (!wasLocked && gate.IsLocked()) lastSafePosition = player->GetPosition();
        if (gate.IsSolid()) collisionManager.RegisterTerrainAABB(gate.GetBounds());
    }
}

void GameScene::UpdateCamera(float elapsedTime)
{
	ImGuiIO& io = ImGui::GetIO();
	if (!io.WantCaptureMouse && io.MouseWheel != 0.0f)
	{
		cameraController.Zoom(io.MouseWheel);
	}

	const DirectX::XMFLOAT3& playerPosition =
		player->GetPosition();

	const BossEncounterZoneData* bossEncounterZone =
		activeBossArena ? &activeBossArena.value() : FindBossEncounterZone(playerPosition);

	const BossApproachCameraZoneData* approachCameraZone =
		FindBossApproachCameraZone(playerPosition);

	const CameraLimitZoneData* cameraLimitZone =
		FindCameraLimitZone(playerPosition);

	// ボス部屋のカメラを最優先する。
	if (bossEncounterZone != nullptr)
	{
		const DirectX::XMFLOAT3 fixedFocus =
		{
			(bossEncounterZone->minX + bossEncounterZone->maxX) * 0.5f,
			(bossEncounterZone->minY + bossEncounterZone->maxY) * 0.5f,
			playerPosition.z
		};

		cameraController.UpdateFixed2D(
			elapsedTime,
			fixedFocus);
	}
	// ボス部屋の外で通路エリア内なら、通路用カメラを使用する。
	else if (approachCameraZone != nullptr)
	{
		const DirectX::XMFLOAT3 fixedFocus =
		{
			(approachCameraZone->minX + approachCameraZone->maxX) * 0.5f,
			(approachCameraZone->minY + approachCameraZone->maxY) * 0.5f,
			playerPosition.z
		};

		cameraController.UpdateFixed2D(
			elapsedTime,
			fixedFocus);
	}
	// どちらの専用エリアにもいなければ、通常カメラを使用する。
	else
	{
		cameraController.SetTarget(playerPosition);

		cameraController.Update(
			elapsedTime,
			playerPosition,
			player->GetVelocity(),
			cameraLimitZone);
	}
}
