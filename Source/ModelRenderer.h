#pragma once

#include <memory>
#include <vector>
#include <wrl.h>
#include <d3d11.h>
#include <DirectXMath.h>
#include "Model.h"
#include "Shader.h"

enum class ShaderId
{
	Basic,
	Model,

	EnumCount
};

class ModelRenderer
{
public:
	ModelRenderer(ID3D11Device* device);
	~ModelRenderer() {}

	// 箱描画
	void Draw(ShaderId shaderId, std::shared_ptr<Model> model);

	// 色指定あり、透明度あり
	void Draw(ShaderId shaderId, std::shared_ptr<Model> model, const DirectX::XMFLOAT4& color, float alpha);

	// 描画実行
	void Render(const RenderContext& rc);

private:
	struct CbScene
	{
		DirectX::XMFLOAT4X4		viewProjection;
		DirectX::XMFLOAT4		lightDirection;
		DirectX::XMFLOAT4		lightColor;
		DirectX::XMFLOAT4		cameraPosition;
		DirectX::XMFLOAT3		skyColor;
		float					hemisphereWeight;
		DirectX::XMFLOAT3		groundColor;
		float					padding;
	};

	struct CbSkeleton
	{
		DirectX::XMFLOAT4X4		boneTransforms[256];
	};

	struct DrawInfo
	{
		ShaderId				shaderId;
		std::shared_ptr<Model>	model;
		float alpha = 1.0f;
		DirectX::XMFLOAT4 color = { 1.0f, 1.0f, 1.0f, 1.0f };
	};

	struct TransparencyDrawInfo
	{
		ShaderId				shaderId;
		const Model::Mesh*		mesh;
		float					distance;
		float alpha = 1.0f;
		DirectX::XMFLOAT4 color = { 1.0f, 1.0f, 1.0f, 1.0f };
	};

	struct CbTransparency
	{
		DirectX::XMFLOAT4 color;
		float alpha;
		DirectX::XMFLOAT3 padding;
	};

	std::unique_ptr<Shader>					shaders[static_cast<int>(ShaderId::EnumCount)];
	std::vector<DrawInfo>					drawInfos;
	std::vector<TransparencyDrawInfo>		transparencyDrawInfos;

	Microsoft::WRL::ComPtr<ID3D11Buffer>	sceneConstantBuffer;
	Microsoft::WRL::ComPtr<ID3D11Buffer>	skeletonConstantBuffer;
	Microsoft::WRL::ComPtr<ID3D11Buffer>	transparencyConstantBuffer;
};
