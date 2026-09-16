#pragma once
#include "Scene.h"

class SceneClear : public Scene
{
public:
    void Initialize() override;
    void Update(float elapsedTime) override;
    void DrawGUI() override;
private:
    void ReturnToTitle();
    float sceneTimer = 0.0f;
    bool requested = false;
};