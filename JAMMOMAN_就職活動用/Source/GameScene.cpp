#include <memory>
#include <optional>
#include <cstddef>
#include <stdexcept>
#include <algorithm>
#include <cmath>
#include <iostream>
#include <imgui.h>
#include "GameScene.h"
#include "Graphics.h"
#include "Camera.h"
#include "Light.h"
#include "EnemyManager.h"
#include "EnemyFactory.h"
#include "EnemyBoss.h"
#include "EnemyWave.h"
#include "Collision.h"
#include "StageLoader.h"
#include "ModelManager.h"
#include "ViewVolume.h"
#include "Audio/Audio.h"
#include "HitStopManager.h"
#include "LoadingProfile.h"

// コンストラクタ
GameScene::GameScene(std::optional<DirectX::XMFLOAT3> savePosition)
{
    LoadingProfile::Scope profile("GameScene");
    ID3D11Device* device = Graphics::Instance().GetDevice();
    backgroundEditor = std::make_unique<BackgroundEditor>(device);
    backgroundFog = std::make_unique<BackgroundFog>(device);
    backgroundCity = std::make_unique<BackgroundCity>(device);
    healthGaugeUI = std::make_unique<HealthGaugeUI>(device);
    readySprite = std::make_unique<Sprite>(device, "Data/Sprite/Ready.png");
    readyPlaying = !savePosition.has_value();
    const char* warningPaths[] = {
        "Data/Sprite/WarningLetters/01_W.png", "Data/Sprite/WarningLetters/02_A.png",
        "Data/Sprite/WarningLetters/03_R.png", "Data/Sprite/WarningLetters/04_N.png",
        "Data/Sprite/WarningLetters/05_I.png", "Data/Sprite/WarningLetters/06_N.png",
        "Data/Sprite/WarningLetters/07_G.png"};
    for (std::size_t i = 0; i < warningLetters.size(); ++i)
    {
        warningLetters[i] = std::make_unique<Sprite>(device, warningPaths[i]);
    }

    // JSON読み込み
    if (!tileTypeManager.LoadDefinitions("Data/Stage/TileTypes.json"))
    {
        std::cerr << "エラー: タイル定義の読み込みに失敗しました。" << std::endl;
        throw std::runtime_error("Failed to load TileTypes.json.");
    }

    if (!StageLoader::Load("Data/Stage/LaboratoryCombinedCourse.stage.json", tileTypeManager, stageData))
    {
        std::cerr << "エラー: ステージデータの読み込みに失敗しました。" << std::endl;

        throw std::runtime_error("Failed to load Data/Stage/LaboratoryCombinedCourse.stage.json.");
    }

    // 3Dオブジェクト
    EnemyBoss::SetDefeat(false);
    profile.Step("stage_json_and_camera");
    player = std::make_unique<Player>(device);
    player->deathRespawnPosition = savePosition;
    if (savePosition)
    {
        player->SetPosition(*savePosition);
    }
    profile.Step("player");
    // 最初の接地記録より前に落下しても戻れるよう、生成位置を初期復帰位置にする
    lastSafePosition = player->GetPosition();
    stage = std::make_unique<Stage>(device, stageData, tileTypeManager);
    profile.Step("stage");

    // Tiledから変換された配置を順番に生成し、EnemyManagerへ所有権を渡す
    for (const EnemySpawnData& spawnData : stageData.enemies)
    {
        if (spawnData.enemyType == EnemyType::Boss)
        {
            // 1つのステージに複数のボスが設定されていた場合は、設定ミスとして扱う
            if (pendingBossSpawnData.has_value())
            {
                throw std::runtime_error("Multiple boss spawn data entries were found.");
            }

            pendingBossSpawnData = spawnData;
            continue;
        }

        auto spawnedEnemy = EnemyFactory::CreateFromSpawnData(device, player.get(), spawnData);

        // Loader通過後に生成できない場合は実装側の対応漏れとして扱う
        if (!spawnedEnemy)
        {
            throw std::runtime_error("Failed to create enemy from stage data.");
        }
        else
        {
            // Register後はEnemyManagerがshared_ptrを保持し、更新・描画・破棄を管理する
            EnemyManager::Instance().Register(spawnedEnemy);
        }
    }

    profile.Step("enemies");
    CollisionManager& collisionManager = CollisionManager::Instance();

    collisionManager.ClearTerrainAABBs();

    for (const AABB& box : stage->GetTerrainAABBs())
    {
        collisionManager.RegisterTerrainAABB(box);
    }

    // Place the first spawn on the floor before the appearance effect starts.
    // The default player position is above the starting platform.
    if (!savePosition)
    {
        const DirectX::XMFLOAT3 drop = {0.0f, -2.0f, 0.0f};
        TerrainSweepHit floorHit;
        if (collisionManager.SweepTerrain(player->GetBodyAABB(), drop, floorHit) == SweepStatus::Hit &&
            std::any_of(floorHit.normals.begin(), floorHit.normals.end(),
                        [](const DirectX::XMFLOAT3& normal) { return normal.y > 0.5f; }))
        {
            auto initialPosition = player->GetPosition();
            initialPosition.y += drop.y * floorHit.time + 0.001f;
            player->SetPosition(initialPosition);
        }
        lastSafePosition = player->GetPosition();
    }

    bossGates = BossGate::FromObjects(stageData.objects);

    for (const auto& gate : bossGates)
    {
        bossGateModels.push_back(
            ModelManager::Instance().CreateInstance(device, "Data/Model/Cube/Cube.cereal"));
    }

    UpdateBossGates(0.0f);
    profile.Step("collision_registration");
}

// 初期化処理
void GameScene::Initialize()
{
    DirectionalLight stageLight;
    LightManager::Instance().SetDirectionalLight(stageLight);
    const float screenWidth = Graphics::Instance().GetScreenWidth();
    const float screenHeight = Graphics::Instance().GetScreenHeight();
    Camera& camera = Camera::Instance();
    camera.SetPerspectiveFov(DirectX::XMConvertToRadians(45), // 画角
                             screenWidth / screenHeight,      // 画面アスペクト比
                             0.1f,                            // ニアクリップ
                             1000.0f                          // ファークリップ
    );
    camera.SetLookAtWithShake({8, 3, 0}, // 視点
                              {0, 2, 0}, // 注視点
                              {0, 1, 0}  // 上ベクトル
    );

    LoadingProfile::gameReady = true;

    if (readyPlaying)
    {
        UpdateCamera(100.0f);
    }

    Audio::Instance().Play("BGM_STAGE",true);
}

// デストラクタ
GameScene::~GameScene()
{
    Audio::Instance().Stop("SE_BOSS_WARNING");
    // EnemyWave は Player を非所有ポインタで参照するため、Player の破棄前に解放する
    EnemyManager::Instance().Clear();

    CollisionManager::Instance().ClearTerrainAABBs();
}

// 更新処理
void GameScene::Update(float elapsedTime)
{
    backgroundFog->Update(isPaused ? 0.0f : elapsedTime);
    backgroundCity->Update(isPaused ? 0.0f : elapsedTime);

    if (backgroundEditor->Update(elapsedTime))
    {
        return;
    }

    HitStopManager::Instance().Update(elapsedTime);

    // READYが消えてから既存のSpawn状態を初めて更新する
    if (readyPlaying)
    {
        if (!isPaused)
        {
            readyTime = (std::min)(READY_DURATION, readyTime + (std::max)(0.0f, elapsedTime));
            readyPlaying = readyTime < READY_DURATION;
        }
        UpdateCamera(isPaused ? 0.0f : elapsedTime);
        return;
    }

    float scaleElapsedTime = elapsedTime * HitStopManager::Instance().GetTimeScale();

    if (!isPaused)
    {
        UpdateBossGates(scaleElapsedTime);
        const bool introWasPlaying = warningPlaying || healthGaugeUI->IsBossIntroPlaying();
        UpdateBossWarning(elapsedTime);

        if (!introWasPlaying)
        {
            player->Update(scaleElapsedTime);
        }

        UpdateBossGates(0.0f);

        // 接地中の位置だけを記録し、空中では最後の安全な足場位置を保持する
        if (player->IsGround())
        {
            MakeSafetyMargin();
        }

        // 移動と接地判定の更新後に、Tiledで指定した落下範囲との重なりを調べる
        if (!introWasPlaying && !player->IsClearing() && !player->IsDead())
        {
            HandleFallRespawn();
            const auto& position = player->GetPosition();
            const AABB body = player->GetBodyAABB();
            for (const auto& point : stageData.savePoints)
            {
                // Save when any part of the body overlaps the XY rectangle.
                if (body.max.x >= point.minX && body.min.x <= point.maxX && body.max.y >= point.minY &&
                    body.min.y <= point.maxY)
                {
                    DirectX::XMFLOAT3 destination{point.minX + (point.maxX - point.minX) * 0.5f, point.minY,
                                                  position.z};
                    // Enlarging a trigger below the floor must not bury the respawn position.
                    // Repeat because lifting out of one box can enter an adjacent stacked box.
                    bool lifted;
                    do
                    {
                        lifted = false;
                        for (const auto& terrain : stage->GetTerrainAABBs())
                        {
                            if (destination.x + (body.max.x - body.min.x) * 0.5f > terrain.min.x &&
                                destination.x - (body.max.x - body.min.x) * 0.5f < terrain.max.x &&
                                destination.z + (body.max.z - body.min.z) * 0.5f > terrain.min.z &&
                                destination.z - (body.max.z - body.min.z) * 0.5f < terrain.max.z &&
                                destination.y >= terrain.min.y && destination.y <= terrain.max.y)
                            {
                                destination.y = terrain.max.y + 0.02f;
                                lifted = true;
                            }
                        }
                    } while (lifted);
                    player->deathRespawnPosition = destination;
                    break;
                }
            }
        }

        // プレイヤーがボス戦開始エリアへ入った場合、保留中のボスを生成する
        if (!player->IsClearing() && !player->IsDead())
        {
            TryStartBossEncounter();
        }

        if (!warningPlaying && !healthGaugeUI->IsBossIntroPlaying() && !player->IsDead())
        {
            EnemyManager::Instance().Update(scaleElapsedTime);
        }

        if (!warningPlaying)
        {
            healthGaugeUI->Update(*player, elapsedTime);
        }

        if (!warningPlaying && !healthGaugeUI->IsBossIntroPlaying())
        {
            if (const auto boss = introBoss.lock())
            {
                boss->SetActionEnable(true);
                Audio::Instance().Play("BGM_BOSS", true);
            }

            introBoss.reset();
        }
    }

    if (!isPaused)
    {
        Camera::Instance().ShakeUpdate(scaleElapsedTime);
    }

    UpdateCamera(elapsedTime);
}

// 描画処理
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
        rc.deviceContext->OMSetBlendState(rc.renderState->GetBlendState(BlendState::Opaque), nullptr,
                                          0xFFFFFFFF);
        backgroundEditor->Render(graphics.GetModelRenderer());
        graphics.GetModelRenderer()->Render(rc);
        backgroundFog->Render();
        return;
    }

    backgroundCity->Render();
    if (!readyPlaying)
    {
        healthGaugeUI->EnqueueRender();
    }
    EnqueueReady();
    EnqueueBossWarning();
    ID3D11DeviceContext* dc = Graphics::Instance().GetDeviceContext();
    RenderState* renderState = Graphics::Instance().GetRenderState();
    PrimitiveRenderer* primitiveRenderer = Graphics::Instance().GetPrimitiveRenderer();
    ShapeRenderer* shapeRenderer = Graphics::Instance().GetShapeRenderer();
    ModelRenderer* modelRenderer = Graphics::Instance().GetModelRenderer();

    Camera& camera = Camera::Instance();
    LightManager& lightManager = LightManager::Instance();
    const ViewVolume worldFrustum(camera.GetView(), camera.GetProjection());
    const auto terrainVisible = [&](const AABB& box)
    {
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

        if (stageObject->GetCollisionAABB(bounds) && !terrainVisible(bounds))
        {
            continue;
        }

        modelRenderer->Draw(ShaderId::Model, stageObject->GetModel(), {1.0f, 1.0f, 1.0f, 1.0f}, 1.0f);
    }

    for (std::size_t i = 0; i < bossGates.size(); ++i)
    {
        const auto& gate = bossGates[i];
        const auto& bounds = gate.GetBounds();
        const float visibleHeight = (bounds.max.y - bounds.min.y) * (1.0f - gate.GetOpenness());

        if (visibleHeight <= 0.0f)
        {
            continue;
        }

        auto center = bounds.GetCenter();
        auto half = bounds.GetHalfSize();

        half.y = visibleHeight * 0.5f;
        center.y = bounds.max.y - half.y;
        DirectX::XMFLOAT4X4 world;
        DirectX::XMStoreFloat4x4(&world,
                                 DirectX::XMMatrixScaling(half.x * 2.0f, half.y * 2.0f, half.z * 2.0f) *
                                     DirectX::XMMatrixTranslation(center.x, center.y, center.z));
        bossGateModels[i]->UpdateTransform(world);
        modelRenderer->Draw(ShaderId::Model, bossGateModels[i], {0.25f, 0.55f, 0.65f, 1.0f}, 1.0f);
    }

    if (!readyPlaying && player->GetScale().x > 0.0f && player->GetScale().y > 0.0f &&
        player->GetScale().z > 0.0f)
    {
        modelRenderer->Draw(ShaderId::Model, player->GetModel(), {1, 1, 1, 1}, 1.0f);
    }
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

#ifdef _DEBUG
    // 実際に衝突判定へ登録した箱を表示する
    for (const AABB& box : stage->GetTerrainAABBs())
    {
        if ((terrainAabbDisplay != 2 && terrainAabbDisplay != 3) || !terrainVisible(box))
        {
            continue;
        }

        shapeRenderer->DrawBox(box.GetCenter(), DirectX::XMFLOAT3{0.0f, 0.0f, 0.0f}, box.GetHalfSize(),
                               DirectX::XMFLOAT4{0.0f, 0.8f, 1.0f, 1.0f});
    }

    for (const auto& stageObject : stage->GetStageObjects())
    {
        AABB box;

        if ((terrainAabbDisplay != 1 && terrainAabbDisplay != 3) || !stageObject->GetCollisionAABB(box) ||
            !terrainVisible(box))
        {
            continue;
        }

        shapeRenderer->DrawBox(box.GetCenter(), DirectX::XMFLOAT3{0.0f, 0.0f, 0.0f}, box.GetHalfSize(),
                               DirectX::XMFLOAT4{1.0f, 0.8f, 0.0f, 1.0f});
    }

#endif

    // 実際の刀判定と同じ掃引カプセルをデバッグ表示する
    player->DrawDebugPrimitive(shapeRenderer);

#ifdef _DEBUG
    EnemyManager::Instance().DrawPrimitive(shapeRenderer);
    EnemyManager::Instance().DrawDebugPrimitive(shapeRenderer);
#endif
    dc->OMSetBlendState(renderState->GetBlendState(BlendState::Opaque), nullptr, 0xFFFFFFFF);
    dc->OMSetDepthStencilState(renderState->GetDepthStencilState(DepthState::TestOnly), 0);
    dc->RSSetState(renderState->GetRasterizerState(RasterizerState::SolidCullNone));
    shapeRenderer->Render(dc, camera.GetView(), camera.GetProjection());

    // 剣の軌跡の頂点をバッファに積む
    player->RenderTrail(primitiveRenderer);

    // ソードトレイル用ステートを明示
    dc->OMSetBlendState(renderState->GetBlendState(BlendState::Additive), nullptr, 0xFFFFFFFF);
    dc->OMSetDepthStencilState(renderState->GetDepthStencilState(DepthState::TestOnly), 0);

    // modelRendererによるステート変更を上書きする
    dc->RSSetState(renderState->GetRasterizerState(RasterizerState::SolidCullNone));

    primitiveRenderer->Render(dc, camera.GetView(), camera.GetProjection(),
                              D3D11_PRIMITIVE_TOPOLOGY_TRIANGLESTRIP);

    // ステートを戻す（次のフレーム/後続処理のため）
    dc->OMSetBlendState(renderState->GetBlendState(BlendState::Opaque), nullptr, 0xFFFFFFFF);
    dc->OMSetDepthStencilState(renderState->GetDepthStencilState(DepthState::TestAndWrite), 0);
}

// GUI描画処理
void GameScene::DrawGUI()
{
#ifdef _DEBUG
    backgroundFog->DrawGUI();
    backgroundCity->DrawGUI();

    if (backgroundEditor->IsEditing())
    {
        return;
    }

    if (ImGui::Begin("Debug"))
    {
        ImGui::Checkbox(u8"ゲーム時間を停止", &isPaused);
        ImGui::Combo(u8"地形AABB表示", &terrainAabbDisplay, u8"非表示\0合成前\0合成後\0両方\0");
        size_t originalAabbCount = 0;
        for (const auto& object : stage->GetStageObjects())
        {
            AABB box;
            if (object->GetCollisionAABB(box))
            {
                ++originalAabbCount;
            }
        }
        ImGui::Text(u8"AABB数: 合成前 %zu / 合成後 %zu", originalAabbCount, stage->GetTerrainAABBs().size());
        ImGui::Text("Enemy count: %zu", EnemyManager::Instance().GetEnemyCount());
#ifdef _DEBUG
        const bool canWarpToStage =
            !activeBossArena && !player->IsDead() && !player->IsClearing() && player->GetScale().x > 0.0f;

        if (canWarpToStage)
        {
            if (ImGui::Button("Warp: late stage / first Wave"))
            {
                DebugWarpToLateStage(136.5f, 15.5f);
            }

            if (ImGui::Button("Warp: upper Wave"))
            {
                DebugWarpToLateStage(172.5f, 28.5f);
            }
        }
        else
        {
            ImGui::TextDisabled("Stage warp unavailable during battle or respawn.");
        }

        ImGui::TextDisabled("Warp restores HP and resets Wave enemies.");
        const bool canWarp = pendingBossSpawnData.has_value() && !activeBossArena &&
                             !stageData.bossEncounterZones.empty() && !player->IsDead() &&
                             !player->IsClearing() && player->GetScale().x > 0.0f;

        if (canWarp)
        {
            if (ImGui::Button("Warp before boss"))
            {
                DebugWarpBeforeBoss();
            }
        }
        else
        {
            ImGui::TextDisabled("Boss warp unavailable (before battle only)");
        }

        if (debugWarpFailed)
        {
            ImGui::Text("No safe ground found for the warp.");
        }
#endif
    }

    ImGui::End();

    player->DrawGUI();

    cameraController.DrawMenuBar();

#endif
}

// XY座標が長方形の4境界すべてに収まっている場合だけ、範囲内と判定する
// 2.5Dの共通レーンを使うため、Z座標は比較しない
bool GameScene::IsInsideFallRespawnZone(const DirectX::XMFLOAT3& position,
                                        const FallRespawnZoneData& zone) const
{
    return position.x >= zone.minX && position.x <= zone.maxX && position.y >= zone.minY &&
           position.y <= zone.maxY;
}

// プレイヤーがボス前通路のカメラエリア内にいるか判定する
// 2.5DゲームなのでZ座標は判定に使用しない
bool GameScene::IsInsideBossApproachCameraZone(const DirectX::XMFLOAT3& position,
                                               const BossApproachCameraZoneData& zone) const
{
    return position.x >= zone.minX && position.x <= zone.maxX && position.y >= zone.minY &&
           position.y <= zone.maxY;
}

bool GameScene::IsInsideCameraLimitZone(const DirectX::XMFLOAT3& position,
                                        const CameraLimitZoneData& zone) const
{
    return position.x >= zone.minX && position.x <= zone.maxX && position.y >= zone.minY &&
           position.y <= zone.maxY;
}

bool GameScene::IsInsideBossEncounterZone(const DirectX::XMFLOAT3& position,
                                          const BossEncounterZoneData& zone) const
{
    return position.x >= zone.minX && position.x <= zone.maxX && position.y >= zone.minY &&
           position.y <= zone.maxY;
}

const BossApproachCameraZoneData* GameScene::FindBossApproachCameraZone(
    const DirectX::XMFLOAT3& position) const
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
    for (const BossEncounterZoneData& zone : stageData.bossEncounterZones)
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

// ステージ内の落下復帰エリアを検索し、該当した最初の1件で復帰処理を行う
void GameScene::HandleFallRespawn()
{
    // 地面に立っている場合は落下中ではないため、復帰させない
    if (player->IsGround())
    {
        return;
    }

    // ループ中に同じ現在位置を繰り返し取得しないよう、定数参照として保持する
    const DirectX::XMFLOAT3& playerPosition = player->GetPosition();

    // Tiledから読み込んだ各長方形と、プレイヤーの足元座標を順番に比較する
    for (const FallRespawnZoneData& zone : stageData.fallRespawnZones)
    {
        if (!IsInsideFallRespawnZone(playerPosition, zone))
        {
            continue;
        }

        player->Respawn(lastSafePosition);

        // 同時に複数の範囲を処理する必要はないため、復帰後は直ちに終了する
        return;
    }
}

void GameScene::MakeSafetyMargin()
{
    const AABB body = player->GetBodyAABB();

    constexpr float safetyMargin = 1.0f;

    const float leftX = body.min.x - safetyMargin;
    const float rightX = body.max.x + safetyMargin;

    float leftFloorY;
    float rightFloorY;

    auto& collision = CollisionManager::Instance();

    const bool leftGround =
        collision.FindTerrainFloor(leftX, body.min.z, body.min.y - 0.1f, body.min.y + 0.1f, leftFloorY);

    const bool rightGround =
        collision.FindTerrainFloor(rightX, body.min.z, body.min.y - 0.1f, body.min.y + 0.1f, rightFloorY);

    if (player->IsGround() && leftGround && rightGround)
    {
        lastSafePosition = player->GetPosition();
    }
}

#ifdef _DEBUG
void GameScene::DebugWarpToLateStage(float targetX, float targetFloorY)
{
    debugWarpFailed = true;

    if (activeBossArena || player->IsDead() || player->IsClearing())
    {
        return;
    }

    const auto body = player->GetBodyAABB();
    const auto oldPosition = player->GetPosition();
    const float halfWidth = (body.max.x - body.min.x) * 0.5f;
    const float halfDepth = (body.max.z - body.min.z) * 0.5f;
    DirectX::XMFLOAT3 destination = {};
    float bestScore = 1000000.0f;

    for (const auto& floor : stage->GetTerrainAABBs())
    {
        if (floor.max.x - floor.min.x < halfWidth * 2.0f + 0.02f || floor.min.z > oldPosition.z - halfDepth ||
            floor.max.z < oldPosition.z + halfDepth || std::abs(floor.max.y - targetFloorY) > 2.0f)
        {
            continue;
        }

        const float x = std::clamp(targetX, floor.min.x + halfWidth + 0.01f, floor.max.x - halfWidth - 0.01f);

        if (std::abs(x - targetX) > 3.0f)
        {
            continue;
        }

        const DirectX::XMFLOAT3 candidate = {x, floor.max.y + 0.01f, oldPosition.z};
        const DirectX::XMFLOAT3 offset = {candidate.x - oldPosition.x, candidate.y - oldPosition.y,
                                          candidate.z - oldPosition.z};
        const AABB targetBody = {{body.min.x + offset.x, body.min.y + offset.y, body.min.z + offset.z},
                                 {body.max.x + offset.x, body.max.y + offset.y, body.max.z + offset.z}};
        TerrainSweepHit hit;

        if (CollisionManager::Instance().SweepTerrain(targetBody, {0, 0, 0}, hit) ==
            SweepStatus::InitialOverlap)
        {
            continue;
        }

        const float score = std::abs(x - targetX) + 2.0f * std::abs(floor.max.y - targetFloorY);

        if (score < bestScore)
        {
            bestScore = score;
            destination = candidate;
        }
    }

    if (bestScore == 1000000.0f)
    {
        return;
    }

    player->DebugWarp(destination, 1.0f);
    lastSafePosition = destination;

    auto& enemies = EnemyManager::Instance();
    enemies.FlushPendingEnemies();

    for (const auto& wave : enemies.GetEnemiesByType<EnemyWave>())
    {
        enemies.Remove(wave);
    }

    for (const auto& spawn : stageData.enemies)
    {
        if (spawn.enemyType == EnemyType::Wave)
        {
            enemies.Register(
                EnemyFactory::CreateFromSpawnData(Graphics::Instance().GetDevice(), player.get(), spawn));
        }
    }

    enemies.FlushPendingEnemies();
    UpdateBossGates(0.0f);
    cameraController.SetMode(CameraController::CameraMode::Follow2D);
    UpdateCamera(100.0f);
    debugWarpFailed = false;
}

void GameScene::DebugWarpBeforeBoss()
{
    debugWarpFailed = true;

    if (!pendingBossSpawnData || activeBossArena || stageData.bossEncounterZones.empty() ||
        player->IsDead() || player->IsClearing())
    {
        return;
    }

    const auto& arena = stageData.bossEncounterZones.front();
    const BossGate* entryGate = nullptr;
    float nearestDistance = 2.0f;

    for (const auto& gate : bossGates)
    {
        const auto& bounds = gate.GetBounds();

        if (gate.IsLocked() || bounds.max.y <= arena.minY || bounds.min.y >= arena.maxY)
        {
            continue;
        }

        const float distance = gate.GetDirection() == BossGate::Direction::Right
                                   ? std::abs(bounds.max.x - arena.minX)
                                   : std::abs(bounds.min.x - arena.maxX);

        if (distance < nearestDistance)
        {
            entryGate = &gate;
            nearestDistance = distance;
        }
    }

    if (!entryGate)
    {
        return;
    }

    const auto body = player->GetBodyAABB();
    const float halfWidth = (body.max.x - body.min.x) * 0.5f;
    const float facing = entryGate->GetDirection() == BossGate::Direction::Right ? 1.0f : -1.0f;
    const auto& gateBounds = entryGate->GetBounds();
    DirectX::XMFLOAT3 destination = {facing > 0.0f ? gateBounds.min.x - halfWidth - 0.5f
                                                   : gateBounds.max.x + halfWidth + 0.5f,
                                     arena.minY, player->GetPosition().z};

    bool foundGround = false;
    float groundY = arena.minY - 2.0f;

    for (const auto& box : stage->GetTerrainAABBs())
    {
        if (destination.x - halfWidth < box.min.x || destination.x + halfWidth > box.max.x ||
            destination.z < box.min.z || destination.z > box.max.z || box.max.y < arena.minY - 2.0f ||
            box.max.y > arena.minY + 0.5f)
        {
            continue;
        }

        if (!foundGround || box.max.y > groundY)
        {
            groundY = box.max.y;
            foundGround = true;
        }
    }

    if (!foundGround)
    {
        return;
    }

    destination.y = groundY + 0.01f;

    AABB targetBody = body;
    const auto& oldPosition = player->GetPosition();
    const DirectX::XMFLOAT3 offset = {destination.x - oldPosition.x, destination.y - oldPosition.y,
                                      destination.z - oldPosition.z};
    targetBody.min = {body.min.x + offset.x, body.min.y + offset.y, body.min.z + offset.z};
    targetBody.max = {body.max.x + offset.x, body.max.y + offset.y, body.max.z + offset.z};

    for (const auto& box : stage->GetTerrainAABBs())
    {
        if (targetBody.max.x > box.min.x && targetBody.min.x < box.max.x && targetBody.max.y > box.min.y &&
            targetBody.min.y < box.max.y && targetBody.max.z > box.min.z && targetBody.min.z < box.max.z)
        {
            return;
        }
    }

    player->DebugWarp(destination, facing);
    lastSafePosition = destination;
    UpdateBossGates(0.0f);
    cameraController.SetMode(CameraController::CameraMode::Follow2D);

    UpdateCamera(100.0f);
    debugWarpFailed = false;
}
#endif

void GameScene::TryStartBossEncounter()
{
    // 保留中のボスがいなければ、未配置または既に生成済みなので何もしない
    if (!pendingBossSpawnData.has_value())
    {
        return;
    }

    if (warningPlaying)
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

        if (!activeBossArena)
        {
            activeBossArena = zone;
            lastSafePosition = playerPosition;
            player->PrepareForBossIntro();
            warningTime = 0.0f;
            warningPlaying = true;
            Audio::Instance().Stop("BGM_STAGE");
            Audio::Instance().Play("SE_BOSS_WARNING");
            return;
        }

        auto spawnedBoss = EnemyFactory::CreateFromSpawnData(Graphics::Instance().GetDevice(), player.get(),
                                                             pendingBossSpawnData.value());

        if (!spawnedBoss)
        {
            throw std::runtime_error("Failed to create boss from stage data.");
        }

        const auto boss = std::dynamic_pointer_cast<EnemyBoss>(spawnedBoss);

        if (!boss)
        {
            throw std::runtime_error("Boss encounter spawned a non-boss enemy.");
        }

        boss->SetArenaBounds(zone.minX, zone.maxX);
        boss->SetVelocity({0.0f, 0.0f, 0.0f});
        boss->SetActionEnable(false);
        introBoss = boss;
        player->PrepareForBossIntro();
        healthGaugeUI->ShowBoss(boss);

        EnemyManager::Instance().Register(spawnedBoss);

        EnemyManager::Instance().FlushPendingEnemies();

        // 配置情報を空にして、次のフレーム以降に同じボスが生成されるのを防ぐ
        activeBossArena = zone;
        lastSafePosition = playerPosition;
        pendingBossSpawnData.reset();
        Audio::Instance().Stop("BGM_STAGE");
        return;
    }
}

// READY文字の描画と横切る光を予約する
void GameScene::EnqueueReady()
{
    if (!readyPlaying)
    {
        return;
    }

    auto* hud = ImGui::GetForegroundDrawList();
    hud->AddCallback([](const ImDrawList*, const ImDrawCmd* command)
                     { static_cast<GameScene*>(command->UserCallbackData)->RenderReady(); }, this);
    hud->AddCallback(ImDrawCallback_ResetRenderState, nullptr);

    auto& graphics = Graphics::Instance();
    const float scale = (std::min)(graphics.GetScreenWidth() / 1280.0f, graphics.GetScreenHeight() / 720.0f);
    const float centerX = graphics.GetScreenWidth() * 0.5f;
    const float centerY = graphics.GetScreenHeight() * 0.45f;
    const float sweep = std::clamp(readyTime / 0.55f, 0.0f, 1.0f);
    if (sweep > 0.0f && sweep < 1.0f)
    {
        const float head = centerX + (sweep * 2.0f - 1.0f) * 760.0f * scale;
        for (int layer = 3; layer >= 1; --layer)
        {
            const float thickness = static_cast<float>(layer) * scale;
            hud->AddRectFilled(ImVec2(head - 540.0f * scale, centerY - thickness),
                               ImVec2(head, centerY + thickness), IM_COL32(100, 195, 255, 50 / layer));
        }
        hud->AddRectFilled(ImVec2(head - 320.0f * scale, centerY - scale), ImVec2(head, centerY + scale),
                           IM_COL32(220, 250, 255, 240));
    }
}

// READY本体と上下へ広がる半透明の残像を描画する
void GameScene::RenderReady()
{
    auto& graphics = Graphics::Instance();
    auto* dc = graphics.GetDeviceContext();
    auto* states = graphics.GetRenderState();
    dc->OMSetBlendState(states->GetBlendState(BlendState::Transparency), nullptr, 0xFFFFFFFF);
    dc->OMSetDepthStencilState(states->GetDepthStencilState(DepthState::NoTestNoWrite), 0);
    dc->RSSetState(states->GetRasterizerState(RasterizerState::SolidCullNone));
    auto* sampler = states->GetSamplerState(SamplerState::LinearClamp);
    dc->PSSetSamplers(0, 1, &sampler);

    const float scale = (std::min)(graphics.GetScreenWidth() / 1280.0f, graphics.GetScreenHeight() / 720.0f);
    const float centerX = graphics.GetScreenWidth() * 0.5f;
    const float centerY = graphics.GetScreenHeight() * 0.45f;
    const float reveal = std::clamp((readyTime - 0.22f) / 0.24f, 0.0f, 1.0f);
    const float fade = std::clamp((READY_DURATION - readyTime) / 0.18f, 0.0f, 1.0f);
    const float width = 640.0f * scale;
    const float height = width * 724.0f / 2172.0f;
    const auto draw = [&](float drawWidth, float drawHeight, float offsetY, float alpha)
    {
        readySprite->Render(dc, centerX - drawWidth * 0.5f, centerY - drawHeight * 0.5f + offsetY, 0,
                            drawWidth, drawHeight, 0, 1, 1, 1, alpha);
    };

    if (readyTime >= 0.38f && readyTime < 0.78f)
    {
        const float expansion = std::clamp((readyTime - 0.38f) / 0.67f, 0.0f, 1.0f);
        const float echoProgress = std::clamp((readyTime - 0.38f) / 0.4f, 0.0f, 1.0f);
        const float fadeIn = std::clamp(echoProgress / 0.15f, 0.0f, 1.0f);
        const float fadeOut = 1.0f - echoProgress;
        const float alpha = fadeIn * fadeOut * fadeOut * 0.18f;
        const float echoHeight = height * (1.0f + expansion * 1.8f);
        const float offset = (30.0f + expansion * 110.0f) * scale;
        draw(width * (1.0f + expansion * 0.18f), echoHeight, -offset, alpha);
        draw(width * (1.0f + expansion * 0.18f), echoHeight, offset, alpha);
    }

    if (reveal > 0.0f)
    {
        draw(width, height * (0.08f + reveal * 0.92f), 0, reveal * fade);
    }
}

// 警告演出の更新処理
void GameScene::UpdateBossWarning(float elapsedTime)
{
    if (warningPlaying)
    {
        warningTime += (std::max)(0.0f, elapsedTime);
        warningPlaying = warningTime < WARNING_DURATION;
        if (!warningPlaying)
        {
            Audio::Instance().Stop("SE_BOSS_WARNING");
        }
    }
}

// HUDより手前に警告の描画を予約する
void GameScene::EnqueueBossWarning()
{
    if (!warningPlaying)
    {
        return;
    }

    auto* hud = ImGui::GetForegroundDrawList();
    hud->AddCallback([](const ImDrawList*, const ImDrawCmd* command)
                     { static_cast<GameScene*>(command->UserCallbackData)->RenderBossWarning(); }, this);
    hud->AddCallback(ImDrawCallback_ResetRenderState, nullptr);

    auto& graphics = Graphics::Instance();
    const float scale = (std::min)(graphics.GetScreenWidth() / 1280.0f, graphics.GetScreenHeight() / 720.0f);
    const float progress = std::clamp((warningTime - 0.9f) / 0.35f, 0.0f, 1.0f);
    const float center = graphics.GetScreenWidth() * 0.5f;
    const float y = graphics.GetScreenHeight() * 0.32f + 82.0f * scale;
    const float pulse = 0.65f + 0.35f * std::cos(warningTime * 12.0f);
    const float fade = std::clamp((WARNING_DURATION - warningTime) / 0.2f, 0.0f, 1.0f);
    if (progress > 0.0f)
    {
        hud->AddRectFilled(ImVec2(center - 440.0f * scale * progress, y),
                           ImVec2(center + 440.0f * scale * progress, y + 5.0f * scale),
                           IM_COL32(255, 12, 8, static_cast<int>(255.0f * pulse * fade)));
    }
}

// 警告文字と出現中の残像を描画する
void GameScene::RenderBossWarning()
{
    auto& graphics = Graphics::Instance();
    auto* dc = graphics.GetDeviceContext();
    auto* states = graphics.GetRenderState();
    dc->OMSetBlendState(states->GetBlendState(BlendState::Transparency), nullptr, 0xFFFFFFFF);
    dc->OMSetDepthStencilState(states->GetDepthStencilState(DepthState::NoTestNoWrite), 0);
    dc->RSSetState(states->GetRasterizerState(RasterizerState::SolidCullNone));
    auto* sampler = states->GetSamplerState(SamplerState::LinearClamp);
    dc->PSSetSamplers(0, 1, &sampler);

    const float scale = (std::min)(graphics.GetScreenWidth() / 1280.0f, graphics.GetScreenHeight() / 720.0f);
    const float widths[] = {330, 330, 295, 285, 185, 295, 289};
    const float glyphScale = 0.43f * scale;
    float x = graphics.GetScreenWidth() * 0.5f - 2009.0f * glyphScale * 0.5f;
    const float y = graphics.GetScreenHeight() * 0.32f - 85.0f * scale;
    const float fade = std::clamp((WARNING_DURATION - warningTime) / 0.2f, 0.0f, 1.0f);
    const float pulse = warningTime < 1.2f ? 1.0f : 0.65f + 0.35f * std::cos(warningTime * 12.0f);
    for (std::size_t i = 0; i < warningLetters.size(); ++i)
    {
        const float age = warningTime - static_cast<float>(i) * 0.13f;
        const float width = widths[i] * glyphScale;
        if (age >= 0.0f)
        {
            const float progress = std::clamp(age / 0.28f, 0.0f, 1.0f);
            const float remaining = 1.0f - progress;
            for (int trail = 3; trail >= 0; --trail)
            {
                const float offset = remaining * static_cast<float>(trail + 1) * 18.0f * scale;
                const float alpha = (trail == 0 ? 1.0f : remaining * 0.18f) * progress * pulse * fade;
                warningLetters[i]->Render(dc, x + offset, y, 0, width * (1.0f + remaining * 0.45f),
                                          390.0f * glyphScale, 0, 1, 1, 1, alpha);
            }
        }
        x += width;
    }
}

// UpdateBossGates の更新処理
void GameScene::UpdateBossGates(float elapsedTime)
{
    if (bossGates.empty())
    {
        return;
    }

    auto& collisionManager = CollisionManager::Instance();
    collisionManager.ClearTerrainAABBs();

    for (const auto& box : stage->GetTerrainAABBs())
    {
        collisionManager.RegisterTerrainAABB(box);
    }

    const auto body = player->GetBodyAABB();

    for (auto& gate : bossGates)
    {
        const bool wasLocked = gate.IsLocked();
        gate.Update(elapsedTime, body);

        if (!wasLocked && gate.IsLocked())
        {
            lastSafePosition = player->GetPosition();
        }

        if (gate.IsSolid())
        {
            collisionManager.RegisterTerrainAABB(gate.GetBounds());
        }
    }
}

// カメラの更新処理
void GameScene::UpdateCamera(float elapsedTime)
{
    ImGuiIO& io = ImGui::GetIO();

    if (!io.WantCaptureMouse && io.MouseWheel != 0.0f)
    {
        cameraController.Zoom(io.MouseWheel);
    }

    const DirectX::XMFLOAT3& playerPosition = player->GetPosition();

    const BossEncounterZoneData* bossEncounterZone =
        activeBossArena ? &activeBossArena.value() : FindBossEncounterZone(playerPosition);

    const BossApproachCameraZoneData* approachCameraZone = FindBossApproachCameraZone(playerPosition);

    // ボス部屋のカメラを最優先する
    if (bossEncounterZone != nullptr)
    {
        const DirectX::XMFLOAT3 fixedFocus = {(bossEncounterZone->minX + bossEncounterZone->maxX) * 0.5f,
                                              (bossEncounterZone->minY + bossEncounterZone->maxY) * 0.5f,
                                              playerPosition.z};

        cameraController.UpdateFixed2D(elapsedTime, fixedFocus);
    }

    // ボス部屋の外で通路エリア内なら、通路用カメラを使用する
    else if (approachCameraZone != nullptr)
    {
        const DirectX::XMFLOAT3 fixedFocus = {(approachCameraZone->minX + approachCameraZone->maxX) * 0.5f,
                                              (approachCameraZone->minY + approachCameraZone->maxY) * 0.5f,
                                              playerPosition.z};

        cameraController.UpdateFixed2D(elapsedTime, fixedFocus);
    }

    // どちらの専用エリアにもいなければ、通常カメラを使用する
    else
    {
        cameraController.SetTarget(playerPosition);

        cameraController.Update(elapsedTime, playerPosition, player->GetVelocity(),
                                stageData.cameraLimitZones);
    }
}
