#include "Character.h"
#include "CollisionManager.h"
#include <cstdlib>

void Model::UpdateTransform(const DirectX::XMFLOAT4X4&)
{
    std::abort();
}

Animator::Animator(Model*, int)
{
    std::abort();
}

void Animator::Play(int, int, bool, float, float, float)
{
    std::abort();
}

bool CollisionManager::Raycast(const DirectX::XMFLOAT3&, const DirectX::XMFLOAT3&, HitResult&)
{
    std::abort();
}
