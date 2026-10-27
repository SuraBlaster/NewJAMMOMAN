#pragma once
#include "Character.h"
class ModelRenderer;

class TitlePlayer : public Character
{
  public:

    // コンストラクタ
    TitlePlayer();

    // 更新処理
    void Update(float elapsedTime) override;

    // 描画処理
    void Render(ModelRenderer* renderer);
    void BeginStart();
    void Reset();

  private:

    enum class State
    {
        Idle,
        WaitingToJump,
        Jump
    };
    State state = State::Idle;
    float timer = 0;
    static constexpr float GROUND_Y = -5.3f;
    static constexpr float JUMP_SPEED = 80.0f;
};
