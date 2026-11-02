#pragma once
#include <memory>
#include <functional>
#include "Scene.h"

class SceneManager
{
public:
    using SceneFactory = std::function<std::shared_ptr<Scene>()>;

    static SceneManager& Instance()
    {
        static SceneManager instance;
        return instance;
    }

    // 更新処理
    void Update(float elapsedTime);

    // 描画処理
    void Render(float elapsedTime);

    // デバッグ画面の描画処理
    void DrawGUI();

    bool IsBackgroundEditing() const
    {
        return currentScene && currentScene->IsBackgroundEditing();
    }

    bool UsesEscapeKey() const
    {
        return currentScene && currentScene->UsesEscapeKey();
    }

    bool IsGameplayPaused() const
    {
        return currentScene && currentScene->IsGameplayPaused();
    }

    void Clear();
    void ChangeScene(SceneFactory factory);
    void ChangeScene(std::shared_ptr<Scene> scene);

private:
    // コンストラクタ
    SceneManager() = default;

    // コンストラクタ
    SceneManager(const SceneManager&) = delete;
    SceneManager& operator=(const SceneManager&) = delete;
    std::shared_ptr<Scene> currentScene;
    SceneFactory nextScene;
};
