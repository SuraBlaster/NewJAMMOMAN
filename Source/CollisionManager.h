#pragma once

#include <vector>
#include <DirectXMath.h>
#include "Collision.h"

class Model;

struct HitResult
{
    DirectX::XMFLOAT3 position;
    DirectX::XMFLOAT3 normal;
    float distance;
};

struct CollisionMesh
{
    const Model* model = nullptr;
    const DirectX::XMFLOAT4X4* transform = nullptr;

    const AABB* bounds = nullptr;
};

struct TerrainSweepHit
{
    float time = 1.0f;
    std::vector<DirectX::XMFLOAT3> normals;
};

class CollisionManager
{
  private:

    // コンストラクタ
    CollisionManager() = default;

    // デストラクタ
    ~CollisionManager() = default;

  public:

    static CollisionManager& Instance()
    {
        static CollisionManager instance;
        return instance;
    }

  public:

    void Register(CollisionMesh* mesh);
    void Unregister(CollisionMesh* mesh);

    bool Raycast(const DirectX::XMFLOAT3& start, const DirectX::XMFLOAT3& end, HitResult& hitResult);

    bool ResolveTerrainPlacement(float x, float y, float z, float& floorY) const;
    bool FindTerrainFloor(float x, float z, float minimumY, float maximumY, float& floorY) const;

    void RegisterTerrainAABB(const AABB& box);
    void ClearTerrainAABBs();

    SweepStatus SweepTerrain(const AABB& movingBox, const DirectX::XMFLOAT3& displacement,
                             TerrainSweepHit& hit) const;
    bool CheckWall(const AABB& body, float displacementX, TerrainSweepHit& hit) const;

  private:

    bool RayCast(const CollisionMesh* mesh, const DirectX::XMFLOAT3& start, const DirectX::XMFLOAT3& end,
                 HitResult& hitResult);

  private:

    std::vector<CollisionMesh*> meshes;
    std::vector<AABB> terrainBoxes;
};