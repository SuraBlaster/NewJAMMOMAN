#include <windows.h>
#include <imgui.h>
#include <iostream>
#include <stdexcept>

#include "Framework.h"
#include "CollisionManager.h"
#include "EnemyProjectile.h"
#include "Effect/Effect.h"
#include "Effect/EffectManager.h"

static void Check(bool condition, const char* message)
{
    if (!condition)
    {
        throw std::runtime_error(message);
    }
}

static void CheckCollision(const AABB& terrain, const DirectX::XMFLOAT3& start,
                           const DirectX::XMFLOAT3& velocity, float elapsedTime)
{
    auto& collision = CollisionManager::Instance();
    collision.ClearTerrainAABBs();
    collision.RegisterTerrainAABB(terrain);

    EnemyProjectile projectile(start, velocity);
    projectile.EnableTerrainCollision(nullptr);
    projectile.Update(elapsedTime);
    Check(!projectile.IsActive(), "Projectile crossed terrain");
}

static void TestTerrain()
{
    CheckCollision({{2, -5, -1}, {2.1f, 5, 1}}, {0, 0, 0}, {20, 0, 0}, 0.2f);
    CheckCollision({{-2.1f, -5, -1}, {-2, 5, 1}}, {0, 0, 0}, {-20, 0, 0}, 0.2f);
    CheckCollision({{-5, -2.1f, -1}, {5, -2, 1}}, {0, 0, 0}, {3, -10, 0}, 0.5f);
    CheckCollision({{-5, 2, -1}, {5, 2.1f, 1}}, {0, 0, 0}, {3, 10, 0}, 0.5f);
    CheckCollision({{2, 2, -1}, {2.1f, 2.1f, 1}}, {0, 0, 0}, {10, 10, 0}, 0.5f);
    CheckCollision({{-1, -1, -1}, {1, 1, 1}}, {0, 0, 0}, {7, 0, 0}, 1.0f / 60.0f);

    auto& collision = CollisionManager::Instance();
    collision.ClearTerrainAABBs();
    collision.RegisterTerrainAABB({{2, -5, -1}, {2.1f, 5, 1}});

    for (int fps : {10, 30, 60, 120})
    {
        EnemyProjectile projectile({0, 0, 0}, {7, 0, 0});
        projectile.EnableTerrainCollision(nullptr);

        for (int frame = 0; frame < fps; ++frame)
        {
            projectile.Update(1.0f / fps);
        }

        Check(!projectile.IsActive(), "FPS-dependent collision");
    }

    EnemyProjectile legacy({0, 0, 0}, {20, 0, 0});
    legacy.Update(0.2f);
    Check(legacy.IsActive(), "Unconfigured projectile behavior changed");

    EnemyProjectile offDepth({0, 0, 3}, {20, 0, 0});
    offDepth.EnableTerrainCollision(nullptr);
    offDepth.Update(0.2f);
    Check(offDepth.IsActive(), "Off-depth terrain hit");

    collision.ClearTerrainAABBs();
}

static void TestEffects()
{
    auto& collision = CollisionManager::Instance();
    auto manager = EffectManager::Instance().GetEffekseerManager();
    auto impact = std::make_shared<Effect>("Data/Effect/Fire.efkefc");
    Check(impact->Play({0, 0, 0}, 0.15f) >= 0, "Fire effect failed to load");
    manager->StopAllEffects();
    EffectManager::Instance().Update(1.0f / 60.0f);

    for (const char* path : {"Data/Effect/ThunderShot.efkefc", "Data/Effect/FlyEnemyShot.efkefc"})
    {
        manager->StopAllEffects();
        EffectManager::Instance().Update(1.0f / 60.0f);
        collision.ClearTerrainAABBs();
        collision.RegisterTerrainAABB({{2, -5, -1}, {2.1f, 5, 1}});

        auto shot = std::make_shared<Effect>(path);

        {
            EnemyProjectile projectile({0, 0, 0}, {7, 0, 0});
            projectile.AttachEffect(shot, 0.5f);
            projectile.EnableTerrainCollision(impact);
            projectile.Update(0.4f);
            Check(!projectile.IsActive(), "Effect bullet did not disappear at terrain");

            EffectManager::Instance().Update(1.0f / 60.0f);
            Check(manager->GetTotalInstanceCount() > 0, "Missing terrain impact effect");
            const int count = manager->GetTotalInstanceCount();

            for (int frame = 0; frame < 20; ++frame)
            {
                projectile.Update(1.0f / 60.0f);
            }

            Check(manager->GetTotalInstanceCount() == count, "Repeated impact effect");
        }

        EffectManager::Instance().Update(1.0f / 60.0f);
        Check(manager->GetTotalInstanceCount() > 0, "Bullet destruction stopped impact effect");
        manager->StopAllEffects();

        for (int frame = 0; frame < 10; ++frame)
        {
            EffectManager::Instance().Update(1.0f / 60.0f);
        }

        EnemyProjectile expired({0, 0, 0}, {7, 0, 0}, 0.18f, 0.1f);
        expired.EnableTerrainCollision(impact);
        expired.Update(1.0f);
        EffectManager::Instance().Update(1.0f / 60.0f);
        Check(!expired.IsActive(), "Bullet did not expire");
        Check(manager->GetTotalInstanceCount() == 0,
              "Expired bullet generated a terrain effect beyond its lifetime");
    }

    collision.ClearTerrainAABBs();
}

int main()
{
    CoInitializeEx(nullptr, COINIT_APARTMENTTHREADED);
    WNDCLASSW windowClass = {};
    windowClass.lpfnWndProc = DefWindowProcW;
    windowClass.hInstance = GetModuleHandleW(nullptr);
    windowClass.lpszClassName = L"ProjectileTerrainTest";
    RegisterClassW(&windowClass);
    HWND window = CreateWindowW(windowClass.lpszClassName, L"Projectile terrain test", WS_POPUP, 0, 0, 1280,
                                720, nullptr, nullptr, windowClass.hInstance, nullptr);

    try
    {
        Framework framework(window);
        ImGui::GetIO().IniFilename = nullptr;
        TestTerrain();
        TestEffects();
        std::cout
            << "PASS: walls/floor/ceiling, thin terrain, diagonal/embedded shots, 10-120 FPS, effect lifetime\n";
    }
    catch (const std::exception& error)
    {
        std::cerr << error.what() << '\n';
        return 1;
    }

    DestroyWindow(window);
    CoUninitialize();
}
