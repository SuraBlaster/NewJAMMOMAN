#pragma once
#include "Character.h"
class ModelRenderer;

class LoadingBoss : public Character
{
  public:

    // コンストラクタ
    LoadingBoss();

    // 更新処理
    void Update(float elapsedTime) override;

    // 描画処理
    void Render(ModelRenderer* renderer);

  private:

    float attackTimer = 1.0f;
    bool attacking = false;
};
