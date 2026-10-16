#include <windows.h>
#include <crtdbg.h>
#include <iostream>
#include <memory>
#include <cmath>
#include "Framework.h"
#include "GameScene.h"
#include "StageLoader.h"

int main()
{
    const HRESULT com = CoInitializeEx(nullptr, COINIT_APARTMENTTHREADED);

    if (FAILED(com))
    {
        return 1;
    }

    WNDCLASSW wc = {};
    wc.lpfnWndProc = DefWindowProcW;
    wc.hInstance = GetModuleHandleW(nullptr);
    wc.lpszClassName = L"GameSceneLoadingSmokeTest";
    RegisterClassW(&wc);
    HWND window = CreateWindowW(wc.lpszClassName, L"Loading test", WS_POPUP, 0, 0, 1280, 720, nullptr,
                                nullptr, wc.hInstance, nullptr);

    if (!window)
    {
        return 1;
    }

    int result = 0;

    try
    {
        Framework framework(window);
        auto scene = std::make_shared<GameScene>();
        scene->Initialize();

        if (!_CrtCheckMemory())
        {
            throw std::runtime_error("Heap corrupted after loading");
        }

        CameraController controller;
        CameraLimitZoneData left;
        left.minX = 0;
        left.maxX = 100;
        left.minY = -20;
        left.maxY = 20;
        left.cameraSection = "third";
        auto right = left;
        right.minX = 100;
        right.maxX = 200;
        const std::vector<CameraLimitZoneData> bounds = {left, right};

        for (float playerX : {95.0f, 96.0f, 99.0f, 100.0f, 101.0f, 105.0f})
        {
            controller.Update(1.0f / 60.0f, {playerX, 2, 0}, {0, 0, 0}, bounds);

            if (std::abs(Camera::Instance().GetFocus().x - playerX) > 0.001f)
            {
                throw std::runtime_error("Camera has extra horizontal follow lag inside section");
            }
        }

        TileTypeManager types;
        StageData data;

        if (!types.LoadDefinitions("Data/Stage/TileTypes.json") ||
            !StageLoader::Load("Data/Stage/LaboratoryCombinedCourse.stage.json", types, data))
        {
            throw std::runtime_error("Cannot load actual course camera bounds");
        }

        CameraController courseController;
        const float path[][2] = {{158, 23}, {159, 23}, {160, 23},    {165, 23},     {170, 23},
                                 {175, 23}, {175, 22}, {175, 21},    {175, 20.05f}, {175, 19},
                                 {175, 18}, {178, 18}, {179.3f, 18}, {180, 18},     {185, 18}};
        bool first = true;

        for (const auto& point : path)
        {
            const float currentY = Camera::Instance().GetFocus().y;
            float expectedY =
                first ? point[1] : currentY + (point[1] - currentY) * (1 - std::exp(-3.0f / 60.0f));
            expectedY = std::clamp(expectedY, point[1] + 2.5f - 4.338f, point[1] - 0.5f + 4.622f);
            courseController.Update(1.0f / 60.0f, {point[0], point[1], 0}, {0, 0, 0}, data.cameraLimitZones);
            const auto& focus = Camera::Instance().GetFocus();

            if (std::abs(focus.x - point[0]) > 0.001f || std::abs(focus.y - expectedY) > 0.001f)
            {
                throw std::runtime_error("Internal Third-Sixth seam changed the camera composition");
            }

            first = false;
        }

        const auto previousFocus = Camera::Instance().GetFocus();
        courseController.Update(1.0f / 60.0f, {186, 0, 0}, {0, 0, 0}, data.cameraLimitZones);

        if (Camera::Instance().GetFocus().x != previousFocus.x ||
            Camera::Instance().GetFocus().y != previousFocus.y)
        {
            throw std::runtime_error("Camera followed the player outside the section");
        }

        scene.reset();

        if (!_CrtCheckMemory())
        {
            throw std::runtime_error("Heap corrupted after scene destruction");
        }

        std::cout
            << "PASS: real GameScene loaded; heap valid; Third-Sixth has no composition changes; outside follow stopped\n";
    }
    catch (const std::exception& error)
    {
        std::cerr << "FAIL: " << error.what() << '\n';
        result = 1;
    }

    DestroyWindow(window);
    CoUninitialize();
    return result;
}
