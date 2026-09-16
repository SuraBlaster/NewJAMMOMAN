#pragma once
#include "Character.h"
class ModelRenderer;

// Title-only idle -> delayed jump; no gameplay collision or input dependencies.
class TitlePlayer : public Character
{
public:
    TitlePlayer();
    void Update(float elapsedTime) override;
    void Render(ModelRenderer* renderer);
    void BeginStart();
    void Reset();

private:
    enum class State { Idle, WaitingToJump, Jump };
    State state = State::Idle;
    float timer = 0;
    static constexpr float GroundY = -5.3f;
    static constexpr float JumpSpeed = 80.0f;
};
