#pragma once
#include "Character.h"
class ModelRenderer;

class ClearPlayer : public Character
{
public:
    ClearPlayer();
    void BeginEntrance();
    void Update(float elapsedTime) override;
    void Render(ModelRenderer* renderer);
    bool IsWaving() const { return state == State::Wave; }
private:
    enum class State { Waiting, Move, Wave };
    State state = State::Waiting;
    static constexpr float MoveSpeed = 5.0f;
};
