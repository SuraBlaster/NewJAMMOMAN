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

bool CollisionManager::Raycast(const DirectX::XMFLOAT3& start, const DirectX::XMFLOAT3& end, HitResult& hit_result)
{
	bool hit = false;
	hit_result.distance = FLT_MAX;
    for (const CollisionMesh* mesh : meshes)
    {
		HitResult temp_result;
        if (RayCast(mesh, start, end, temp_result))
        {
            if (temp_result.distance < hit_result.distance)
            {
				hit_result = temp_result;
			}
			hit = true;
        }
	}
    return hit;
}

bool CollisionManager::RayCast(const CollisionMesh* collision_mesh, const DirectX::XMFLOAT3& start, const DirectX::XMFLOAT3& end, HitResult& hit_result)
{
	DirectX::XMVECTOR RayStart = DirectX::XMLoadFloat3(&start);
	DirectX::XMVECTOR RayEnd = DirectX::XMLoadFloat3(&end);
	DirectX::XMVECTOR RayVec = DirectX::XMVectorSubtract(RayEnd, RayStart);
	DirectX::XMVECTOR RayLength = DirectX::XMVector3Length(RayVec);

	// ワールド空間のレイの長さ
	float distance = DirectX::XMVectorGetX(RayLength);
	if (distance == 0.0f) return false;

	bool hit = false;
	DirectX::XMMATRIX W = DirectX::XMLoadFloat4x4(collision_mesh->transform);
	for (const Model::Mesh& mesh : collision_mesh->model->GetMeshes())
	{
		// レイをワールド空間からローカル空間へ変換
		DirectX::XMMATRIX G = DirectX::XMLoadFloat4x4(&mesh.node->globalTransform);
		DirectX::XMMATRIX Transform = DirectX::XMMatrixMultiply(G, W);
		DirectX::XMMATRIX InverseTransform = DirectX::XMMatrixInverse(nullptr, Transform);

		DirectX::XMVECTOR S = DirectX::XMVector3Transform(RayStart, InverseTransform);
		DirectX::XMVECTOR E = DirectX::XMVector3Transform(RayEnd, InverseTransform);
		DirectX::XMVECTOR SE = DirectX::XMVectorSubtract(E, S);
		DirectX::XMVECTOR V = DirectX::XMVector3Normalize(SE);
		DirectX::XMVECTOR Length = DirectX::XMVector3Length(SE);

		// レイの長さ
		float length = DirectX::XMVectorGetX(Length);
		if (length <= 0.0f) continue;

		float neart = length;

#if 0
		// バウンディングボックスとの交差判定（高速化）
		DirectX::XMVECTOR BBoxMin = DirectX::XMLoadFloat3(&mesh.bounding_box[0]);
		DirectX::XMVECTOR BBoxMax = DirectX::XMLoadFloat3(&mesh.bounding_box[1]);
		DirectX::BoundingBox bbox;
		DirectX::BoundingBox::CreateFromPoints(bbox, BBoxMin, BBoxMax);

		float bbox_distance = length;
		if (!bbox.Intersects(S, V, bbox_distance))
		{
			// バウンディングボックスと交差しない場合はスキップ
			continue;
		}
#endif

		// 三角形（面）との交差判定
		const std::vector<Model::Vertex>& vertices = mesh.vertices;
		const std::vector<uint32_t>& indices = mesh.indices;

		bool hit_mesh = false;
		DirectX::XMVECTOR HitPosition;
		DirectX::XMVECTOR HitNormal;
		for (size_t i = 0; i < indices.size(); i += 3)
		{
			// 三角形の頂点を抽出
			const Model::Vertex& a = vertices[indices[i]];
			const Model::Vertex& b = vertices[indices[i + 1]];
			const Model::Vertex& c = vertices[indices[i + 2]];

			DirectX::XMVECTOR A = DirectX::XMLoadFloat3(&a.position);
			DirectX::XMVECTOR B = DirectX::XMLoadFloat3(&b.position);
			DirectX::XMVECTOR C = DirectX::XMLoadFloat3(&c.position);

			// 三角形の三辺ベクトルを算出
			DirectX::XMVECTOR AB = DirectX::XMVectorSubtract(B, A);
			DirectX::XMVECTOR BC = DirectX::XMVectorSubtract(C, B);
			DirectX::XMVECTOR CA = DirectX::XMVectorSubtract(A, C);

			// 三角形の法線ベクトルを算出		
			DirectX::XMVECTOR N = DirectX::XMVector3Cross(AB, BC);

			// 内積の結果がプラスならば裏向き
			DirectX::XMVECTOR Dot = DirectX::XMVector3Dot(V, N);
			float dot = DirectX::XMVectorGetX(Dot);
			if (dot >= 0) continue;

			// 三角形とレイの交差判定
			float dist = neart;
			if (!DirectX::TriangleTests::Intersects(S, V, A, B, C, dist))
				continue;

			if (dist >= neart) continue;

			neart = dist;
			HitPosition = DirectX::XMVectorAdd(S, DirectX::XMVectorScale(V, neart));
			HitNormal = N;

			hit_mesh = true;
		}
		if (hit_mesh)
		{
			// ローカル空間からワールド空間へ変換
			DirectX::XMVECTOR WorldHitPosition = DirectX::XMVector3Transform(HitPosition, Transform);
			DirectX::XMVECTOR WorldHitVec = DirectX::XMVectorSubtract(WorldHitPosition, RayStart);
			DirectX::XMVECTOR WorldHitDistance = DirectX::XMVector3Length(WorldHitVec);
			float world_hit_distance = DirectX::XMVectorGetX(WorldHitDistance);

			// ヒット情報保存
			if (distance > world_hit_distance)
			{
				distance = world_hit_distance;

				DirectX::XMVECTOR WorldNormal = XMVector3TransformNormal(HitNormal, Transform);
				DirectX::XMStoreFloat3(&hit_result.position, WorldHitPosition);
				DirectX::XMStoreFloat3(&hit_result.normal, DirectX::XMVector3Normalize(WorldNormal));
				hit_result.distance = world_hit_distance;

				hit = true;
			}
		}
	}
	return hit;
}
