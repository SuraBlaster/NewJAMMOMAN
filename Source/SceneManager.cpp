#include "LoadingProfile.h"
#include "SceneManager.h"
#include <utility>

void SceneManager::Update(float elapsedTime)
{
    if (nextScene)
    {
        LoadingProfile::Scope profile("SceneTransition");
        auto factory = std::move(nextScene);
        nextScene = nullptr;
        if (currentScene) currentScene->Finalize();
        currentScene.reset();
        profile.Step("old_scene_cleanup");
        currentScene = factory();
        profile.Step("construct");
        if (currentScene) currentScene->Initialize();
        profile.Step("initialize");
    }
    if (currentScene) currentScene->Update(elapsedTime);
}

void SceneManager::Render(float elapsedTime)
{
    if (currentScene) currentScene->Render(elapsedTime);
}

void SceneManager::DrawGUI()
{
    if (currentScene) currentScene->DrawGUI();
}

void SceneManager::Clear()
{
    nextScene = nullptr;
    if (currentScene) currentScene->Finalize();
    currentScene.reset();
}

void SceneManager::ChangeScene(SceneFactory factory)
{
    if (factory) nextScene = std::move(factory);
}

void SceneManager::ChangeScene(std::shared_ptr<Scene> scene)
{
    if (scene && scene != currentScene)
        ChangeScene([scene]() { return scene; });
}