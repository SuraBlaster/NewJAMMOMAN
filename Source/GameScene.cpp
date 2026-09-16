#include <imgui.h>
#include "Graphics.h"
#include "Camera.h"
#include "Light.h"
#include "GameScene.h"
#include <iostream>
#include <stdexcept>
#include <StageLoader.h>
#include "EnemyManager.h"
#include "EnemyBoss.h"
#include "EnemyEgg.h"
#include "EnemyFan.h"
#include "EnemyFly.h"
#include "EnemyHover.h"
#include "EnemyMage.h"
#include "EnemyScatter.h"
#include "EnemyWave.h"

namespace
{
	// 読み込み済みの配置データから対応する敵クラスを生成する。Managerへの登録は呼び出し側で行う。
	std::shared_ptr<Enemy> CreateEnemyFromSpawnData(ID3D11Device* device, Player* player, const EnemySpawnData& spawnData)
	{
		// 2.5Dの奥行きレーンを揃えるため、Z座標はプレイヤーの現在位置を使用する。
		DirectX::XMFLOAT3 spawnPosition = { spawnData.positionX,spawnData.positionY,player->GetPosition().z };

		// 共通のEnemyポインタとして返し、呼び出し側の登録処理を敵種別から独立させる。
		switch (spawnData.enemyType)
		{
		case EnemyType::Wave:
		{
			auto enemyWave = std::make_shared<EnemyWave>(device, player);
			enemyWave->SetPosition(spawnPosition);
			// StageData側の方向をEnemyWave固有の方向型へ変換する。
			if (spawnData.enemySpawnDirection == EnemySpawnDirection::Right)
			{
				enemyWave->SetDirection(EnemyWave::Direction::Right);
			}
			else
			{
				enemyWave->SetDirection(EnemyWave::Direction::Left);
			}
			return enemyWave;
		}
		case EnemyType::Scatter:
		{
			auto enemyScatter = std::make_shared<EnemyScatter>(device, player,spawnPosition);
			return enemyScatter;
		}
		case EnemyType::Egg:
		{
			auto enemyEgg = std::make_shared<EnemyEgg>(device, player, spawnPosition);
			return enemyEgg;
		}
		case EnemyType::Mage:
		{
			auto enemyMage = std::make_shared<EnemyMage>(device, player);
			enemyMage->SetPosition(spawnPosition);
			return enemyMage;
		}
		case EnemyType::Fan:
		{
			auto enemyFan = std::make_shared<EnemyFan>(device, player);
			enemyFan->SetPosition(spawnPosition);
			return enemyFan;
		}
		case EnemyType::Hover:
		{
			auto enemyHover = std::make_shared<EnemyHover>(device, player);
			enemyHover->SetPosition(spawnPosition);
			return enemyHover;
		}
		case EnemyType::Fly:
		{
			auto enemyFly = std::make_shared<EnemyFly>(device, player);
			enemyFly->SetPosition(spawnPosition);
			return enemyFly;
		}
		case EnemyType::Boss:
		{
			auto enemyBoss = std::make_shared<EnemyBoss>(device, player);
			enemyBoss->SetPosition(spawnPosition);
			return enemyBoss;
		}
		case EnemyType::Unknown:
		{
		}
		default:
		{
		}
		}

		return nullptr;
	}
}

// コンストラクタ
GameScene::GameScene()
{
	ID3D11Device* device = Graphics::Instance().GetDevice();
	float screenWidth = Graphics::Instance().GetScreenWidth();
	float screenHeight = Graphics::Instance().GetScreenHeight();

	// JSON読み込み
	if (!tileTypeManager.LoadDefinitions("Data/Stage/TileTypes.json"))
	{
		std::cerr << "エラー: タイル定義の読み込みに失敗しました。" << std::endl;
		throw std::runtime_error("Failed to load TileTypes.json.");
	}

	if (!StageLoader::Load(
		"Data/Stage/ConvertedStage.stage.json",
		tileTypeManager,
		stageData))
	{
		std::cerr
			<< "エラー: ステージデータの読み込みに失敗しました。"
			<< std::endl;

		throw std::runtime_error(
			"Failed to load ConvertedStage.stage.json.");
	}

	// カメラ設定
	Camera& camera = Camera::Instance();
	camera.SetPerspectiveFov(
		DirectX::XMConvertToRadians(45),	// 画角
		screenWidth / screenHeight,			// 画面アスペクト比
		0.1f,								// ニアクリップ
		1000.0f								// ファークリップ
	);
	camera.SetLookAt(
		{ 8, 3, 0 },		// 視点
		{ 0, 2, 0 },		// 注視点
		{ 0, 1, 0 }			// 上ベクトル
	);

	// 3Dオブジェクト
	player = std::make_unique<Player>(device);
	// 最初の接地記録より前に落下しても戻れるよう、生成位置を初期復帰位置にする。
	lastSafePosition = player->GetPosition();
	stage = std::make_unique<Stage>(
		device,
		stageData,
		tileTypeManager);

	

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

		auto spawnedEnemy = CreateEnemyFromSpawnData(
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
}

GameScene::~GameScene()
{
	// EnemyWave は Player を非所有ポインタで参照するため、Player の破棄前に解放する。
	EnemyManager::Instance().Clear();
}

// 更新処理
void GameScene::Update(float elapsedTime)
{
	if (!isPaused)
	{
		player->Update(elapsedTime);

		// 接地中の位置だけを記録し、空中では最後の安全な足場位置を保持する。
		if (player->IsGround())
		{
			lastSafePosition = player->GetPosition();
		}

		// 移動と接地判定の更新後に、Tiledで指定した落下範囲との重なりを調べる。
		HandleFallRespawn();

		// プレイヤーがボス戦開始エリアへ入った場合、保留中のボスを生成する。
		TryStartBossEncounter();

		EnemyManager::Instance().Update(elapsedTime);
	}

	ImGuiIO& io = ImGui::GetIO();
	if (!io.WantCaptureMouse && io.MouseWheel != 0.0f)
	{
		cameraController.Zoom(io.MouseWheel);
	}

	const DirectX::XMFLOAT3& playerPosition =
		player->GetPosition();

	const BossEncounterZoneData* bossEncounterZone =
		FindBossEncounterZone(playerPosition);

	const BossApproachCameraZoneData* approachCameraZone =
		FindBossApproachCameraZone(playerPosition);

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
			player->GetVelocity());
	}
}

void GameScene::Render(float elapsedTime)
{
	ID3D11DeviceContext* dc = Graphics::Instance().GetDeviceContext();
	RenderState* renderState = Graphics::Instance().GetRenderState();
	PrimitiveRenderer* primitiveRenderer = Graphics::Instance().GetPrimitiveRenderer();
	ShapeRenderer* shapeRenderer = Graphics::Instance().GetShapeRenderer();
	ModelRenderer* modelRenderer = Graphics::Instance().GetModelRenderer();

	Camera& camera = Camera::Instance();
	LightManager& lightManager = LightManager::Instance();

	// レンダーステート設定
	dc->OMSetBlendState(renderState->GetBlendState(BlendState::Opaque), nullptr, 0xFFFFFFFF);
	dc->OMSetDepthStencilState(renderState->GetDepthStencilState(DepthState::TestAndWrite), 0);
	dc->RSSetState(renderState->GetRasterizerState(RasterizerState::SolidCullNone));

	// モデル描画をキューに積む
	for (const auto& stageObject : stage->GetStageObjects())
	{
		modelRenderer->Draw(
			ShaderId::Model,
			stageObject->GetModel(),
			{ 0.5f, 0.5f, 0.5f, 1.0f },
			1.0f);
	}
	modelRenderer->Draw(ShaderId::Model, player->GetModel(), { 1,1,1,1 }, 1.0f);
	EnemyManager::Instance().Render(modelRenderer);

	const auto& trails = player->GetTrails();
	for (const auto& trail : trails)
	{
		if (trail.trailModel)
		{
			DirectX::XMMATRIX S = DirectX::XMMatrixScaling(trail.scale.x, trail.scale.y, trail.scale.z);
			DirectX::XMMATRIX R = DirectX::XMMatrixRotationRollPitchYaw(trail.angle.x, trail.angle.y, trail.angle.z);
			DirectX::XMMATRIX T = DirectX::XMMatrixTranslation(trail.position.x, trail.position.y, trail.position.z);
			DirectX::XMFLOAT4X4 world;
			DirectX::XMStoreFloat4x4(&world, S * R * T);

			trail.trailModel->UpdateTransform(world);
			modelRenderer->Draw(ShaderId::Model, trail.trailModel, trail.drawColor, trail.alpha);
		}
	}

	// ここで不透明モデル(ステージ・プレイヤー・残像)を実際に描画し切る
	RenderContext rc;
	rc.deviceContext = dc;
	rc.renderState = renderState;
	rc.camera = &camera;
	rc.lightManager = &lightManager;
	modelRenderer->Render(rc);

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

bool GameScene::IsInsideBossEncounterZone(const DirectX::XMFLOAT3& position, const BossEncounterZoneData& zone) const
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
		if (!IsInsideBossEncounterZone(playerPosition, zone))
		{
			continue;
		}

		auto spawnedBoss = CreateEnemyFromSpawnData(
			Graphics::Instance().GetDevice(),
			player.get(),
			pendingBossSpawnData.value());

		if (!spawnedBoss)
		{
			throw std::runtime_error(
				"Failed to create boss from stage data.");
		}

		EnemyManager::Instance().Register(spawnedBoss);

		// 配置情報を空にして、次のフレーム以降に同じボスが生成されるのを防ぐ。
		pendingBossSpawnData.reset();
		return;
	}
}
