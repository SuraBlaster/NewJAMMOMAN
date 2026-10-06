#pragma once
#include "Character.h"
class ModelRenderer;

// Loading-screen presentation only; no combat or collision dependencies.
class LoadingBoss : public Character
{
public:
    LoadingBoss();
    void Update(float elapsedTime) override;
    void Render(ModelRenderer* renderer);
private:
    float attackTimer = 1.0f;
    bool attacking = false;
};
