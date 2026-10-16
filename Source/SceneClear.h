#pragma once
#include "Scene.h"
#include "Sprite.h"
#include "ClearPlayer.h"
#include <memory>

class SceneClear : public Scene
{
  public:

    // 初期化処理
    void Initialize() override;

    // 終了処理
    void Finalize() override;

    // 更新処理
    void Update(float elapsedTime) override;

    // 描画処理
    void Render(float elapsedTime) override;

    // デバッグ画面の描画処理
    void DrawGUI() override;

  private:

    void ReturnToTitle();
    static constexpr float TYPEWRITER_DELAY = 0.5f;
    static constexpr float LETTER_INTERVAL = 0.08f;
    static constexpr float WAVE_DISPLAY_TIME = 4.0f;
    std::unique_ptr<Sprite> background;
    std::unique_ptr<ClearPlayer> player;
    float sceneTimer = 0.0f;
    float waveTimer = 0.0f;
    bool presented = false;
    bool requested = false;
};
