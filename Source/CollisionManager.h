#pragma once

#include <vector>
#include <DirectXMath.h>

class Model;

struct HitResult
{
	DirectX::XMFLOAT3	position;
	DirectX::XMFLOAT3	normal;
	float				distance;
};

struct CollisionMesh
{
	const Model*				model = nullptr;
	const DirectX::XMFLOAT4X4*	transform = nullptr;
};


class CollisionManager
{
private:
	CollisionManager() = default;
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

	bool Raycast(const DirectX::XMFLOAT3& start, const DirectX::XMFLOAT3& end, HitResult& hit_result);


private:
	bool RayCast(const CollisionMesh* mesh, const DirectX::XMFLOAT3& start, const DirectX::XMFLOAT3& end, HitResult& hit_result);
private:
	std::vector<CollisionMesh*> meshes;
};