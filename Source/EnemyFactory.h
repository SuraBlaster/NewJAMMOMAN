#pragma once
#include <d3d11.h>
#include <memory>
#include "StageData.h"

class Enemy;
class Player;

namespace EnemyFactory
{
    std::shared_ptr<Enemy> CreateFromSpawnData(
        ID3D11Device* device, Player* player, const EnemySpawnData& spawnData);
}
