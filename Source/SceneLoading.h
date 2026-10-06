#pragma once
#include "SceneManager.h"
#include <utility>
#include "Sprite.h"
#include <string>
#include "LoadingBoss.h"
#include "LoadingPlayer.h"

// GameScene creates GPU resources and touches shared managers in its constructor.
// Defer construction until this screen has been presented; do not use a worker.
class SceneLoading : public Scene
{
public:
    explicit SceneLoading(SceneManager::SceneFactory factory, bool fromTitle = false)
        : factory(std::move(factory)), showTitleIntro(fromTitle) {}
    void Initialize() override;
    void Finalize() override;
    void Update(float elapsedTime) override;
    void Render(float elapsedTime) override;
    void DrawGUI() override;
private:
    void RenderCharacters();
    std::unique_ptr<LoadingBoss> boss;
    std::unique_ptr<LoadingPlayer> player;
    void DrawDescription(float scale, float offsetX, float offsetY);
    void DrawLoadingText(float scale, float offsetX, float offsetY);
    static constexpr float TypewriterDelay = 0.6f;
    static constexpr float LetterInterval = 0.06f;
    static constexpr float MinimalDisplayTime = 0.3f;
    static constexpr float IntroReadTime = 2.0f;
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
