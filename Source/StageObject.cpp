#include "StageObject.h"

StageObject::StageObject(std::shared_ptr<Model> model, const StageObjectData& stageObjectData, const TileInfo& tileInfo, float positionZ)
{
    this->model = model;

    position.x = stageObjectData.positionX;
    position.y = stageObjectData.positionY;
    position.z = positionZ;

    rotation.x = 0;
    rotation.y = 0;
    rotation.z = DirectX::XMConvertToRadians(stageObjectData.rotationDegrees);

    scale.x = tileInfo.defaultScale.x * stageObjectData.scaleX;
    scale.y = tileInfo.defaultScale.y * stageObjectData.scaleY;
    scale.z = tileInfo.defaultScale.z;

    UpdateTransform();

    this->model->UpdateTransform(transform);

    if (tileInfo.collision)
    {
        collisionMesh.model = this->model.get();
        collisionMesh.transform = &transform;
        CollisionManager::Instance().Register(&collisionMesh);
        registerCollisionMesh = true;
    }
}

StageObject::~StageObject()
{
    if (registerCollisionMesh)
    {
        CollisionManager::Instance().Unregister(&collisionMesh);
    }
}

void StageObject::UpdateTransform()
{
    DirectX::XMMATRIX S = DirectX::XMMatrixScaling(scale.x, scale.y, scale.z);
    DirectX::XMMATRIX R = DirectX::XMMatrixRotationRollPitchYaw(rotation.x, rotation.y, rotation.z);
    DirectX::XMMATRIX T = DirectX::XMMatrixTranslation(position.x, position.y, position.z);
    DirectX::XMStoreFloat4x4(&transform, S * R * T);
}
