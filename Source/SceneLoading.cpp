#include "LoadingProfile.h"
#include "SceneLoading.h"
#include "Graphics.h"
#include "TextRenderer.h"
#include "Camera.h"
#include "Light.h"
#include <algorithm>
#include <cmath>

void SceneLoading::Initialize()
{
    LoadingProfile::gameReady = false;
    LoadingProfile::firstGameFrame = false;
    LoadingProfile::loadingStart = LoadingProfile::Clock::now();
    LoadingProfile::Scope profile("Loading.prepare");
    timer = 0.0f;
    presented = requested = false;
    descriptionPresented = false;
    readTimer = 0.0f;
    displayedDescription.clear();
    if (showTitleIntro)
    {
        background = std::make_unique<Sprite>(
            Graphics::Instance().GetDevice(), "Data/Sprite/LoadingBackground.png", true);
        profile.Step("background");
        boss = std::make_unique<LoadingBoss>();
        profile.Step("boss");
    }
    player = std::make_unique<LoadingPlayer>();
    profile.Step("player");
    LoadingProfile::waitStart = LoadingProfile::Clock::now();
}

void SceneLoading::Finalize()
{
    background.reset();
    boss.reset();
    player.reset();
}

void SceneLoading::Update(float elapsedTime)
{
    // Loading stalls are not visible animation time. Advance only after a
    // rendered frame, and limit catch-up so one slow frame cannot skip the intro.
    const float dt = presented ? (std::clamp)(elapsedTime, 0.0f, 0.05f) : 0.0f;
    presented = false;
    timer += dt;
    if (boss) boss->Update(dt);
    player->Update(dt);
    if (!showTitleIntro)
    {
        if (factory && !requested && timer >= MinimalDisplayTime)
        {
            requested = true;
        LoadingProfile::Record("Loading.presentation_wait", LoadingProfile::Milliseconds(LoadingProfile::waitStart));
            SceneManager::Instance().ChangeScene(std::move(factory));
        }
        return;
    }
    if (descriptionPresented) readTimer += dt;

    // All animation uses the same visible timeline.
    const float progress = (std::max)(0.0f, timer - TypewriterDelay) / LetterInterval;
    const size_t count = static_cast<size_t>((std::min)(progress, static_cast<float>(description.size())));
    displayedDescription = description.substr(0, count);

    // Leave time to read the complete sentence before constructing the next scene.
    const float readTime = IntroReadTime;
    if (factory && descriptionPresented && !requested && readTimer >= readTime)
    {
        requested = true;
        LoadingProfile::Record("Loading.presentation_wait", LoadingProfile::Milliseconds(LoadingProfile::waitStart));
        SceneManager::Instance().ChangeScene(std::move(factory));
    }
}

void SceneLoading::Render(float elapsedTime)
{
    auto& graphics = Graphics::Instance();
    if (!showTitleIntro) graphics.Clear(0.0f, 0.0f, 0.0f, 1.0f);
    auto* dc = graphics.GetDeviceContext();
    auto* state = graphics.GetRenderState();
    dc->OMSetBlendState(state->GetBlendState(BlendState::Opaque), nullptr, 0xFFFFFFFF);
    dc->OMSetDepthStencilState(state->GetDepthStencilState(DepthState::NoTestNoWrite), 0);
    dc->RSSetState(state->GetRasterizerState(RasterizerState::SolidCullNone));
    auto* sampler = state->GetSamplerState(SamplerState::LinearClamp);
    dc->PSSetSamplers(0, 1, &sampler);

    if (background)
        background->Render(dc, 0, 0, 0, graphics.GetScreenWidth(), graphics.GetScreenHeight(),
            0, 1, 1, 1, 1);
    RenderCharacters();
}

void SceneLoading::RenderCharacters()
{
    auto& graphics = Graphics::Instance();
    auto* dc = graphics.GetDeviceContext();
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
    if (boss) boss->Render(renderer);
    player->Render(renderer);
    RenderContext context = {};
    context.deviceContext = dc;
    context.renderState = graphics.GetRenderState();
    context.camera = &camera;
    context.lightManager = &LightManager::Instance();
    renderer->Render(context);
    dc->RSSetViewports(1, &previous);
}

void SceneLoading::DrawGUI()
{
    auto& graphics = Graphics::Instance();
    const float scale = (std::min)(graphics.GetScreenWidth() / 1280, graphics.GetScreenHeight() / 720);
    const float offsetX = (graphics.GetScreenWidth() - 1280 * scale) * 0.5f;
    const float offsetY = (graphics.GetScreenHeight() - 720 * scale) * 0.5f;
    const DirectX::XMFLOAT4 headingColor = {0.06f, 0.20f, 0.36f, 1};

    if (showTitleIntro)
    {
        TextRenderer::DrawCentered(offsetX + 640 * scale, offsetY + 150 * scale,
            "WIND", 84 * scale, headingColor);
        TextRenderer::DrawCentered(offsetX + 640 * scale, offsetY + 470 * scale,
            "GER", 84 * scale, headingColor);
        DrawDescription(scale, offsetX, offsetY);
        if (displayedDescription.size() == description.size()) descriptionPresented = true;
    }
    DrawLoadingText(scale, offsetX, offsetY);
    presented = true;
}

void SceneLoading::DrawDescription(float scale, float offsetX, float offsetY)
{
    // Insert a visual line break without counting it as a typed character.
    std::string text = displayedDescription;
    const size_t lineBreak = description.find(" At");
    if (text.size() > lineBreak) text[lineBreak] = '\n';
    TextRenderer::Draw(offsetX + 100 * scale, offsetY + 315 * scale,
        text.c_str(), 30 * scale, {0.04f, 0.07f, 0.12f, 1});
}

void SceneLoading::DrawLoadingText(float scale, float offsetX, float offsetY)
{
    const char* text = "Now Loading";
    const float fontSize = 42 * scale;
    float x = offsetX + 1216 * scale - TextRenderer::Measure(text, fontSize).x;
    const float baseY = offsetY + 634 * scale;
    const float envelope = std::pow(std::sin(timer * 2.0f), 2.0f);

    for (size_t i = 0; text[i]; ++i)
    {
        const char letter[] = {text[i], '\0'};
        const float wave = (1 + std::sin(1.5708f + timer * 8.0f + i * 0.5f)) * 0.5f;
        const float y = baseY - envelope * wave * 14 * scale;
        TextRenderer::Draw(x, y, letter, fontSize, {0.88f, 0.95f, 1, 1});
        x += TextRenderer::Measure(letter, fontSize).x;
    }
}
