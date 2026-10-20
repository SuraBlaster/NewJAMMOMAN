#include "LoadingProfile.h"
#include "SceneTitle.h"
#include "Audio/Audio.h"
#include "SceneLoading.h"
#include "SceneManager.h"
#include "GameScene.h"
#include "InputManager.h"
#include "Graphics.h"
#include "Camera.h"
#include "Light.h"
#include "TextRenderer.h"
#include <imgui.h>

// 初期化処理
void SceneTitle::Initialize()
{
    isTransitioning = false;
    introTimer = transitionTimer = blinkTimer = 0.0f;
    firstUpdate = true;
    titleBgmStarted = false;
    titleBgmDelayTimer = 0.0f;
    introSoundPhase = -1;
#ifdef _DEBUG
    introPaused = false;
#endif
    auto* device = Graphics::Instance().GetDevice();
    const bool keepPngColors = true;
    triangle = std::make_unique<Sprite>(device, "Data/Sprite/Triangle.png", keepPngColors);
    logo = std::make_unique<Sprite>(device, "Data/Sprite/JAMMOMAN.png", keepPngColors);
    xMark = std::make_unique<Sprite>(device, "Data/Sprite/X.png", keepPngColors);
    whiteSprite = std::make_unique<Sprite>(device);
    titlePlayer = std::make_unique<TitlePlayer>();
    earth = std::make_shared<Model>(device, "Data/Model/earth/Earth.gltf");
    earthSpin = 0;
    UpdateDecorations(0);
}

// 終了処理
void SceneTitle::Finalize()
{
    Audio::Instance().Stop("SE_TITLE_TRIANGLE");
    Audio::Instance().Stop("SE_TITLE_ROTATION");
    Audio::Instance().Stop("SE_TITLE_APPEAR");
    Audio::Instance().Stop("BGM_TITLE");
    titleBgmStarted = false;
    triangle.reset();
    logo.reset();
    xMark.reset();
    whiteSprite.reset();
    titlePlayer.reset();
    earth.reset();
}

void SceneTitle::StartGame()
{
    if (isTransitioning)
    {
        return;
    }

    if (introTimer < TitleAnimation::INTRO_END)
    {
        introTimer = TitleAnimation::INTRO_END;
        return;
    }

    isTransitioning = true;
    transitionTimer = 2.0f;
    titlePlayer->BeginStart();
}

// 更新処理
void SceneTitle::Update(float elapsedTime)
{

    const float dt = firstUpdate ? 0.0f : (std::clamp)(elapsedTime, 0.0f, 0.1f);
    const bool acceptInput = !firstUpdate;
    firstUpdate = false;
    const float previousIntroTimer = introTimer;
#ifdef _DEBUG
    if (!introPaused)
#endif
        introTimer = (std::min)(introTimer + dt, TitleAnimation::INTRO_END);
    if (previousIntroTimer < TitleAnimation::X_END && introTimer >= TitleAnimation::X_END)
    {
        Audio::Instance().Play("SE_TITLE_APPEAR");
    }
    blinkTimer = std::fmod(blinkTimer + dt, 1.0f);

    if (LoadingProfile::Automatic() && acceptInput && !isTransitioning)
    {
        StartGame();
    }

    if (acceptInput)
    {
        UpdateInput();
    }

    UpdateIntroSound();
    if (!titleBgmStarted && introTimer >= TitleAnimation::X_END)
    {
#ifdef _DEBUG
        if (!introPaused)
#endif
        {
            // XがはまるSEを先に聞かせてからBGMを開始する。
            constexpr float BGM_DELAY = 0.5f;
            titleBgmDelayTimer += (std::min)(dt, (std::max)(0.0f, introTimer - TitleAnimation::X_END));
            if (titleBgmDelayTimer >= BGM_DELAY)
            {
                Audio::Instance().Play("BGM_TITLE", true);
                titleBgmStarted = true;
            }
        }
    }
    UpdateTransition(dt);

    if (introTimer >= TitleAnimation::X_END)
    {
        UpdateDecorations(dt);
    }
}

void SceneTitle::UpdateIntroSound()
{
    int phase = TitleAnimation::SoundPhase(introTimer);
#ifdef _DEBUG
    if (introPaused)
    {
        phase = -1;
    }
#endif
    if (phase == introSoundPhase)
    {
        return;
    }

    auto& audio = Audio::Instance();
    if (introSoundPhase >= 0 && introSoundPhase < 3)
    {
        audio.Stop("SE_TITLE_TRIANGLE");
    }
    else if (introSoundPhase == 3)
    {
        audio.Stop("SE_TITLE_ROTATION");
    }

    if (phase >= 0 && phase < 3)
    {
        audio.Play("SE_TITLE_TRIANGLE", true);
    }
    else if (phase == 3)
    {
        audio.Play("SE_TITLE_ROTATION");
    }
    introSoundPhase = phase;
}

// UpdateDecorations の更新処理
void SceneTitle::UpdateDecorations(float elapsedTime)
{
    using namespace DirectX;

    earthSpin = std::fmod(earthSpin + XMConvertToRadians(4.0f) * elapsedTime, XM_2PI);
    const XMMATRIX world = XMMatrixScaling(0.00225f, 0.00225f, 0.00225f) * XMMatrixRotationY(earthSpin) *
                           XMMatrixRotationZ(XMConvertToRadians(-45.0f)) *
                           XMMatrixTranslation(-6.0f, -19.0f, 5.0f);
    XMFLOAT4X4 transform;
    XMStoreFloat4x4(&transform, world);
    earth->UpdateTransform(transform);
    titlePlayer->Update(elapsedTime);
}

// RenderDecorations の描画処理
void SceneTitle::RenderDecorations()
{
    if (introTimer < TitleAnimation::X_END)
    {
        return;
    }

    auto& graphics = Graphics::Instance();
    auto& camera = Camera::Instance();
    camera.SetPerspectiveFov(DirectX::XMConvertToRadians(45),
                             graphics.GetScreenWidth() / graphics.GetScreenHeight(), 0.1f, 1000.0f);
    camera.SetLookAt({0, 0, -20}, {0, 0, 0}, {0, 1, 0});

    auto* renderer = graphics.GetModelRenderer();
    renderer->Draw(ShaderId::Model, earth);
    titlePlayer->Render(renderer);
    RenderContext context = {};
    context.deviceContext = graphics.GetDeviceContext();
    context.renderState = graphics.GetRenderState();
    context.camera = &camera;
    context.lightManager = &LightManager::Instance();
    renderer->Render(context);
}

// UpdateInput の更新処理
void SceneTitle::UpdateInput()
{
    const auto& io = ImGui::GetIO();
    bool keyPressed = false;

    if (!io.WantCaptureKeyboard)
    {
        for (int key = 0; key < IM_ARRAYSIZE(io.KeysDown); ++key)
        {
            keyPressed |= ImGui::IsKeyPressed(key, false);
        }
    }

    constexpr InputButton START_BUTTONS =
        InputManager::BTN_A | InputManager::BTN_B | InputManager::BTN_X | InputManager::BTN_Y |
        InputManager::BTN_START | InputManager::BTN_BACK | InputManager::BTN_UP | InputManager::BTN_RIGHT |
        InputManager::BTN_DOWN | InputManager::BTN_LEFT | InputManager::BTN_LEFT_THUMB |
        InputManager::BTN_RIGHT_THUMB | InputManager::BTN_LEFT_SHOULDER | InputManager::BTN_RIGHT_SHOULDER |
        InputManager::BTN_LEFT_TRIGGER | InputManager::BTN_RIGHT_TRIGGER;
    const bool padPressed = (InputManager::Instance().GetButtonDown() & START_BUTTONS) != 0;

    if (padPressed || keyPressed)
    {
        StartGame();
    }
}

// UpdateTransition の更新処理
void SceneTitle::UpdateTransition(float elapsedTime)
{
    if (!isTransitioning)
    {
        return;
    }

    transitionTimer -= elapsedTime;

    if (transitionTimer > 0.0f)
    {
        return;
    }

    SceneManager::Instance().ChangeScene(
        []()
        { return std::make_shared<SceneLoading>([]() { return std::make_shared<GameScene>(); }, true); });
}

// 描画処理
void SceneTitle::Render(float elapsedTime)
{
    auto& graphics = Graphics::Instance();
    auto* dc = graphics.GetDeviceContext();
    auto* renderState = graphics.GetRenderState();

    graphics.Clear(5.0f / 255, 8.0f / 255, 17.0f / 255, 1);
    RenderDecorations();
    dc->OMSetBlendState(renderState->GetBlendState(BlendState::Transparency), nullptr, 0xFFFFFFFF);
    dc->OMSetDepthStencilState(renderState->GetDepthStencilState(DepthState::NoTestNoWrite), 0);
    dc->RSSetState(renderState->GetRasterizerState(RasterizerState::SolidCullNone));
    auto* sampler = renderState->GetSamplerState(SamplerState::LinearClamp);
    dc->PSSetSamplers(0, 1, &sampler);

    screenScale = (std::min)(graphics.GetScreenWidth() / 1280, graphics.GetScreenHeight() / 720);
    screenOffsetX = (graphics.GetScreenWidth() - 1280 * screenScale) * 0.5f;
    screenOffsetY = (graphics.GetScreenHeight() - 720 * screenScale) * 0.5f;

    const auto animation = TitleAnimation::Evaluate(introTimer);
    RenderTriangleLines(dc, animation);
    RenderImages(dc, animation);

    if (animation.impactAlpha > 0)
    {
        whiteSprite->Render(dc, 0, 0, 0, graphics.GetScreenWidth(), graphics.GetScreenHeight(), 0, 1, 1, 1,
                            animation.impactAlpha * 0.22f);
    }
}

// RenderTriangleLines の描画処理
void SceneTitle::RenderTriangleLines(ID3D11DeviceContext* dc, const TitleAnimation::State& animation)
{
    if (animation.lineAlpha <= 0)
    {
        return;
    }

    const float fill =
        TitleAnimation::Progress(introTimer, TitleAnimation::HOLD_END, TitleAnimation::HOLD_END + 0.3f);

    if (fill > 0)
    {
        auto& graphics = Graphics::Instance();
        auto* renderer = graphics.GetPrimitiveRenderer();

        for (const auto& point : animation.vertices)
        {
            renderer->AddVertex({2 * (screenOffsetX + point.x * screenScale) / graphics.GetScreenWidth() - 1,
                                 1 - 2 * (screenOffsetY + point.y * screenScale) / graphics.GetScreenHeight(),
                                 0},
                                {1.0f, 0.0f, 0.0f, fill * animation.lineAlpha});
        }

        DirectX::XMFLOAT4X4 identity;
        DirectX::XMStoreFloat4x4(&identity, DirectX::XMMatrixIdentity());
        renderer->Render(dc, identity, identity, D3D11_PRIMITIVE_TOPOLOGY_TRIANGLELIST);
    }

    for (int edge = 0; edge < 3; ++edge)
    {
        const auto line = TitleAnimation::VisibleEdge(animation, edge);

        if (!line.visible)
        {
            continue;
        }

        const float alpha = animation.lineAlpha;
        RenderLine(dc, line.from, line.to, 2, {1.0f, 0.0f, 0.0f, alpha});
    }
}

// RenderLine の描画処理
void SceneTitle::RenderLine(ID3D11DeviceContext* dc, TitleAnimation::Point from, TitleAnimation::Point to,
                            float thickness, const DirectX::XMFLOAT4& color)
{

    const float length = TitleAnimation::Length(from, to);

    if (length <= 0)
    {
        return;
    }

    const float angle = DirectX::XMConvertToDegrees(std::atan2(to.y - from.y, to.x - from.x));
    const float centerX = (from.x + to.x) * 0.5f;
    const float centerY = (from.y + to.y) * 0.5f;

    whiteSprite->Render(dc, screenOffsetX + (centerX - length * 0.5f) * screenScale,
                        screenOffsetY + (centerY - thickness * 0.5f) * screenScale, 0, length * screenScale,
                        thickness * screenScale, angle, color.x, color.y, color.z, color.w);
}

// RenderImages の描画処理
void SceneTitle::RenderImages(ID3D11DeviceContext* dc, const TitleAnimation::State& animation)
{
    namespace Anim = TitleAnimation;
    const float scale = screenScale;

    if (animation.triangleAlpha > 0)
    {
        triangle->Render(dc, screenOffsetX + Anim::TRIANGLE_ORIGIN.x * scale,
                         screenOffsetY + Anim::TRIANGLE_ORIGIN.y * scale, 0, Anim::TRIANGLE_WIDTH * scale,
                         Anim::TRIANGLE_HEIGHT * scale, 0, 1, 1, 1, animation.triangleAlpha);
    }

    if (animation.logoProgress > 0)
    {
        logo->Render(dc, screenOffsetX + 190 * scale,
                     screenOffsetY + (190 + (1 - animation.logoProgress) * 24) * scale, 0, 750 * scale,
                     (750 * 276.0f / 1782) * scale, 0, 1, 1, 1, animation.logoProgress);
    }

    if (animation.xAlpha > 0)
    {

        const float height = 240 * animation.xScale;
        const float width = height * 760.0f / 1056;

        xMark->Render(dc, screenOffsetX + (1050 - width * 0.5f) * scale,
                      screenOffsetY + (255 - height * 0.5f) * scale, 0, width * scale, height * scale, 744,
                      701, 760, 1056, 0, 1, 1, 1, animation.xAlpha);
    }
}

// デバッグ画面の描画処理
void SceneTitle::DrawGUI()
{
    if (introTimer >= TitleAnimation::INTRO_END && !isTransitioning && blinkTimer < 0.5f)
    {
        TextRenderer::DrawCentered(screenOffsetX + 640 * screenScale, screenOffsetY + 600 * screenScale,
                                   "Press Any Button", 32 * screenScale);
    }
#ifdef _DEBUG
    DrawDebugGUI();
#endif
}

#ifdef _DEBUG

// DrawDebugGUI の描画処理
void SceneTitle::DrawDebugGUI()
{
    auto* viewport = ImGui::GetMainViewport();
    ImGui::SetNextWindowPos(ImVec2(viewport->Pos.x + 12, viewport->Pos.y + viewport->Size.y - 145),
                            ImGuiCond_Once);
    ImGui::SetNextWindowSize(ImVec2(320, 130), ImGuiCond_Once);

    if (ImGui::Begin("Title animation"))
    {
        if (ImGui::Checkbox("Pause intro", &introPaused))
        {
            UpdateIntroSound();
        }

        if (ImGui::SliderFloat("Time", &introTimer, 0, TitleAnimation::INTRO_END, "%.2f s"))
        {
            introPaused = true;
            UpdateIntroSound();
            isTransitioning = false;
            titlePlayer->Reset();
        }

        if (ImGui::Button("Replay intro"))
        {
            Audio::Instance().Stop("BGM_TITLE");
            titleBgmStarted = false;
            titleBgmDelayTimer = 0.0f;
            Audio::Instance().Stop("SE_TITLE_TRIANGLE");
            Audio::Instance().Stop("SE_TITLE_ROTATION");
            Audio::Instance().Stop("SE_TITLE_APPEAR");
            introSoundPhase = -1;
            introTimer = blinkTimer = 0;
            introPaused = isTransitioning = false;
            titlePlayer->Reset();
            earthSpin = 0;
            UpdateDecorations(0);
        }
    }

    ImGui::End();
}
#endif
