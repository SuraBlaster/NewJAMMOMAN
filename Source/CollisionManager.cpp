#include <DirectXCollision.h>
#include "CollisionManager.h"
#include "Model.h"

void CollisionManager::Register(CollisionMesh* mesh)
{
    meshes.emplace_back(mesh);
}

void CollisionManager::Unregister(CollisionMesh* mesh)
{
    auto it = std::find(meshes.begin(), meshes.end(), mesh);

    if (it != meshes.end())
    {
        meshes.erase(it);
    }
}

bool CollisionManager::Raycast(const DirectX::XMFLOAT3& start, const DirectX::XMFLOAT3& end,
                               HitResult& hitResult)
{
    bool hit = false;
    hitResult.distance = FLT_MAX;
    const AABB segmentBounds = {
        {(std::min)(start.x, end.x), (std::min)(start.y, end.y), (std::min)(start.z, end.z)},
        {(std::max)(start.x, end.x), (std::max)(start.y, end.y), (std::max)(start.z, end.z)}};

    for (const CollisionMesh* mesh : meshes)
    {
        if (mesh->bounds)
        {
            const auto& box = *mesh->bounds;

            if (segmentBounds.max.x < box.min.x || segmentBounds.min.x > box.max.x ||
                segmentBounds.max.y < box.min.y || segmentBounds.min.y > box.max.y ||
                segmentBounds.max.z < box.min.z || segmentBounds.min.z > box.max.z)
            {
                continue;
            }
        }

        HitResult tempResult;

        if (RayCast(mesh, start, end, tempResult))
        {
            if (tempResult.distance < hitResult.distance)
            {
                hitResult = tempResult;
            }

            hit = true;
        }
    }

    return hit;
}

bool CollisionManager::ResolveTerrainPlacement(float x, float y, float z, float& floorY) const
{
    float top = y;
    bool found = false, expanded;
    do
    {
        expanded = false;

        for (const auto& box : terrainBoxes)
        {
            if (x < box.min.x || x > box.max.x || z < box.min.z || z > box.max.z ||
                box.min.y > top + 0.0001f || box.max.y <= top + 0.0001f)
            {
                continue;
            }

            top = box.max.y;
            found = expanded = true;
        }
    } while (expanded);

    if (found)
    {
        floorY = top;
    }

    return found;
}

bool CollisionManager::FindTerrainFloor(float x, float z, float minimumY, float maximumY, float& floorY) const
{
    bool found = false;

    for (const auto& box : terrainBoxes)
    {
        if (x < box.min.x || x > box.max.x || z < box.min.z || z > box.max.z || box.max.y < minimumY ||
            box.max.y > maximumY)
        {
            continue;
        }

        if (!found || box.max.y > floorY)
        {
            floorY = box.max.y;
        }

        found = true;
    }

    return found;
}

bool CollisionManager::CheckWall(const AABB& body, float displacementX, TerrainSweepHit& hit) const
{
    hit = TerrainSweepHit{};

    if (displacementX == 0.0f)
    {
        return false;
    }

    if (SweepTerrain(body, {displacementX, 0.0f, 0.0f}, hit) != SweepStatus::Hit)
    {
        return false;
    }

    const float direction = displacementX < 0.0f ? -1.0f : 1.0f;

    for (const auto& normal : hit.normals)
    {
        if (normal.x * direction < -0.5f)
        {
            return true;
        }
    }

    return false;
}

void CollisionManager::RegisterTerrainAABB(const AABB& box)
{
    terrainBoxes.push_back(box);
}

void CollisionManager::ClearTerrainAABBs()
{
    terrainBoxes.clear();
}

SweepStatus CollisionManager::SweepTerrain(const AABB& movingBox, const DirectX::XMFLOAT3& displacement,
                                           TerrainSweepHit& hit) const
{
    hit = TerrainSweepHit{};

    std::vector<SweepHit> candidates;
    float earliestTime = 1.0f;

    // 1. すべて同じ開始位置・移動量で調べる
    for (const AABB& obstacle : terrainBoxes)
    {
        SweepHit candidate;

        const SweepStatus status = Collision::SweepAABB(movingBox, displacement, obstacle, candidate);

        // 初期めり込みは、通常の移動衝突とは別に扱う
        if (status == SweepStatus::InitialOverlap)
        {
            return SweepStatus::InitialOverlap;
        }

        if (status != SweepStatus::Hit)
        {
            continue;
        }

        earliestTime = (std::min)(earliestTime, candidate.time);

        candidates.push_back(candidate);
    }

    if (candidates.empty())
    {
        return SweepStatus::NoHit;
    }

    // 2. 最初の接触時刻を確定する
    hit.time = earliestTime;

    constexpr float TIME_TOLERANCE = 0.000001f;

    // 3. 最初の時刻とほぼ同時に当たる面を集める
    for (const SweepHit& candidate : candidates)
    {
        if (std::abs(candidate.time - earliestTime) > TIME_TOLERANCE)
        {
            continue;
        }

        for (int i = 0; i < candidate.normalCount; ++i)
        {
            const DirectX::XMFLOAT3& normal = candidate.normals[i];

            bool alreadyAdded = false;

            for (const DirectX::XMFLOAT3& existing : hit.normals)
            {
                if (existing.x == normal.x && existing.y == normal.y && existing.z == normal.z)
                {
                    alreadyAdded = true;
                    break;
                }
            }

            if (!alreadyAdded)
            {
                hit.normals.push_back(normal);
            }
        }
    }

    return SweepStatus::Hit;
}

bool CollisionManager::RayCast(const CollisionMesh* collisionMesh, const DirectX::XMFLOAT3& start,
                               const DirectX::XMFLOAT3& end, HitResult& hitResult)
{
    DirectX::XMVECTOR rayStart = DirectX::XMLoadFloat3(&start);
    DirectX::XMVECTOR rayEnd = DirectX::XMLoadFloat3(&end);
    DirectX::XMVECTOR rayVec = DirectX::XMVectorSubtract(rayEnd, rayStart);
    DirectX::XMVECTOR rayLength = DirectX::XMVector3Length(rayVec);

    // ワールド空間のレイの長さ
    float distance = DirectX::XMVectorGetX(rayLength);

    if (distance == 0.0f)
    {
        return false;
    }

    bool hit = false;
    DirectX::XMMATRIX w = DirectX::XMLoadFloat4x4(collisionMesh->transform);

    for (const Model::Mesh& mesh : collisionMesh->model->GetMeshes())
    {
        // レイをワールド空間からローカル空間へ変換
        DirectX::XMMATRIX g = DirectX::XMLoadFloat4x4(&mesh.node->globalTransform);
        DirectX::XMMATRIX transformMatrix = DirectX::XMMatrixMultiply(g, w);
        DirectX::XMMATRIX inverseTransform = DirectX::XMMatrixInverse(nullptr, transformMatrix);

        DirectX::XMVECTOR s = DirectX::XMVector3Transform(rayStart, inverseTransform);
        DirectX::XMVECTOR e = DirectX::XMVector3Transform(rayEnd, inverseTransform);
        DirectX::XMVECTOR se = DirectX::XMVectorSubtract(e, s);
        DirectX::XMVECTOR v = DirectX::XMVector3Normalize(se);
        DirectX::XMVECTOR lengthVector = DirectX::XMVector3Length(se);

        // レイの長さ
        float length = DirectX::XMVectorGetX(lengthVector);

        if (length <= 0.0f)
        {
            continue;
        }

        float neart = length;

#if 0
        // バウンディングボックスとの交差判定（高速化）
        DirectX::XMVECTOR bboxMin = DirectX::XMLoadFloat3(&mesh.boundingBox[0]);
        DirectX::XMVECTOR bboxMax = DirectX::XMLoadFloat3(&mesh.boundingBox[1]);
        DirectX::BoundingBox bbox;
        DirectX::BoundingBox::CreateFromPoints(bbox, bboxMin, bboxMax);

        float bboxDistance = length;

        if (!bbox.Intersects(s, v, bboxDistance))
        {
            // バウンディングボックスと交差しない場合はスキップ
            continue;
        }
#endif

        // 三角形（面）との交差判定
        const std::vector<Model::Vertex>& vertices = mesh.vertices;
        const std::vector<uint32_t>& indices = mesh.indices;

        bool hitMesh = false;
        DirectX::XMVECTOR hitPosition;
        DirectX::XMVECTOR hitNormal;

        for (size_t i = 0; i < indices.size(); i += 3)
        {
            // 三角形の頂点を抽出
            const Model::Vertex& a = vertices[indices[i]];
            const Model::Vertex& b = vertices[indices[i + 1]];
            const Model::Vertex& c = vertices[indices[i + 2]];

            DirectX::XMVECTOR aVector = DirectX::XMLoadFloat3(&a.position);
            DirectX::XMVECTOR bVector = DirectX::XMLoadFloat3(&b.position);
            DirectX::XMVECTOR cVector = DirectX::XMLoadFloat3(&c.position);

            // 三角形の三辺ベクトルを算出
            DirectX::XMVECTOR ab = DirectX::XMVectorSubtract(bVector, aVector);
            DirectX::XMVECTOR bc = DirectX::XMVectorSubtract(cVector, bVector);
            DirectX::XMVECTOR ca = DirectX::XMVectorSubtract(aVector, cVector);

            // 三角形の法線ベクトルを算出
            DirectX::XMVECTOR n = DirectX::XMVector3Cross(ab, bc);

            // 内積の結果がプラスならば裏向き
            DirectX::XMVECTOR dotVector = DirectX::XMVector3Dot(v, n);
            float dot = DirectX::XMVectorGetX(dotVector);

            if (dot >= 0)
            {
                continue;
            }

            // 三角形とレイの交差判定
            float dist = neart;

            if (!DirectX::TriangleTests::Intersects(s, v, aVector, bVector, cVector, dist))
            {
                continue;
            }

            if (dist >= neart)
            {
                continue;
            }

            neart = dist;
            hitPosition = DirectX::XMVectorAdd(s, DirectX::XMVectorScale(v, neart));
            hitNormal = n;

            hitMesh = true;
        }

        if (hitMesh)
        {
            // ローカル空間からワールド空間へ変換
            DirectX::XMVECTOR worldHitPosition = DirectX::XMVector3Transform(hitPosition, transformMatrix);
            DirectX::XMVECTOR worldHitVec = DirectX::XMVectorSubtract(worldHitPosition, rayStart);
            DirectX::XMVECTOR worldHitDistanceVector = DirectX::XMVector3Length(worldHitVec);
            float worldHitDistance = DirectX::XMVectorGetX(worldHitDistanceVector);

            // ヒット情報保存
            if (distance > worldHitDistance)
            {
                distance = worldHitDistance;

                DirectX::XMVECTOR worldNormal = XMVector3TransformNormal(hitNormal, transformMatrix);
                DirectX::XMStoreFloat3(&hitResult.position, worldHitPosition);
                DirectX::XMStoreFloat3(&hitResult.normal, DirectX::XMVector3Normalize(worldNormal));
                hitResult.distance = worldHitDistance;

                hit = true;
            }
        }
    }

    return hit;
}
