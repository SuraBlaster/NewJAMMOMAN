#include "SceneClear.h"
#include "SceneTitle.h"
#include "SceneManager.h"
#include "Graphics.h"
#include "Camera.h"
#include "Light.h"
#include "TextRenderer.h"
#include <algorithm>
#include <string>

namespace
{
    constexpr char Message[] = "Thank You For Playing";
    constexpr size_t MessageLength = sizeof(Message) - 1;
}

void SceneClear::Initialize()
{
    sceneTimer = waveTimer = 0.0f;
    presented = requested = false;
    background = std::make_unique<Sprite>(Graphics::Instance().GetDevice(),
        "Data/Sprite/LoadingBackground.png", true);
    player = std::make_unique<ClearPlayer>();
}

void SceneClear::Finalize()
{
    player.reset();
    background.reset();
}

void SceneClear::ReturnToTitle()
{
    if (requested) return;
    requested = true;
    SceneManager::Instance().ChangeScene([]() { return std::make_shared<SceneTitle>(); });
}

void SceneClear::Update(float elapsedTime)
{
    // Count visible time only, so loading stalls cannot skip the ending.
    const float dt = presented ? (std::clamp)(elapsedTime, 0.0f, 0.05f) : 0.0f;
    presented = false;
    sceneTimer += dt;
    if (sceneTimer >= TypewriterDelay + MessageLength * LetterInterval + 0.4f)
        player->BeginEntrance();
    const bool wasWaving = player->IsWaving();
    player->Update(dt);
    if (wasWaving) waveTimer += dt;
    if (waveTimer >= WaveDisplayTime) ReturnToTitle();
}

void SceneClear::Render(float elapsedTime)
{
    auto& graphics = Graphics::Instance();
    auto* dc = graphics.GetDeviceContext();
    auto* state = graphics.GetRenderState();
    graphics.Clear(0, 0, 0, 1);
    dc->OMSetBlendState(state->GetBlendState(BlendState::Opaque), nullptr, 0xFFFFFFFF);
    dc->OMSetDepthStencilState(state->GetDepthStencilState(DepthState::NoTestNoWrite), 0);
    dc->RSSetState(state->GetRasterizerState(RasterizerState::SolidCullNone));
    auto* sampler = state->GetSamplerState(SamplerState::LinearClamp);
    dc->PSSetSamplers(0, 1, &sampler);
    background->Render(dc, 0, 0, 0, graphics.GetScreenWidth(), graphics.GetScreenHeight(),
        0, 1, 1, 1, 1);

    const float scale = (std::min)(graphics.GetScreenWidth() / 1280, graphics.GetScreenHeight() / 720);
    D3D11_VIEWPORT previous = {};
    UINT count = 1;
    dc->RSGetViewports(&count, &previous);
    const D3D11_VIEWPORT viewport = {
        (graphics.GetScreenWidth() - 1280 * scale) * 0.5f,
        (graphics.GetScreenHeight() - 720 * scale) * 0.5f,
        1280 * scale, 720 * scale, 0, 1};
    dc->RSSetViewports(1, &viewport);
    auto& camera = Camera::Instance();
    camera.SetPerspectiveFov(DirectX::XMConvertToRadians(45), 1280.0f / 720, 0.1f, 1000);
    camera.SetLookAt({0, 0, -20}, {0, 0, 0}, {0, 1, 0});
    auto* renderer = graphics.GetModelRenderer();
    player->Render(renderer);
    RenderContext context = {};
    context.deviceContext = dc;
    context.renderState = state;
    context.camera = &camera;
    context.lightManager = &LightManager::Instance();
    renderer->Render(context);
    dc->RSSetViewports(1, &previous);
}

void SceneClear::DrawGUI()
{
    auto& graphics = Graphics::Instance();
    const float scale = (std::min)(graphics.GetScreenWidth() / 1280, graphics.GetScreenHeight() / 720);
    const float offsetX = (graphics.GetScreenWidth() - 1280 * scale) * 0.5f;
    const float offsetY = (graphics.GetScreenHeight() - 720 * scale) * 0.5f;
    const float progress = (std::max)(0.0f, sceneTimer - TypewriterDelay) / LetterInterval;
    const size_t count = static_cast<size_t>((std::min)(progress, static_cast<float>(MessageLength)));
    const std::string visible(Message, count);
    const float fontSize = 56 * scale;
    // Fix the full sentence's left edge so typing does not slide existing letters.
    const auto size = TextRenderer::Measure(Message, fontSize);
    TextRenderer::Draw(offsetX + 640 * scale - size.x * 0.5f,
        offsetY + 340 * scale - size.y * 0.5f, visible.c_str(), fontSize,
        {0.06f, 0.20f, 0.36f, 1});
    presented = true;
}
