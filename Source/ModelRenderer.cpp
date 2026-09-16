#include <algorithm>
#include "Misc.h"
#include "GpuResourceUtils.h"
#include "ModelRenderer.h"
#include "BasicShader.h"
#include "ModelShader.h"

// コンストラクタ
ModelRenderer::ModelRenderer(ID3D11Device* device)
{
	// シーン用定数バッファ
	GpuResourceUtils::CreateConstantBuffer(
		device,
		sizeof(CbScene),
		sceneConstantBuffer.GetAddressOf());

	// スケルトン用定数バッファ
	GpuResourceUtils::CreateConstantBuffer(
		device,
		sizeof(CbSkeleton),
		skeletonConstantBuffer.GetAddressOf());

	GpuResourceUtils::CreateConstantBuffer(
		device,
		sizeof(CbTransparency),
		transparencyConstantBuffer.GetAddressOf());

	// シェーダー生成
	shaders[static_cast<int>(ShaderId::Basic)] = std::make_unique<BasicShader>(device);
	shaders[static_cast<int>(ShaderId::Model)] = std::make_unique<ModelShader>(device);
}

// 箱描画
void ModelRenderer::Draw(ShaderId shaderId, std::shared_ptr<Model> model)
{
	DrawInfo& drawInfo = drawInfos.emplace_back();
	drawInfo.shaderId = shaderId;
	drawInfo.model = model;
}

void ModelRenderer::Draw(ShaderId shaderId, std::shared_ptr<Model> model, const DirectX::XMFLOAT4& color, float alpha)
{
	DrawInfo& drawInfo = drawInfos.emplace_back();
	drawInfo.shaderId = shaderId;
	drawInfo.model = model;
	drawInfo.alpha = alpha;
	drawInfo.color = color;
}

// 描画実行
void ModelRenderer::Render(const RenderContext& rc)
{
	ID3D11DeviceContext* dc = rc.deviceContext;

	// シーン用定数バッファ更新
	{
		CbScene cbScene{};
		DirectX::XMMATRIX V = DirectX::XMLoadFloat4x4(&rc.camera->GetView());
		DirectX::XMMATRIX P = DirectX::XMLoadFloat4x4(&rc.camera->GetProjection());
		DirectX::XMStoreFloat4x4(&cbScene.viewProjection, V * P);
		const DirectionalLight& directionalLight = rc.lightManager->GetDirectionalLight();
		cbScene.lightDirection.x = directionalLight.direction.x;
		cbScene.lightDirection.y = directionalLight.direction.y;
		cbScene.lightDirection.z = directionalLight.direction.z;
		cbScene.lightColor.x = directionalLight.color.x;
		cbScene.lightColor.y = directionalLight.color.y;
		cbScene.lightColor.z = directionalLight.color.z;
		const DirectX::XMFLOAT3& eye = rc.camera->GetEye();
		cbScene.cameraPosition.x = eye.x;
		cbScene.cameraPosition.y = eye.y;
		cbScene.cameraPosition.z = eye.z;
		cbScene.skyColor = rc.lightManager->GetSkyColor();
		cbScene.groundColor = rc.lightManager->GetGroundColor();
		cbScene.hemisphereWeight = rc.lightManager->GetHemisphereWeight();
		dc->UpdateSubresource(sceneConstantBuffer.Get(), 0, 0, &cbScene, 0, 0);
	}

	// サンプラステート設定
	ID3D11SamplerState* samplerStates[] = { rc.renderState->GetSamplerState(SamplerState::LinearWrap) };
	dc->PSSetSamplers(0, _countof(samplerStates), samplerStates);

	// レンダーステート設定
	dc->OMSetDepthStencilState(rc.renderState->GetDepthStencilState(DepthState::TestAndWrite), 0);
	dc->RSSetState(rc.renderState->GetRasterizerState(RasterizerState::SolidCullBack));

	// メッシュ描画関数
	auto drawMesh = [&](const Model::Mesh& mesh, Shader* shader, const DirectX::XMFLOAT4& color, float alpha)
		{
			UINT stride = sizeof(Model::Vertex);
			UINT offset = 0;
			dc->IASetVertexBuffers(0, 1, mesh.vertexBuffer.GetAddressOf(), &stride, &offset);
			dc->IASetIndexBuffer(mesh.indexBuffer.Get(), DXGI_FORMAT_R32_UINT, 0);
			dc->IASetPrimitiveTopology(D3D11_PRIMITIVE_TOPOLOGY_TRIANGLELIST);

			// 透明度定数バッファ更新
			CbTransparency cbTransparency{};
			cbTransparency.alpha = alpha;
			cbTransparency.color = color;
			dc->UpdateSubresource(transparencyConstantBuffer.Get(), 0, nullptr, &cbTransparency, 0, 0);

			// スケルトン用定数バッファ更新
			CbSkeleton cbSkeleton{};
			if (mesh.bones.size() > 0)
			{
				for (size_t i = 0; i < mesh.bones.size(); ++i)
				{
					const Model::Bone& bone = mesh.bones.at(i);
					DirectX::XMMATRIX WorldTransform = DirectX::XMLoadFloat4x4(&bone.node->worldTransform);
					DirectX::XMMATRIX OffsetTransform = DirectX::XMLoadFloat4x4(&bone.offsetTransform);
					DirectX::XMMATRIX BoneTransform = OffsetTransform * WorldTransform;
					DirectX::XMStoreFloat4x4(&cbSkeleton.boneTransforms[i], BoneTransform);
				}
			}
			else
			{
				cbSkeleton.boneTransforms[0] = mesh.node->worldTransform;
			}
			dc->UpdateSubresource(skeletonConstantBuffer.Get(), 0, 0, &cbSkeleton, 0, 0);

			// 描画直前で定数バッファを確実にスロットへバインドする
			ID3D11Buffer* vsConstantBuffers[] = { skeletonConstantBuffer.Get(), sceneConstantBuffer.Get(), nullptr, transparencyConstantBuffer.Get() };
			ID3D11Buffer* psConstantBuffers[] = { sceneConstantBuffer.Get(), nullptr, transparencyConstantBuffer.Get() }; // PointLightは今回使用しないためnullptr
			dc->VSSetConstantBuffers(6, _countof(vsConstantBuffers), vsConstantBuffers);
			dc->PSSetConstantBuffers(7, _countof(psConstantBuffers), psConstantBuffers);

			shader->Update(rc, mesh);
			dc->DrawIndexed(static_cast<UINT>(mesh.indices.size()), 0, 0);
		};

	DirectX::XMVECTOR CameraPosition = DirectX::XMLoadFloat3(&rc.camera->GetEye());
	DirectX::XMVECTOR CameraFront = DirectX::XMLoadFloat3(&rc.camera->GetFront());

	dc->OMSetBlendState(rc.renderState->GetBlendState(BlendState::Opaque), nullptr, 0xFFFFFFFF);

	// 不透明描画処理
	for (DrawInfo& drawInfo : drawInfos)
	{
		Shader* shader = shaders[static_cast<int>(drawInfo.shaderId)].get();
		shader->Begin(rc);

		for (const Model::Mesh& mesh : drawInfo.model->GetMeshes())
		{
			bool isTransparent = (drawInfo.alpha < 1.0f) ||
				(mesh.material->alphaMode == Model::AlphaMode::Blend) ||
				(mesh.material->baseColor.w > 0.01f && mesh.material->baseColor.w < 0.99f);

			if (isTransparent)
			{
				TransparencyDrawInfo& info = transparencyDrawInfos.emplace_back();
				info.shaderId = drawInfo.shaderId;
				info.mesh = &mesh;
				info.alpha = drawInfo.alpha;
				info.color = drawInfo.color;

				DirectX::XMVECTOR Position = DirectX::XMVectorSet(
					mesh.node->worldTransform._41,
					mesh.node->worldTransform._42,
					mesh.node->worldTransform._43,
					0.0f);
				DirectX::XMVECTOR Vec = DirectX::XMVectorSubtract(Position, CameraPosition);
				info.distance = DirectX::XMVectorGetX(DirectX::XMVector3Dot(CameraFront, Vec));
				continue;
			}

			drawMesh(mesh, shader, drawInfo.color, drawInfo.alpha);
		}

		shader->End(rc);
	}
	drawInfos.clear();

	dc->OMSetBlendState(rc.renderState->GetBlendState(BlendState::Transparency), nullptr, 0xFFFFFFFF);

	std::sort(transparencyDrawInfos.begin(), transparencyDrawInfos.end(),
		[](const TransparencyDrawInfo& lhs, const TransparencyDrawInfo& rhs)
		{
			return lhs.distance > rhs.distance;
		});

	// 半透明描画処理
	for (const TransparencyDrawInfo& info : transparencyDrawInfos)
	{
		Shader* shader = shaders[static_cast<int>(info.shaderId)].get();
		shader->Begin(rc);
		drawMesh(*info.mesh, shader, info.color, info.alpha);
		shader->End(rc);
	}
	transparencyDrawInfos.clear();

	// 後処理（バインド解除）
	ID3D11Buffer* nullBuffers[] = { nullptr, nullptr, nullptr, nullptr };
	dc->VSSetConstantBuffers(6, 4, nullBuffers);
	dc->PSSetConstantBuffers(7, 3, nullBuffers);

	for (ID3D11SamplerState*& samplerState : samplerStates) { samplerState = nullptr; }
	dc->PSSetSamplers(0, _countof(samplerStates), samplerStates);
}