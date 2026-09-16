#include "SceneClear.h"
#include "SceneTitle.h"
#include "SceneLoading.h"
#include "SceneManager.h"
#include "GamePad.h"
#include "SceneUI.h"
#include <algorithm>
#include <cstring>

void SceneClear::Initialize()
{
    sceneTimer = 0.0f;
    requested = false;
}

void SceneClear::ReturnToTitle()
{
    if (requested) return;
    requested = true;
    SceneManager::Instance().ChangeScene([]() {
        return std::make_shared<SceneLoading>(
            []() { return std::make_shared<SceneTitle>(); });
    });
}

void SceneClear::Update(float elapsedTime)
{
    sceneTimer += elapsedTime;
    if (sceneTimer >= 8.0f ||
        ImGui::IsKeyPressed(ImGuiKey_Enter, false) ||
        (GamePad::Instance().GetButtonDown() & (GamePad::BTN_START | GamePad::BTN_A)))
        ReturnToTitle();
}

void SceneClear::DrawGUI()
{
    SceneUI::Background("CLEAR", IM_COL32(22, 42, 38, 255));
    if (SceneUI::BeginPanel("Clear controls"))
    {
        const char* text = "Thank You For Playing!";
        const int count = static_cast<int>((std::min)(sceneTimer / 0.05f,
            static_cast<float>(std::strlen(text))));
        ImGui::Text("%.*s", count, text);
        if (ImGui::Button("Return to Title", ImVec2(240, 42))) ReturnToTitle();
    }
    ImGui::End();
}