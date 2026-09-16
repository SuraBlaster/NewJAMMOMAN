#include "TextRenderer.h"
#include <imgui.h>
#include <cmath>
#include <filesystem>

ImFont* TextRenderer::font = nullptr;

bool TextRenderer::Initialize(const char* fontPath, float fontSize)
{
    if (!ImGui::GetCurrentContext() || !fontPath || !*fontPath ||
        !std::isfinite(fontSize) || fontSize <= 0)
        return false;

    // Font changes after uploading the atlas require rebuilding GPU resources.
    // Keep this class startup-only so callers do not need to manage that process.
    auto* atlas = ImGui::GetIO().Fonts;
    if (font || atlas->Locked || atlas->IsBuilt()) return false;

    std::error_code error;
    if (!std::filesystem::is_regular_file(std::filesystem::u8path(fontPath), error))
        return false;

    font = atlas->AddFontFromFileTTF(fontPath, fontSize, nullptr, atlas->GetGlyphRangesJapanese());
    return font != nullptr;
}

void TextRenderer::Finalize()
{
    font = nullptr;
}

DirectX::XMFLOAT2 TextRenderer::Measure(const char* text, float size)
{
    if (!ImGui::GetCurrentContext() || !font || !text ||
        !std::isfinite(size) || size < 0 || !font->ContainerAtlas->IsBuilt())
        return {0, 0};
    const auto result = font->CalcTextSizeA(size > 0 ? size : font->FontSize, FLT_MAX, 0, text);
    return {result.x, result.y};
}

void TextRenderer::Draw(float x, float y, const char* text, float size,
    const DirectX::XMFLOAT4& color)
{
    DrawText(x, y, text, size, color, false);
}

void TextRenderer::DrawCentered(float x, float y, const char* text, float size,
    const DirectX::XMFLOAT4& color)
{
    DrawText(x, y, text, size, color, true);
}

void TextRenderer::DrawText(float x, float y, const char* text, float size,
    const DirectX::XMFLOAT4& color, bool centered)
{
    if (!ImGui::GetCurrentContext() || !font || !text || !*text ||
        !ImGui::GetIO().Fonts->Locked || !std::isfinite(size) || size < 0)
        return;

    const float fontSize = size > 0 ? size : font->FontSize;
    auto* viewport = ImGui::GetMainViewport();
    const ImU32 tint = ImGui::ColorConvertFloat4ToU32(ImVec4(color.x, color.y, color.z, color.w));
    auto* drawList = ImGui::GetForegroundDrawList(viewport);

    // Center each line separately, including Japanese UTF-8 strings.
    const char* line = text;
    do
    {
        const char* end = line;
        while (*end && *end != '\n') ++end;
        const float width = centered ? font->CalcTextSizeA(fontSize, FLT_MAX, 0, line, end).x : 0;
        const ImVec2 position(viewport->Pos.x + x - width * 0.5f, viewport->Pos.y + y);
        drawList->AddText(font, fontSize, position, tint, line, end);
        y += fontSize;
        if (!*end) break;
        line = end + 1;
    } while (*line);
}
