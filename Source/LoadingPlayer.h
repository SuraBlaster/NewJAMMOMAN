#pragma once
#include "Character.h"
class ModelRenderer;

class LoadingPlayer : public Character
{
  public:

    // コンストラクタ
    LoadingPlayer();

    // 更新処理
    void Update(float elapsedTime) override;

    // 描画処理
    void Render(ModelRenderer* renderer);
};
