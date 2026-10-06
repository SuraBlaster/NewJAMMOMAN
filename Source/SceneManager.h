#pragma once
#include <functional>
#include <memory>
#include "Scene.h"

// Factories run only after the old scene has been destroyed.
class SceneManager
{
public:
    using SceneFactory = std::function<std::shared_ptr<Scene>()>;
    static SceneManager& Instance()
    {
        static SceneManager instance;
        return instance;
    }
    void Update(float elapsedTime);
    void Render(float elapsedTime);
    void DrawGUI();
    bool IsBackgroundEditing() const { return currentScene && currentScene->IsBackgroundEditing(); }
    void Clear();
    void ChangeScene(SceneFactory factory);
    void ChangeScene(std::shared_ptr<Scene> scene);
private:
    SceneManager() = default;
    SceneManager(const SceneManager&) = delete;
    SceneManager& operator=(const SceneManager&) = delete;
    std::shared_ptr<Scene> currentScene;
    SceneFactory nextScene;
};