#include <windows.h>
#include <imgui.h>
#include <iostream>
#include <stdexcept>
#include <vector>

#include "Framework.h"
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

int main()
{
    CoInitializeEx(nullptr, COINIT_APARTMENTTHREADED);
    WNDCLASSW windowClass = {};
    windowClass.lpfnWndProc = DefWindowProcW;
    windowClass.hInstance = GetModuleHandleW(nullptr);
    windowClass.lpszClassName = L"ProjectileEffectTest";
    RegisterClassW(&windowClass);
    HWND window = CreateWindowW(windowClass.lpszClassName, L"Effect test", WS_POPUP, 0, 0, 1280, 720, nullptr,
                                nullptr, windowClass.hInstance, nullptr);

    try
    {
        Framework framework(window);
        ImGui::GetIO().IniFilename = nullptr;
        auto effect = std::make_shared<Effect>("Data/Effect/ThunderShot.efkefc");
        auto manager = EffectManager::Instance().GetEffekseerManager();

        auto resource = Effekseer::Effect::Create(manager, u"Data/Effect/ThunderShot.efkefc");
        Check(resource != nullptr, "ThunderShot resource failed to load");

        for (int index = 0; index < resource->GetColorImageCount(); ++index)
        {
            Check(resource->GetColorImage(index) != nullptr, "Missing ThunderShot color texture");
        }

        for (int index = 0; index < resource->GetNormalImageCount(); ++index)
        {
            Check(resource->GetNormalImage(index) != nullptr, "Missing ThunderShot normal texture");
        }

        const auto handle = effect->Play({0, 0, 0}, 0.5f);
        Check(handle >= 0, "ThunderShot failed to load");
        effect->Stop(handle);

        std::vector<EnemyProjectile> projectiles;

        for (int index = 0; index < 20; ++index)
        {
            projectiles.emplace_back(DirectX::XMFLOAT3{0, 0, 0}, DirectX::XMFLOAT3{-7, 0, 0}, 0.2f, 5.0f, 1);
            projectiles.back().AttachEffect(effect, 0.5f);
        }

        for (auto& projectile : projectiles)
        {
            projectile.Update(1.0f / 60.0f);
        }

        EffectManager::Instance().Update(1.0f / 60.0f);
        Check(manager->GetTotalInstanceCount() > 0, "Vector growth stopped the effects");

        projectiles.erase(projectiles.begin() + 1, projectiles.begin() + 10);
        projectiles.front().Destroy();
        projectiles.back().Update(6.0f);
        Check(!projectiles.front().IsActive(), "Destroy did not deactivate bullet");
        Check(!projectiles.back().IsActive(), "Expired bullet remains active");
        projectiles.clear();

        for (int frame = 0; frame < 10; ++frame)
        {
            EffectManager::Instance().Update(1.0f / 60.0f);
        }

        Check(manager->GetTotalInstanceCount() == 0, "Projectile effects leaked after destruction");
        std::cout
            << "PASS: ThunderShot load, vector growth/erase, explicit destruction, expiry and cleanup\n";
    }
    catch (const std::exception& error)
    {
        std::cerr << error.what() << '\n';
        return 1;
    }

    DestroyWindow(window);
    CoUninitialize();
}
