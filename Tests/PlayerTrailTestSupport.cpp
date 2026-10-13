// GPU-free dependencies. This test does not validate model geometry copying.
#include "Model.h"
#include "PrimitiveRenderer.h"
#include "ModelRenderer.h"
#include <cstdlib>

Model::Model(ID3D11Device*, const char*, float) {}
Model::Model(const Model&) {}
void Model::UpdateTransform(const DirectX::XMFLOAT4X4&) { std::abort(); }
void ModelRenderer::Draw(ShaderId, std::shared_ptr<Model>, const DirectX::XMFLOAT4&, float)
{
    std::abort();
}
void PrimitiveRenderer::AddVertex(const DirectX::XMFLOAT3&, const DirectX::XMFLOAT4&,
    const DirectX::XMFLOAT2&)
{
    std::abort(); // Rendering is outside this test.
}
