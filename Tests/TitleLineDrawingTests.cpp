#include <imgui.h>
#include <cassert>
#include <cstdio>

int main()
{
    ImGui::CreateContext();
    auto& io = ImGui::GetIO();
    io.IniFilename = nullptr;
    io.DisplaySize = ImVec2(1280, 720);
    io.DeltaTime = 1.0f / 60.0f;
    unsigned char* pixels;
    int width, height;
    io.Fonts->GetTexDataAsRGBA32(&pixels, &width, &height);
    ImGui::NewFrame();
    auto* draw = ImGui::GetBackgroundDrawList();
    const ImVec2 triangle[] = { {640, 160}, {900, 540}, {380, 540} };
    for (int i = 0; i < 3; ++i)
    {
        const int previous = draw->VtxBuffer.Size;
        draw->AddLine(triangle[i], triangle[(i + 1) % 3], IM_COL32(255, 40, 70, 255), 3.0f);
        assert(draw->VtxBuffer.Size > previous);
    }
    ImGui::Render();
    const auto* data = ImGui::GetDrawData();
    assert(data->Valid && data->TotalVtxCount > 0 && data->TotalIdxCount > 0);
    for (int i = 0; i < draw->IdxBuffer.Size; ++i)
        assert(draw->IdxBuffer[i] < draw->VtxBuffer.Size);
    std::printf("Triangle lines: PASS (%d vertices, %d indices)\n",
        data->TotalVtxCount, data->TotalIdxCount);
    ImGui::DestroyContext();
}
