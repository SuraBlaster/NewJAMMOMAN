#include "TextRenderer.h"
#include <imgui.h>
#include <cassert>
#include <cmath>
#include <cstdio>

int main()
{
    assert(!TextRenderer::Initialize("Data/Font/ArialUni.ttf"));
    for (int run = 0; run < 2; ++run)
    {
        ImGui::CreateContext();
        auto& io = ImGui::GetIO();
        io.IniFilename = nullptr;
        io.DisplaySize = ImVec2(1280, 720);
        io.DeltaTime = 1.0f / 60;
        assert(!TextRenderer::Initialize("Data/Font/not-found.ttf"));
        assert(!TextRenderer::Initialize("Data/Font/ArialUni.ttf", 0));
        assert(TextRenderer::Initialize("Data/Font/ArialUni.ttf", 32));
        assert(!TextRenderer::Initialize("Data/Font/ArialUni.ttf", 32));
        unsigned char* pixels;
        int width, height;
        io.Fonts->GetTexDataAsRGBA32(&pixels, &width, &height);
        assert(io.Fonts->Fonts[0]->FindGlyphNoFallback(0x65e5));
        ImGui::NewFrame();
        auto* draw = ImGui::GetForegroundDrawList();
        const auto red = DirectX::XMFLOAT4(1, 0, 0, 1);
        TextRenderer::Draw(100, 100, "Text", 32, red);
        assert(draw->VtxBuffer.Size > 0);
        const float left = draw->VtxBuffer[0].pos.x;
        const int previous = draw->VtxBuffer.Size;
        TextRenderer::DrawCentered(640, 100, "Text", 32, red);
        const float centered = draw->VtxBuffer[previous].pos.x;
        const float textWidth = ImGui::GetFont()->CalcTextSizeA(32, FLT_MAX, 0, "Text").x;
        // ImGui snaps glyph positions to whole pixels.
        assert(std::abs(centered - left - (540 - textWidth / 2)) <= 1.0f);
        assert(draw->VtxBuffer[0].col == IM_COL32(255, 0, 0, 255));
        TextRenderer::Draw(100, 200, u8"日本語\nゲームスタート", 24);
        assert(draw->VtxBuffer.Size > previous * 2);
        const int count = draw->VtxBuffer.Size;
        TextRenderer::Draw(0, 0, nullptr);
        TextRenderer::Draw(0, 0, "");
        TextRenderer::Draw(0, 0, "invalid", -1);
        assert(draw->VtxBuffer.Size == count);
        ImGui::Render();
        assert(ImGui::GetDrawData()->TotalVtxCount > 0);
        TextRenderer::Finalize();
        ImGui::DestroyContext();
    }
    puts("TextRenderer: font loading, Japanese, alignment, color, lifecycle PASS");
}
