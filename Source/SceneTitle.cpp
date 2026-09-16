#include "SceneTitle.h"
#include "SceneLoading.h"
#include "SceneManager.h"
#include "GameScene.h"
#include "GamePad.h"
#include "Graphics.h"
#include "Camera.h"
#include "Light.h"
#include <imgui.h>

void SceneTitle::Initialize()
{
    isTransitioning = false;
    introTimer = transitionTimer = blinkTimer = 0.0f;
    firstUpdate = true;
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

void SceneTitle::Finalize()
{
    triangle.reset();
    logo.reset();
    xMark.reset();
    whiteSprite.reset();
    titlePlayer.reset();
    earth.reset();
}

void SceneTitle::StartGame()
{
    if (isTransitioning) return;
    // First press skips the intro; a fresh press starts the game.
    if (introTimer < TitleAnimation::IntroEnd)
    {
        introTimer = TitleAnimation::IntroEnd;
        return;
    }
    isTransitioning = true;
    transitionTimer = 2.0f;
    titlePlayer->BeginStart();
}

void SceneTitle::Update(float elapsedTime)
{
    // Model loading can be included in the first/next frame's elapsed time.
    // Start at zero and limit catch-up so the visible intro never jumps ahead.
    const float dt = firstUpdate ? 0.0f : (std::clamp)(elapsedTime, 0.0f, 0.1f);
    const bool acceptInput = !firstUpdate;
    firstUpdate = false;
#ifdef _DEBUG
    if (!introPaused)
#endif
        introTimer = (std::min)(introTimer + dt, TitleAnimation::IntroEnd);
    blinkTimer = std::fmod(blinkTimer + dt, 1.0f);
    if (acceptInput) UpdateInput();
    UpdateTransition(dt);
    if (introTimer >= TitleAnimation::XEnd)
        UpdateDecorations(dt);
}

void SceneTitle::UpdateDecorations(float elapsedTime)
{
    using namespace DirectX;
    // Spin around the local north axis, then tilt that axis clockwise on screen.
    earthSpin = std::fmod(earthSpin + XMConvertToRadians(4.0f) * elapsedTime, XM_2PI);
    const XMMATRIX world = XMMatrixScaling(0.00225f, 0.00225f, 0.00225f)
        * XMMatrixRotationY(earthSpin)
        * XMMatrixRotationZ(XMConvertToRadians(-45.0f))
        * XMMatrixTranslation(-6.0f, -19.0f, 5.0f);
    XMFLOAT4X4 transform;
    XMStoreFloat4x4(&transform, world);
    earth->UpdateTransform(transform);
    titlePlayer->Update(elapsedTime);
}

void SceneTitle::RenderDecorations()
{
    if (introTimer < TitleAnimation::XEnd) return;
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

void SceneTitle::UpdateInput()
{
    const auto buttons = GamePad::Instance().GetButtonDown();
    const bool padPressed = (buttons & (GamePad::BTN_START | GamePad::BTN_A |
        GamePad::BTN_B | GamePad::BTN_X | GamePad::BTN_Y)) != 0;
    const bool enterPressed = !ImGui::GetIO().WantTextInput &&
        ImGui::IsKeyPressed(ImGuiKey_Enter, false);
    if (padPressed || enterPressed) StartGame();
}

void SceneTitle::UpdateTransition(float elapsedTime)
{
    if (!isTransitioning) return;
    transitionTimer -= elapsedTime;
    if (transitionTimer > 0.0f) return;

    SceneManager::Instance().ChangeScene([]() {
        return std::make_shared<SceneLoading>(
            []() { return std::make_shared<GameScene>(); }, true);
    });
}

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
        whiteSprite->Render(dc, 0, 0, 0, graphics.GetScreenWidth(), graphics.GetScreenHeight(),
            0, 1, 1, 1, animation.impactAlpha * 0.22f);
}

void SceneTitle::RenderTriangleLines(ID3D11DeviceContext* dc, const TitleAnimation::State& animation)
{
    if (animation.lineAlpha <= 0) return;

    // During the turn, the wire becomes a red face before handing off to the PNG.
    const float fill = TitleAnimation::Progress(introTimer, TitleAnimation::HoldEnd,
        TitleAnimation::HoldEnd + 0.3f);
    if (fill > 0)
    {
        auto& graphics = Graphics::Instance();
        auto* renderer = graphics.GetPrimitiveRenderer();
        for (const auto& point : animation.vertices)
            renderer->AddVertex({
                2 * (screenOffsetX + point.x * screenScale) / graphics.GetScreenWidth() - 1,
                1 - 2 * (screenOffsetY + point.y * screenScale) / graphics.GetScreenHeight(), 0},
                {0.9f, 0.015f, 0.04f, fill * animation.lineAlpha});
        DirectX::XMFLOAT4X4 identity;
        DirectX::XMStoreFloat4x4(&identity, DirectX::XMMatrixIdentity());
        renderer->Render(dc, identity, identity, D3D11_PRIMITIVE_TOPOLOGY_TRIANGLELIST);
    }

    for (int edge = 0; edge < 3; ++edge)
    {
        const auto line = TitleAnimation::VisibleEdge(animation, edge);
        if (!line.visible) continue;

        const float alpha = animation.lineAlpha;
        RenderLine(dc, line.from, line.to, 2, {0.1f + 0.9f * fill, 1.0f, 0.05f, alpha});
    }
}

void SceneTitle::RenderLine(ID3D11DeviceContext* dc, TitleAnimation::Point from,
    TitleAnimation::Point to, float thickness, const DirectX::XMFLOAT4& color)
{
    // A thin white sprite rotated around its center becomes a line segment.
    const float length = TitleAnimation::Length(from, to);
    if (length <= 0) return;
    const float angle = DirectX::XMConvertToDegrees(std::atan2(to.y - from.y, to.x - from.x));
    const float centerX = (from.x + to.x) * 0.5f;
    const float centerY = (from.y + to.y) * 0.5f;

    whiteSprite->Render(dc,
        screenOffsetX + (centerX - length * 0.5f) * screenScale,
        screenOffsetY + (centerY - thickness * 0.5f) * screenScale, 0,
        length * screenScale, thickness * screenScale, angle,
        color.x, color.y, color.z, color.w);
}

void SceneTitle::RenderImages(ID3D11DeviceContext* dc, const TitleAnimation::State& animation)
{
    namespace Anim = TitleAnimation;
    const float scale = screenScale;

    if (animation.triangleAlpha > 0)
        triangle->Render(dc,
            screenOffsetX + Anim::TriangleOrigin.x * scale,
            screenOffsetY + Anim::TriangleOrigin.y * scale, 0,
            Anim::TriangleWidth * scale, Anim::TriangleHeight * scale,
            0, 1, 1, 1, animation.triangleAlpha);

    if (animation.logoProgress > 0)
        logo->Render(dc,
            screenOffsetX + 190 * scale,
            screenOffsetY + (190 + (1 - animation.logoProgress) * 24) * scale, 0,
            750 * scale, (750 * 276.0f / 1782) * scale,
            0, 1, 1, 1, animation.logoProgress);

    if (animation.xAlpha > 0)
    {
        const float height = 240 * animation.xScale;
        const float width = height * 760.0f / 1056;
        // Crop the transparent margins using Sprite::Render's source rectangle.
        xMark->Render(dc,
            screenOffsetX + (1050 - width * 0.5f) * scale,
            screenOffsetY + (255 - height * 0.5f) * scale, 0,
            width * scale, height * scale,
            744, 701, 760, 1056, 0,
            1, 1, 1, animation.xAlpha);
    }
}

void SceneTitle::DrawGUI()
{
    auto* viewport = ImGui::GetMainViewport();
    ImGui::SetNextWindowViewport(viewport->ID);
    ImGui::SetNextWindowPos(ImVec2(viewport->Pos.x + viewport->Size.x * 0.5f,
        viewport->Pos.y + viewport->Size.y * 0.84f), ImGuiCond_Always, ImVec2(0.5f, 0.5f));
    if (ImGui::Begin("Title controls", nullptr, ImGuiWindowFlags_NoDecoration |
        ImGuiWindowFlags_AlwaysAutoResize | ImGuiWindowFlags_NoMove |
        ImGuiWindowFlags_NoSavedSettings | ImGuiWindowFlags_NoBackground | ImGuiWindowFlags_NoDocking))
    {
        if (introTimer < TitleAnimation::IntroEnd)
        {
            if (ImGui::Button("Skip Intro", ImVec2(240, 42))) StartGame();
        }
        else if (isTransitioning) ImGui::TextUnformatted("Starting...");
        else
        {
            if (blinkTimer < 0.5f) ImGui::TextUnformatted("PRESS ENTER / START");
            else ImGui::NewLine();
            if (ImGui::Button("Start Game", ImVec2(240, 42))) StartGame();
        }
    }
    ImGui::End();
#ifdef _DEBUG
    DrawDebugGUI();
#endif
}

#ifdef _DEBUG
void SceneTitle::DrawDebugGUI()
{
    auto* viewport = ImGui::GetMainViewport();
    ImGui::SetNextWindowPos(ImVec2(viewport->Pos.x + 12, viewport->Pos.y + viewport->Size.y - 145), ImGuiCond_Once);
    ImGui::SetNextWindowSize(ImVec2(320, 130), ImGuiCond_Once);
    if (ImGui::Begin("Title animation"))
    {
        ImGui::Checkbox("Pause intro", &introPaused);
        if (ImGui::SliderFloat("Time", &introTimer, 0, TitleAnimation::IntroEnd, "%.2f s"))
        {
            introPaused = true;
            isTransitioning = false;
            titlePlayer->Reset();
        }
        if (ImGui::Button("Replay intro"))
        {
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
