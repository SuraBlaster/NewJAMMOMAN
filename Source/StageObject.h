#pragma once
#include <memory>
#include <Model.h>
#include <CollisionManager.h>
#include <StageData.h>
#include <FileTypeManager.h>

class StageObject
{
public:
    StageObject(std::shared_ptr<Model> model, const StageObjectData& stageObjectData, const TileInfo& tileInfo, float positionZ);
    ~StageObject();

    void UpdateTransform();

    const std::shared_ptr<Model>& GetModel() const { return model; }
private:
    std::shared_ptr<Model> model;
    CollisionMesh collisionMesh;
    bool registerCollisionMesh = false;

    DirectX::XMFLOAT3 position;
    DirectX::XMFLOAT3 rotation;
    DirectX::XMFLOAT3 scale;
    DirectX::XMFLOAT4X4 transform;
};
