#pragma once

#include <windows.h>
#include "HighResolutionTimer.h"
#include "Scene.h"
#include "Audio/Audio.h"

class Framework
{
  public:

    // コンストラクタ
    Framework(HWND hWnd);

    // デストラクタ
    ~Framework();

  private:

    // 更新処理
    void Update(float elapsedTime);

    // 描画処理
    void Render(float elapsedTime);

    template <class T> void ChangeSceneButtonGUI(const char* name);

    void SceneSelectGUI();

    void CalculateFrameStats();

  public:

    int Run();
    LRESULT CALLBACK HandleMessage(HWND hWnd, UINT msg, WPARAM wParam, LPARAM lParam);

  private:

    std::unique_ptr<Audio> audio;
    const HWND hWnd;
    HighResolutionTimer timer;
};
