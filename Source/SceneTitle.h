#pragma once
#include <memory>
#include "Scene.h"
#include "Sprite.h"
#include "TitleAnimation.h"
#include "TitlePlayer.h"

class SceneTitle : public Scene
{
public:
    void Initialize() override;
    void Finalize() override;
    void Update(float elapsedTime) override;
    void Render(float elapsedTime) override;
    void DrawGUI() override;

private:
    void StartGame();
    void UpdateInput();
    void UpdateTransition(float elapsedTime);
    void UpdateDecorations(float elapsedTime);
    void RenderDecorations();
    void RenderTriangleLines(ID3D11DeviceContext* dc, const TitleAnimation::State& animation);
    void RenderImages(ID3D11DeviceContext* dc, const TitleAnimation::State& animation);
    void RenderLine(ID3D11DeviceContext* dc, TitleAnimation::Point from,
        TitleAnimation::Point to, float thickness, const DirectX::XMFLOAT4& color);
#ifdef _DEBUG
    void DrawDebugGUI();
#endif

    std::unique_ptr<Sprite> triangle;
    std::unique_ptr<Sprite> logo;
    std::unique_ptr<Sprite> xMark;
    std::unique_ptr<Sprite> whiteSprite;
    std::unique_ptr<TitlePlayer> titlePlayer;
    std::shared_ptr<Model> earth;
    float earthSpin = 0.0f;

    // Convert the 1280 x 720 animation coordinates to the current screen.
    float screenScale = 1.0f;
    float screenOffsetX = 0.0f;
    float screenOffsetY = 0.0f;

    bool isTransitioning = false;
    float transitionTimer = 0.0f;
    float blinkTimer = 0.0f;
    float introTimer = 0.0f;
    bool firstUpdate = true;
#ifdef _DEBUG
    bool introPaused = false;
#endif
};
