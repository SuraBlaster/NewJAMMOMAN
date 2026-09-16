#pragma once
#include "SceneManager.h"
#include <utility>
#include "Sprite.h"
#include <string>

// GameScene creates GPU resources and touches shared managers in its constructor.
// Defer construction until this screen has been presented; do not use a worker.
class SceneLoading : public Scene
{
public:
    explicit SceneLoading(SceneManager::SceneFactory factory, bool useSpecial = false)
        : factory(std::move(factory)), useSpecialRender(useSpecial) {}
    void Initialize() override;
    void Finalize() override;
    void Update(float elapsedTime) override;
    void Render(float elapsedTime) override;
    void DrawGUI() override;
private:
    void DrawDescription(float scale, float offsetX, float offsetY);
    void DrawLoadingText(float scale, float offsetX, float offsetY);
    static constexpr float TypewriterDelay = 0.6f;
    static constexpr float LetterInterval = 0.06f;
    const std::string description = "He crushes the player's will. At the labolatory";
    std::string displayedDescription;
    std::unique_ptr<Sprite> background;
    SceneManager::SceneFactory factory;
    bool useSpecialRender = false;
    bool presented = false;
    bool requested = false;
    float timer = 0.0f;
};
