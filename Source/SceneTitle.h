#pragma once
#include <memory>
#include "Scene.h"
#include "Sprite.h"
#include "TitleAnimation.h"
#include "TitlePlayer.h"

class SceneTitle : public Scene
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

    void StartGame();
    void UpdateIntroSound();

    // UpdateInput の更新処理
    void UpdateInput();

    // UpdateTransition の更新処理
    void UpdateTransition(float elapsedTime);

    // UpdateDecorations の更新処理
    void UpdateDecorations(float elapsedTime);

    // RenderDecorations の描画処理
    void RenderDecorations();

    // RenderTriangleLines の描画処理
    void RenderTriangleLines(ID3D11DeviceContext* dc, const TitleAnimation::State& animation);

    // RenderImages の描画処理
    void RenderImages(ID3D11DeviceContext* dc, const TitleAnimation::State& animation);

    // RenderLine の描画処理
    void RenderLine(ID3D11DeviceContext* dc, TitleAnimation::Point from, TitleAnimation::Point to,
                    float thickness, const DirectX::XMFLOAT4& color);
#ifdef _DEBUG

    // DrawDebugGUI の描画処理
    void DrawDebugGUI();
#endif

    std::unique_ptr<Sprite> triangle;
    std::unique_ptr<Sprite> logo;
    std::unique_ptr<Sprite> xMark;
    std::unique_ptr<Sprite> whiteSprite;
    std::unique_ptr<TitlePlayer> titlePlayer;
    std::shared_ptr<Model> earth;
    float earthSpin = 0.0f;

    float screenScale = 1.0f;
    float screenOffsetX = 0.0f;
    float screenOffsetY = 0.0f;

    bool isTransitioning = false;
    float transitionTimer = 0.0f;
    float blinkTimer = 0.0f;
    float introTimer = 0.0f;
    bool firstUpdate = true;
    bool titleBgmStarted = false;
    float titleBgmDelayTimer = 0.0f;
    int introSoundPhase = -1;
#ifdef _DEBUG
    bool introPaused = false;
#endif
};
