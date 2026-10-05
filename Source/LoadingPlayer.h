#pragma once
#include "Character.h"
class ModelRenderer;

class LoadingPlayer : public Character
{
public:
    LoadingPlayer();
    void Update(float elapsedTime) override;
    void Render(ModelRenderer* renderer);
};
