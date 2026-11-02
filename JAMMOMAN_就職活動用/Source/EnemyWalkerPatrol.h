#pragma once

#include "CollisionManager.h"

namespace EnemyWalkerPatrol
{
    inline bool ShouldTurn(const DirectX::XMFLOAT3& position, float radius, float height, float direction,
                           float displacement)
    {
        auto& collisions = CollisionManager::Instance();

        const float probeX = position.x + displacement + direction * (radius + 0.05f);

        float currentFloorY = 0.0f;
        float nextFloorY = 0.0f;

        const bool standingOnTerrain = collisions.FindTerrainFloor(position.x, position.z, position.y - 0.1f,
                                                                   position.y + 0.01f, currentFloorY);

        const bool floorAhead = collisions.FindTerrainFloor(probeX, position.z, position.y - 0.25f,
                                                            position.y + 0.01f, nextFloorY);

        const AABB body = {{position.x - radius, position.y + 0.02f, position.z - radius},
                           {position.x + radius, position.y + height, position.z + radius}};

        TerrainSweepHit wallHit;
        const bool wallAhead = collisions.CheckWall(body, displacement, wallHit);

        return wallAhead || (standingOnTerrain && !floorAhead);
    }
}
