#include "SceneManager.h"
#include <utility>

void SceneManager::Update(float elapsedTime)
{
    if (nextScene)
    {
        auto factory = std::move(nextScene);
        nextScene = nullptr;
        if (currentScene) currentScene->Finalize();
        currentScene.reset();
        currentScene = factory();
        if (currentScene) currentScene->Initialize();
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