#include <windows.h>
#include <imgui.h>
#include <algorithm>
#include <array>
#include <chrono>
#include <cmath>
#include <cstdio>
#include <fstream>
#include <memory>
#include <random>
#include <unordered_set>
#include "Model.h"
#include "Animator.h"
#include "Effect/Effect.h"
#include "PlayerTrail.h"
#include "Collision.h"
#include "GamePad.h"
#include "Input.h"
#include "Mouse.h"
#include "EnemyActivation.h"
#define private public
#define protected public
#include "Player.h"
#include "EnemyWave.h"
#include "InputManager.h"
#undef protected
#undef private

#pragma comment(linker,                                                                                      \
                "/alternatename:?UpdateStateMachine@Player@@QEAAXM@Z=?UpdateStateMachine@Player@@AEAAXM@Z")
#pragma comment(                                                                                             \
    linker, "/alternatename:?SetState@Player@@QEAAXW4StateId@1@@Z=?SetState@Player@@AEAAXW4StateId@1@@Z")
#pragma comment(                                                                                             \
    linker, "/alternatename:?CollideWithPlayer@EnemyWave@@QEAAXXZ=?CollideWithPlayer@EnemyWave@@AEAAXXZ")
#include "Framework.h"
#include "Graphics.h"
#include "Stage.h"
#include "StageLoader.h"
#include "TileTypeManager.h"
#include "CollisionManager.h"
#include "Effect/EffectManager.h"
#include "HitStopManager.h"
#include "BossGate.h"
#include "HealthGaugeUI.h"
#include "CameraController.h"
#include "BackgroundEditor.h"
#include "BackgroundFog.h"
#include "BackgroundCity.h"
#include "EnemyManager.h"
#include <optional>
#define private public
#include "GameScene.h"
#undef private
#pragma comment(                                                                                             \
    linker, "/alternatename:?HandleFallRespawn@GameScene@@QEAAXXZ=?HandleFallRespawn@GameScene@@AEAAXXZ")
#pragma comment(linker, "/alternatename:?UpdateCamera@GameScene@@QEAAXM@Z=?UpdateCamera@GameScene@@AEAAXM@Z")
#pragma comment(                                                                                             \
    linker,                                                                                                  \
    "/alternatename:?DebugWarpToLateStage@GameScene@@QEAAXMM@Z=?DebugWarpToLateStage@GameScene@@AEAAXMM@Z")

int main(int argc, char** argv)
{
    const bool withScene = argc > 1;
    CoInitializeEx(nullptr, COINIT_APARTMENTTHREADED);
    WNDCLASSW wc = {};
    wc.lpfnWndProc = DefWindowProcW;
    wc.hInstance = GetModuleHandleW(nullptr);
    wc.lpszClassName = L"PlayerAirborneInvestigation";
    RegisterClassW(&wc);
    HWND window = CreateWindowW(wc.lpszClassName, L"Physics test", WS_POPUP, 0, 0, 1280, 720, nullptr,
                                nullptr, wc.hInstance, nullptr);
    int result = 0;

    try
    {
        Framework framework(window);
        ImGui::GetIO().IniFilename = nullptr;
        Audio::Instance().SetMasterVolume(0);
        auto* device = Graphics::Instance().GetDevice();
        GameScene scene;
        scene.Initialize();

        EnemyManager::Instance().Clear();
        auto& collision = CollisionManager::Instance();
        Player& player = *scene.player;
        EnemyWave wave(device, &player);
        auto& input = InputManager::Instance();
        auto& effects = EffectManager::Instance();
        auto& hitStop = HitStopManager::Instance();

        if (argc > 1 && std::string(argv[1]) == "--warp")
        {
            player.SetScale({.01f, .01f, .01f});

            for (const auto target :
                 {DirectX::XMFLOAT3{136.5f, 15.5f, 0}, DirectX::XMFLOAT3{172.5f, 28.5f, 0},
                  DirectX::XMFLOAT3{136.5f, 15.5f, 0}})
            {
                player.health = 1;
                player.useGravity = false;
                scene.DebugWarpToLateStage(target.x, target.y);

                if (scene.debugWarpFailed || !player.useGravity || player.health != player.maxHealth ||
                    std::abs(player.position.x - target.x) > .1f ||
                    std::abs(player.position.y - target.y) > .02f)
                {
                    throw std::runtime_error("Warp destination/reset failed");
                }

                TerrainSweepHit hit;

                if (collision.SweepTerrain(player.GetBodyAABB(), {0, 0, 0}, hit) ==
                    SweepStatus::InitialOverlap)
                {
                    throw std::runtime_error("Warp overlaps terrain");
                }

                for (int frame = 0; frame < 10; ++frame)
                {
                    player.Update(1.f / 60);
                }

                if (!player.IsGround())
                {
                    throw std::runtime_error("Warp failed to land");
                }

                EnemyManager::Instance().Update(0);
                size_t expected = 0;

                for (const auto& spawn : scene.stageData.enemies)
                {
                    if (spawn.enemyType == EnemyType::Wave)
                    {
                        ++expected;
                    }
                }

                if (EnemyManager::Instance().GetEnemiesByType<EnemyWave>().size() != expected)
                {
                    throw std::runtime_error("Warp did not reset Wave count");
                }

                std::printf("PASS warp x=%.3f y=%.3f grounded=%d HP=%d Waves=%zu\n", player.position.x,
                            player.position.y, player.IsGround(), player.health, expected);
            }

            EnemyManager::Instance().Clear();
            return 0;
        }

        const Player::StateId priorStates[] = {Player::StateId::Idle,      Player::StateId::Move,
                                               Player::StateId::JumpFall,  Player::StateId::Dash,
                                               Player::StateId::WallSlide, Player::StateId::Attack};
        std::mt19937 random(143561706);
        std::uniform_real_distribution<float> offsetX(-6.0f, 0.0f), offsetY(-1.8f, .7f);
        int cases = 0, rejected = 0, overlaps = 0, gravityLeaks = 0, suspensions = 0, airWalkCases = 0;
        std::ofstream evidence(withScene ? "output/wave-diagnosis/scene-airborne-evidence.csv"
                                         : "output/wave-diagnosis/airborne-evidence.csv");
        evidence
            << "case,region,fps,prior,input,event,time,x,y,vy,gravityEnabled,wallClimb,state,nextState,overlap\n";

        for (const auto region : {DirectX::XMFLOAT3{143.5625f, 17.0625f, 0}, {176.4375f, 28.9375f, 0}})
        {
            for (int fps : {30, 60, 120})
            {
                for (auto prior : priorStates)
                {
                    for (int pattern = 0; pattern < 5; ++pattern)
                    {
                        for (int sample = 0; sample < 12; ++sample)
                        {
                            DirectX::XMFLOAT3 start = {region.x + offsetX(random), region.y + offsetY(random),
                                                       0};
                            player.PrepareForBossIntro();
                            player.SetPosition(start);
                            TerrainSweepHit initial;

                            if (collision.SweepTerrain(player.GetBodyAABB(), {0, 0, 0}, initial) ==
                                SweepStatus::InitialOverlap)
                            {
                                ++rejected;
                                continue;
                            }

                            effects.GetEffekseerManager()->StopAllEffects();
                            input.axisLX = 0;
                            input.buttons = input.buttonDown = input.buttonUp = 0;
                            player.useGravity = true;
                            player.isGround = false;
                            player.health = player.maxHealth = 20;
                            player.invincibleTime = 0;
                            scene.cameraController = CameraController{};
                            float bestSafeDistance = 1.0e9f;

                            for (const auto& box : scene.stage->GetTerrainAABBs())
                            {
                                if (box.max.x - box.min.x < .802f || box.min.z > -.4f || box.max.z < .4f ||
                                    box.max.y < region.y - 8 || box.max.y > region.y + 1)
                                {
                                    continue;
                                }

                                const float safeX =
                                    std::clamp(region.x - 3, box.min.x + .401f, box.max.x - .401f);
                                const float distance =
                                    std::abs(safeX - (region.x - 3)) + std::abs(region.y - box.max.y);

                                if (distance < bestSafeDistance)
                                {
                                    bestSafeDistance = distance;
                                    scene.lastSafePosition = {safeX, box.max.y + .001f, 0};
                                }
                            }

                            if (bestSafeDistance == 1.0e9f)
                            {
                                throw std::runtime_error("No safe floor for scene test");
                            }

                            player.SetState(prior);
                            player.UpdateStateMachine(1.0f / fps);

                            wave.SetPosition({start.x + (sample % 2 ? -.65f : .65f), start.y + .7f, 0});
                            wave.CollideWithPlayer();

                            if (player.GetHealth() != 19)
                            {
                                throw std::runtime_error("Wave contact failed");
                            }

                            const int id = ++cases;
                            bool seenWalk = false, seenOverlap = false, seenLeak = false,
                                 seenSuspension = false;
                            float stationary = 0, time = 0;
                            const auto record = [&](const char* event)
                            {
                                evidence << id << ',' << region.x << ',' << fps << ',' << int(prior) << ','
                                         << pattern << ',' << event << ',' << time << ',' << player.position.x
                                         << ',' << player.position.y << ',' << player.velocity.y << ','
                                         << player.useGravity << ',' << player.wallClimb << ','
                                         << int(player.currentState) << ',' << int(player.nextState) << ','
                                         << player.hasBodyOverlap << '\n';
                            };

                            for (int frame = 0; frame < fps * 2; ++frame)
                            {
                                const float rawDt = 1.0f / fps;
                                hitStop.Update(rawDt);
                                const float dt = rawDt * hitStop.GetTimeScale();
                                time += dt;
                                input.axisLX = pattern == 1 ? -1.f : pattern == 2 ? 1.f : 0.f;
                                input.buttons = pattern == 3   ? InputManager::BTN_B
                                                : pattern == 4 ? InputManager::BTN_A
                                                               : 0;

                                input.buttonDown = frame == fps ? input.buttons : 0;
                                input.buttonUp = 0;
                                const float previousY = player.position.y;
                                player.Update(dt);

                                if (withScene)
                                {
                                    if (player.IsGround())
                                    {
                                        scene.lastSafePosition = player.GetPosition();
                                    }

                                    scene.HandleFallRespawn();
                                    scene.UpdateCamera(rawDt);
                                }

                                if (player.hasBodyOverlap && !seenOverlap)
                                {
                                    seenOverlap = true;
                                    ++overlaps;
                                    record("overlap");
                                }

                                if (!player.useGravity && player.currentState != Player::StateId::Dash &&
                                    !seenLeak)
                                {
                                    seenLeak = true;
                                    ++gravityLeaks;
                                    record("gravity-leak");
                                }

                                if (!player.IsGround() &&
                                    (player.currentState == Player::StateId::Move ||
                                     player.currentState == Player::StateId::Idle) &&
                                    !seenWalk)
                                {
                                    seenWalk = true;
                                    ++airWalkCases;
                                    record("air-walk-animation");
                                }

                                if (!player.IsGround() && player.currentState != Player::StateId::Dash &&
                                    std::abs(player.position.y - previousY) < .000001f)
                                {
                                    stationary += dt;
                                }
                                else
                                {
                                    stationary = 0;
                                }

                                if (stationary > .4f && !seenSuspension)
                                {
                                    seenSuspension = true;
                                    ++suspensions;
                                    record("suspension");
                                }

                                if (player.position.y < start.y - 25)
                                {
                                    break;
                                }
                            }

                            if (cases % 100 == 0)
                            {
                                std::printf("progress cases=%d\n", cases);
                                std::fflush(stdout);
                            }

                            Audio::Instance().StopAll();
                        }
                    }
                }
            }
        }

        std::printf(
            "RESULT scene=%d cases=%d invalid-starts=%d overlap-cases=%d gravity-leaks=%d suspension-cases=%d airborne-idle-or-move=%d\n",
            withScene, cases, rejected, overlaps, gravityLeaks, suspensions, airWalkCases);

        collision.ClearTerrainAABBs();
        player.PrepareForBossIntro();
        player.SetPosition({140, 17, 0});
        player.SetVelocity({0, 0, 0});
        player.useGravity = false;
        const float before = player.position.y;
        input.axisLX = 0;
        input.buttons = input.buttonDown = input.buttonUp = 0;

        for (int i = 0; i < 60; ++i)
        {
            player.Update(1.f / 60);
        }

        if (player.position.y != before)
        {
            throw std::runtime_error("Suspension positive control failed");
        }

        std::puts("CONTROL: disabled gravity with zero vertical speed suspends the real Player");
        input.axisLX = 0;
        input.buttons = input.buttonDown = input.buttonUp = 0;
        effects.GetEffekseerManager()->StopAllEffects();
    }
    catch (const std::exception& e)
    {
        std::fprintf(stderr, "FAIL: %s\n", e.what());
        result = 1;
    }

    DestroyWindow(window);
    CoUninitialize();
    return result;
}
