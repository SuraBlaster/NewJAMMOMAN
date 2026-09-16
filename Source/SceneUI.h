#pragma once
#include <imgui.h>

// Use the installed ImGui renderer and font; no external title assets are needed.
namespace SceneUI
{
    inline void Background(const char* heading, ImU32 color)
    {
        auto* viewport = ImGui::GetMainViewport();
        auto* draw = ImGui::GetBackgroundDrawList(viewport);
        const ImVec2 pos = viewport->Pos;
        const ImVec2 size = viewport->Size;
        draw->AddRectFilled(pos, ImVec2(pos.x + size.x, pos.y + size.y), color);
        const float fontSize = ImGui::GetFontSize() * 3.0f;
        const ImVec2 textSize = ImGui::GetFont()->CalcTextSizeA(fontSize, FLT_MAX, 0, heading);
        draw->AddText(ImGui::GetFont(), fontSize,
            ImVec2(pos.x + (size.x - textSize.x) * 0.5f, pos.y + size.y * 0.28f),
            IM_COL32(240, 245, 255, 255), heading);
    }

    inline bool BeginPanel(const char* name)
    {
        auto* viewport = ImGui::GetMainViewport();
        ImGui::SetNextWindowViewport(viewport->ID);
        ImGui::SetNextWindowPos(ImVec2(viewport->Pos.x + viewport->Size.x * 0.5f,
            viewport->Pos.y + viewport->Size.y * 0.55f), ImGuiCond_Always, ImVec2(0.5f, 0.5f));
        return ImGui::Begin(name, nullptr, ImGuiWindowFlags_NoDecoration |
            ImGuiWindowFlags_AlwaysAutoResize | ImGuiWindowFlags_NoMove |
            ImGuiWindowFlags_NoSavedSettings | ImGuiWindowFlags_NoBackground |
            ImGuiWindowFlags_NoDocking);
    }
}