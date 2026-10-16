#include <windows.h>
#include <chrono>
#include <iostream>
#include <imgui.h>

#include "Framework.h"
#include "GameScene.h"
#include "Graphics.h"
#include "ImGuiRenderer.h"
#include "InputManager.h"
#include "EnemyManager.h"
#include "EnemyWalker.h"
#include "Player.h"

using Clock = std::chrono::steady_clock;

static double Milliseconds(Clock::time_point begin, Clock::time_point end)
{
    return std::chrono::duration<double, std::milli>(end - begin).count();
}

static void Measure(GameScene& scene, const char* label)
{
    auto& graphics = Graphics::Instance();
    auto* context = graphics.GetDeviceContext();

    double update = 0.0;
    double render = 0.0;
    double gpu = 0.0;
    constexpr int FRAMES = 45;

    for (int frame = -10; frame < FRAMES; ++frame)
    {
        InputManager::Instance().Update();
        ImGuiRenderer::NewFrame();

        const auto begin = Clock::now();
        scene.Update(1.0f / 60.0f);
        const auto updated = Clock::now();

        D3D11_QUERY_DESC descriptor = {D3D11_QUERY_TIMESTAMP_DISJOINT, 0};
        Microsoft::WRL::ComPtr<ID3D11Query> disjoint;
        Microsoft::WRL::ComPtr<ID3D11Query> start;
        Microsoft::WRL::ComPtr<ID3D11Query> end;
        graphics.GetDevice()->CreateQuery(&descriptor, &disjoint);
        descriptor.Query = D3D11_QUERY_TIMESTAMP;
        graphics.GetDevice()->CreateQuery(&descriptor, &start);
        graphics.GetDevice()->CreateQuery(&descriptor, &end);

        context->Begin(disjoint.Get());
        context->End(start.Get());
        graphics.SetRenderTargets();
        scene.Render(1.0f / 60.0f);
        ImGuiRenderer::Render(context);
        context->End(end.Get());
        context->End(disjoint.Get());

        const auto rendered = Clock::now();
        context->Flush();

        D3D11_QUERY_DATA_TIMESTAMP_DISJOINT timing;

        while (context->GetData(disjoint.Get(), &timing, sizeof(timing), 0) == S_FALSE)
        {
            SwitchToThread();
        }

        UINT64 startTime = 0;
        UINT64 endTime = 0;
        context->GetData(start.Get(), &startTime, sizeof(startTime), 0);
        context->GetData(end.Get(), &endTime, sizeof(endTime), 0);

        if (frame >= 0)
        {
            update += Milliseconds(begin, updated);
            render += Milliseconds(updated, rendered);

            if (!timing.Disjoint)
            {
                gpu += double(endTime - startTime) * 1000.0 / timing.Frequency;
            }
        }
    }

    std::cout << label << " | enemies=" << EnemyManager::Instance().GetEnemyCount()
              << " | CPU update=" << update / FRAMES << "ms render=" << render / FRAMES
              << "ms GPU=" << gpu / FRAMES << "ms" << std::endl;
}

int main()
{
    CoInitializeEx(nullptr, COINIT_APARTMENTTHREADED);

    WNDCLASSW windowClass = {};
    windowClass.lpfnWndProc = DefWindowProcW;
    windowClass.hInstance = GetModuleHandleW(nullptr);
    windowClass.lpszClassName = L"EnemyFrameBenchmark";
    RegisterClassW(&windowClass);

    HWND window = CreateWindowW(windowClass.lpszClassName, L"Enemy benchmark", WS_POPUP, 0, 0, 1280, 720,
                                nullptr, nullptr, windowClass.hInstance, nullptr);

    try
    {
        Framework framework(window);
        ImGui::GetIO().IniFilename = nullptr;
        GameScene scene;
        scene.Initialize();

        Measure(scene, "Current mixed enemy course");
        EnemyManager::Instance().Clear();
        Measure(scene, "Same course without enemies");

        Player player(Graphics::Instance().GetDevice());
        player.SetPosition({10, 2, 0});

        for (int index = 0; index < 15; ++index)
        {
            auto enemy = std::make_shared<EnemyWalker>(Graphics::Instance().GetDevice(), &player);
            enemy->SetPosition({5.0f + index * 0.5f, -0.5f, 0});
            EnemyManager::Instance().Register(enemy);
        }

        Measure(scene, "Stress: 15 active walkers");
        EnemyManager::Instance().Clear();
    }
    catch (const std::exception& error)
    {
        std::cerr << error.what() << std::endl;
        return 1;
    }

    DestroyWindow(window);
    CoUninitialize();
}
