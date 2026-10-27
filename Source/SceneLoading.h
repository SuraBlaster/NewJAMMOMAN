#pragma once
#include "SceneManager.h"
#include <utility>
#include <atomic>
#include <exception>
#include <thread>
#include "Sprite.h"
#include <string>
#include "LoadingBoss.h"
#include "LoadingPlayer.h"

class SceneLoading : public Scene
{
  public:

    // コンストラクタ
    explicit SceneLoading(SceneManager::SceneFactory factory, bool fromTitle = false)
        : factory(std::move(factory)), showTitleIntro(fromTitle)
    {
    }

    // デストラクタ
    ~SceneLoading() override;

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

    void StartLoading();
    void JoinLoading();
    std::thread loadingThread;
    std::atomic<bool> loadComplete{false};
    std::shared_ptr<Scene> loadedScene;
    std::exception_ptr loadError;

    // RenderCharacters の描画処理
    void RenderCharacters();
    std::unique_ptr<LoadingBoss> boss;
    std::unique_ptr<LoadingPlayer> player;

    // DrawDescription の描画処理
    void DrawDescription(float scale, float offsetX, float offsetY);

    // DrawLoadingText の描画処理
    void DrawLoadingText(float scale, float offsetX, float offsetY);
    static constexpr float TYPEWRITER_DELAY = 0.6f;
    static constexpr float LETTER_INTERVAL = 0.06f;
    static constexpr float MINIMAL_DISPLAY_TIME = 0.3f;
    static constexpr float INTRO_READ_TIME = 2.0f;
    const std::string description = "He crushes the player's will. At the labolatory";
    std::string displayedDescription;
    std::unique_ptr<Sprite> background;
    SceneManager::SceneFactory factory;
    bool showTitleIntro = false;
    bool presented = false;
    bool requested = false;
    bool descriptionPresented = false;
    float readTimer = 0.0f;
    float timer = 0.0f;
};
