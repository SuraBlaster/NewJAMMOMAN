#pragma once
#include "Character.h"
class ModelRenderer;

class ClearPlayer : public Character
{
  public:

    // コンストラクタ
    ClearPlayer();
    void BeginEntrance();

    // 更新処理
    void Update(float elapsedTime) override;

    // 描画処理
    void Render(ModelRenderer* renderer);

    bool IsWaving() const
    {
        return state == State::Wave;
    }

  private:

    enum class State
    {
        Waiting,
        Move,
        Wave
    };
    State state = State::Waiting;
    float waveTime = 0.0f;
    static constexpr float MOVE_SPEED = 5.0f;
};
