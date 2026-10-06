#pragma once
#include "Scene.h"
#include "Sprite.h"
#include "ClearPlayer.h"
#include <memory>

class SceneClear : public Scene
{
public:
    void Initialize() override;
    void Finalize() override;
    void Update(float elapsedTime) override;
    void Render(float elapsedTime) override;
    void DrawGUI() override;
private:
    void ReturnToTitle();
    static constexpr float TypewriterDelay = 0.5f;
    static constexpr float LetterInterval = 0.08f;
    static constexpr float WaveDisplayTime = 4.0f;
    std::unique_ptr<Sprite> background;
    std::unique_ptr<ClearPlayer> player;
    float sceneTimer = 0.0f;
    float waveTimer = 0.0f;
    bool presented = false;
    bool requested = false;
};
