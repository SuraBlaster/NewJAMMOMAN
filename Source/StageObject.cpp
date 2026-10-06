#include "StageObject.h"
#include <algorithm>
#include <limits>

StageObject::StageObject(std::shared_ptr<Model> model, const StageObjectData& stageObjectData, const TileInfo& tileInfo, float positionZ, std::shared_ptr<Model> collisionModel)
{
    this->model = model;
    this->collisionModel = collisionModel ? collisionModel : model;

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
    if (this->collisionModel != this->model)
        this->collisionModel->UpdateTransform(transform);

    if (tileInfo.collision)
    {
        BuildCollisionAABB();
        collisionMesh.model = this->collisionModel.get();
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

void StageObject::BuildCollisionAABB()
{
    hasCollisionAABB = false;

    const float largest =
        (std::numeric_limits<float>::max)();

    const float lowest =
        (std::numeric_limits<float>::lowest)();

    AABB box = {
        { largest, largest, largest },
        { lowest, lowest, lowest }
    };

    bool hasVertex = false;

    const DirectX::XMMATRIX objectWorld =
        DirectX::XMLoadFloat4x4(&transform);

    for (const Model::Mesh& mesh : collisionModel->GetMeshes())
    {
        if (!mesh.node)
        {
            continue;
        }

        // モデル内部の変換に、ステージ上の配置を組み合わせる。
        const DirectX::XMMATRIX nodeTransform =
            DirectX::XMLoadFloat4x4(
                &mesh.node->globalTransform);

        const DirectX::XMMATRIX worldTransform =
            nodeTransform * objectWorld;

        for (const auto& vertex : mesh.vertices)
        {
            DirectX::XMFLOAT3 worldPosition;

            DirectX::XMStoreFloat3(
                &worldPosition,
                DirectX::XMVector3TransformCoord(
                    DirectX::XMLoadFloat3(&vertex.position),
                    worldTransform));

            box.min.x = (std::min)(
                box.min.x, worldPosition.x);

            box.min.y = (std::min)(
                box.min.y, worldPosition.y);

            box.min.z = (std::min)(
                box.min.z, worldPosition.z);

            box.max.x = (std::max)(
                box.max.x, worldPosition.x);

            box.max.y = (std::max)(
                box.max.y, worldPosition.y);

            box.max.z = (std::max)(
                box.max.z, worldPosition.z);

            hasVertex = true;
        }
    }

    if (!hasVertex)
    {
        return;
    }

    // 厚みのない箱は、今回の立体地形として扱わない。
    if (box.min.x >= box.max.x ||
        box.min.y >= box.max.y ||
        box.min.z >= box.max.z)
    {
        return;
    }

    collisionAABB = box;
    hasCollisionAABB = true;
}
