#pragma once
#include <memory>
#include <Model.h>
#include <CollisionManager.h>
#include <StageData.h>
#include <TileTypeManager.h>
#include "Collision.h"

class StageObject
{
public:
    StageObject(std::shared_ptr<Model> model, const StageObjectData& stageObjectData, const TileInfo& tileInfo, float positionZ, std::shared_ptr<Model> collisionModel = nullptr);
    ~StageObject();

    void UpdateTransform();

    const std::shared_ptr<Model>& GetModel() const { return model; }

    bool GetCollisionAABB(AABB& outBox) const
    {
        if (!hasCollisionAABB)
        {
            return false;
        }

        outBox = collisionAABB;
        return true;
    }
private:
    void BuildCollisionAABB();

private:
    std::shared_ptr<Model> model;
    std::shared_ptr<Model> collisionModel;
    CollisionMesh collisionMesh;
    bool registerCollisionMesh = false;

    DirectX::XMFLOAT3 position;
    DirectX::XMFLOAT3 rotation;
    DirectX::XMFLOAT3 scale;
    DirectX::XMFLOAT4X4 transform;

    AABB collisionAABB = {};
    bool hasCollisionAABB = false;
};
