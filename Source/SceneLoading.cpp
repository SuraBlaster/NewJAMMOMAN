#include "SceneLoading.h"
#include "Graphics.h"
#include "TextRenderer.h"
#include <algorithm>
#include <cmath>

void SceneLoading::Initialize()
{
    timer = 0.0f;
    presented = requested = false;
    displayedDescription.clear();
    background = std::make_unique<Sprite>(
        Graphics::Instance().GetDevice(), "Data/Sprite/LoadingBackground.png", true);
}

void SceneLoading::Finalize()
{
    background.reset();
}

void SceneLoading::Update(float elapsedTime)
{
    timer += (std::max)(0.0f, elapsedTime);

    // Derive the letter count from time, so slow frames do not slow down typing.
    const float progress = (std::max)(0.0f, timer - TypewriterDelay) / LetterInterval;
    const size_t count = static_cast<size_t>((std::min)(progress, static_cast<float>(description.size())));
    displayedDescription = description.substr(0, count);

    // Leave time to read the complete sentence before constructing the next scene.
    const float readTime = useSpecialRender ? 2.0f : 1.0f;
    const float delay = TypewriterDelay + description.size() * LetterInterval + readTime;
    if (factory && presented && !requested && timer >= delay)
    {
        requested = true;
        SceneManager::Instance().ChangeScene(std::move(factory));
    }
}

void SceneLoading::Render(float elapsedTime)
{
    auto& graphics = Graphics::Instance();
    auto* dc = graphics.GetDeviceContext();
    auto* state = graphics.GetRenderState();
    dc->OMSetBlendState(state->GetBlendState(BlendState::Opaque), nullptr, 0xFFFFFFFF);
    dc->OMSetDepthStencilState(state->GetDepthStencilState(DepthState::NoTestNoWrite), 0);
    dc->RSSetState(state->GetRasterizerState(RasterizerState::SolidCullNone));
    auto* sampler = state->GetSamplerState(SamplerState::LinearClamp);
    dc->PSSetSamplers(0, 1, &sampler);

    background->Render(dc, 0, 0, 0, graphics.GetScreenWidth(), graphics.GetScreenHeight(),
        0, 1, 1, 1, 1);
}

void SceneLoading::DrawGUI()
{
    auto& graphics = Graphics::Instance();
    const float scale = (std::min)(graphics.GetScreenWidth() / 1280, graphics.GetScreenHeight() / 720);
    const float offsetX = (graphics.GetScreenWidth() - 1280 * scale) * 0.5f;
    const float offsetY = (graphics.GetScreenHeight() - 720 * scale) * 0.5f;
    const DirectX::XMFLOAT4 headingColor = {0.06f, 0.20f, 0.36f, 1};

    TextRenderer::DrawCentered(offsetX + 640 * scale, offsetY + 150 * scale,
        "WIND", 84 * scale, headingColor);
    TextRenderer::DrawCentered(offsetX + 640 * scale, offsetY + 470 * scale,
        "GER", 84 * scale, headingColor);
    DrawDescription(scale, offsetX, offsetY);
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
    const float fontSize = 28 * scale;
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
