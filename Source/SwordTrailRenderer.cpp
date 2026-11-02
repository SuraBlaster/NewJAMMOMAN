#include <algorithm>
#include <cstring>
#include "Misc.h"
#include "GpuResourceUtils.h"
#include "SwordTrailRenderer.h"

namespace
{
    // 白く光る芯、緑の光、柄に近い側の淡い色
    const DirectX::XMFLOAT4 CORE_COLOR = {1.00f, 1.00f, 0.95f, 1.0f};
    const DirectX::XMFLOAT4 GLOW_COLOR = {0.08f, 1.00f, 0.38f, 1.0f};
    const DirectX::XMFLOAT4 ROOT_COLOR = {1.00f, 1.00f, 0.92f, 1.0f};

    constexpr float SPARK_DRAG = 3.0f;      // 散った粒がすぐ減速するように
    constexpr float SPARK_BUOYANCY = 0.8f;  // 光の粒がふわっと浮くように
    constexpr float SPARKS_PER_UNIT = 1.6f; // 刃先が動いた距離あたりの発生数
}

SwordTrailRenderer::SwordTrailRenderer(ID3D11Device* device)
{
    D3D11_INPUT_ELEMENT_DESC inputElementDesc[]{
        {"POSITION", 0, DXGI_FORMAT_R32G32B32_FLOAT, 0, D3D11_APPEND_ALIGNED_ELEMENT,
         D3D11_INPUT_PER_VERTEX_DATA, 0},
        {"TEXCOORD", 0, DXGI_FORMAT_R32G32_FLOAT, 0, D3D11_APPEND_ALIGNED_ELEMENT,
         D3D11_INPUT_PER_VERTEX_DATA, 0},
    };
    GpuResourceUtils::LoadVertexShader(device, "Data/Shader/SwordTrailVS.cso", inputElementDesc,
                                       _countof(inputElementDesc), inputLayout.GetAddressOf(),
                                       vertexShader.GetAddressOf());
    GpuResourceUtils::LoadPixelShader(device, "Data/Shader/SwordTrailPS.cso", pixelShader.GetAddressOf());

    D3D11_INPUT_ELEMENT_DESC sparkInputElementDesc[]{
        {"POSITION", 0, DXGI_FORMAT_R32G32B32_FLOAT, 0, D3D11_APPEND_ALIGNED_ELEMENT,
         D3D11_INPUT_PER_VERTEX_DATA, 0},
        {"TEXCOORD", 0, DXGI_FORMAT_R32G32_FLOAT, 0, D3D11_APPEND_ALIGNED_ELEMENT,
         D3D11_INPUT_PER_VERTEX_DATA, 0},
        {"ALPHA", 0, DXGI_FORMAT_R32_FLOAT, 0, D3D11_APPEND_ALIGNED_ELEMENT, D3D11_INPUT_PER_VERTEX_DATA, 0},
    };
    GpuResourceUtils::LoadVertexShader(device, "Data/Shader/SwordSparkVS.cso", sparkInputElementDesc,
                                       _countof(sparkInputElementDesc), sparkInputLayout.GetAddressOf(),
                                       sparkVertexShader.GetAddressOf());
    GpuResourceUtils::LoadPixelShader(device, "Data/Shader/SwordSparkPS.cso",
                                      sparkPixelShader.GetAddressOf());

    GpuResourceUtils::CreateConstantBuffer(device, sizeof(CbScene), sceneConstantBuffer.GetAddressOf());
    GpuResourceUtils::CreateConstantBuffer(device, sizeof(CbSwordTrail), trailConstantBuffer.GetAddressOf());

    D3D11_BUFFER_DESC desc{};
    desc.ByteWidth = sizeof(Vertex) * VERTEX_CAPACITY;
    desc.Usage = D3D11_USAGE_DYNAMIC;
    desc.BindFlags = D3D11_BIND_VERTEX_BUFFER;
    desc.CPUAccessFlags = D3D11_CPU_ACCESS_WRITE;
    HRESULT hr = device->CreateBuffer(&desc, nullptr, vertexBuffer.GetAddressOf());
    _ASSERT_EXPR(SUCCEEDED(hr), HRTrace(hr));

    desc.ByteWidth = sizeof(SparkVertex) * MAX_SPARKS * 6;
    hr = device->CreateBuffer(&desc, nullptr, sparkVertexBuffer.GetAddressOf());
    _ASSERT_EXPR(SUCCEEDED(hr), HRTrace(hr));

    vertices.reserve(VERTEX_CAPACITY);
    sparks.reserve(MAX_SPARKS);
    sparkVertices.reserve(MAX_SPARKS * 6);
}

float SwordTrailRenderer::Random(float min, float max)
{
    return std::uniform_real_distribution<float>(min, max)(randomEngine);
}

void SwordTrailRenderer::Update(float elapsedTime)
{
    time += elapsedTime;

    const float drag = (std::max)(0.0f, 1.0f - SPARK_DRAG * elapsedTime);

    for (Spark& spark : sparks)
    {
        spark.life -= elapsedTime;
        spark.velocity.x *= drag;
        spark.velocity.y = spark.velocity.y * drag + SPARK_BUOYANCY * elapsedTime;
        spark.velocity.z *= drag;
        spark.position.x += spark.velocity.x * elapsedTime;
        spark.position.y += spark.velocity.y * elapsedTime;
        spark.position.z += spark.velocity.z * elapsedTime;
    }

    sparks.erase(
        std::remove_if(sparks.begin(), sparks.end(), [](const Spark& spark) { return spark.life <= 0.0f; }),
        sparks.end());
}

void SwordTrailRenderer::SpawnSpark(const DirectX::XMFLOAT3& position, const DirectX::XMFLOAT3& velocity)
{
    if (sparks.size() >= MAX_SPARKS)
    {
        return;
    }

    Spark spark;
    spark.position = position;
    spark.velocity = velocity;
    spark.maxLife = Random(0.35f, 0.75f);
    spark.life = spark.maxLife;
    spark.size = Random(0.06f, 0.16f);
    spark.twinkle = Random(0.0f, 6.28f);
    sparks.push_back(spark);
}

void SwordTrailRenderer::EmitSwingSparks(const DirectX::XMFLOAT3& root, const DirectX::XMFLOAT3& tip,
                                         const DirectX::XMFLOAT3& previousRoot,
                                         const DirectX::XMFLOAT3& previousTip, float elapsedTime)
{
    using namespace DirectX;

    const XMVECTOR tipVector = XMLoadFloat3(&tip);
    const XMVECTOR previousTipVector = XMLoadFloat3(&previousTip);
    const XMVECTOR swing = XMVectorSubtract(tipVector, previousTipVector);
    const float swingLength = XMVectorGetX(XMVector3Length(swing));

    // 速く振るほど多く散らす
    sparkEmitAccumulator += swingLength * SPARKS_PER_UNIT;
    const int count = static_cast<int>(sparkEmitAccumulator);
    sparkEmitAccumulator -= static_cast<float>(count);

    const XMVECTOR swingVelocity =
        elapsedTime > 0.0f ? XMVectorScale(swing, 0.12f / elapsedTime) : XMVectorZero();

    for (int i = 0; i < count; i++)
    {
        // 前フレームと今フレームの刃の間の、刃先寄りの位置を選ぶ
        const float t = Random(0.0f, 1.0f);
        const float along = Random(0.55f, 1.0f);
        const XMVECTOR rootPoint = XMVectorLerp(XMLoadFloat3(&previousRoot), XMLoadFloat3(&root), t);
        const XMVECTOR tipPoint = XMVectorLerp(previousTipVector, tipVector, t);
        XMFLOAT3 position, velocity;
        XMStoreFloat3(&position, XMVectorLerp(rootPoint, tipPoint, along));
        XMStoreFloat3(&velocity,
                      XMVectorAdd(swingVelocity, XMVectorSet(Random(-0.6f, 0.6f), Random(-0.3f, 0.9f),
                                                             Random(-0.6f, 0.6f), 0.0f)));
        SpawnSpark(position, velocity);
    }
}

void SwordTrailRenderer::EmitBurst(const DirectX::XMFLOAT3& root, const DirectX::XMFLOAT3& tip, int count)
{
    using namespace DirectX;

    for (int i = 0; i < count; i++)
    {
        XMFLOAT3 position;
        XMStoreFloat3(&position, XMVectorLerp(XMLoadFloat3(&root), XMLoadFloat3(&tip), Random(0.45f, 1.0f)));
        SpawnSpark(position, {Random(-1.2f, 1.2f), Random(-0.4f, 1.4f), Random(-1.2f, 1.2f)});
    }
}

void SwordTrailRenderer::Begin(const Params& params)
{
    this->params = params;
    vertices.clear();
}

void SwordTrailRenderer::AddVertex(const DirectX::XMFLOAT3& position, const DirectX::XMFLOAT2& texcoord)
{
    // ストリップを分割して描くと継ぎ目で三角形が崩れるため、容量を超えた分は捨てる
    if (vertices.size() < VERTEX_CAPACITY)
    {
        vertices.push_back({position, texcoord});
    }
}

void SwordTrailRenderer::Render(ID3D11DeviceContext* dc, const DirectX::XMFLOAT4X4& view,
                                const DirectX::XMFLOAT4X4& projection)
{
    const bool hasTrail = vertices.size() >= 3;

    if (!hasTrail && sparks.empty())
    {
        vertices.clear();
        return;
    }

    CbScene cbScene;
    DirectX::XMStoreFloat4x4(&cbScene.viewProjection,
                             DirectX::XMLoadFloat4x4(&view) * DirectX::XMLoadFloat4x4(&projection));
    dc->UpdateSubresource(sceneConstantBuffer.Get(), 0, 0, &cbScene, 0, 0);

    CbSwordTrail cbTrail{};
    cbTrail.coreColor = CORE_COLOR;
    cbTrail.glowColor = GLOW_COLOR;
    cbTrail.rootColor = ROOT_COLOR;
    cbTrail.time = time;
    cbTrail.fade = params.fade;
    cbTrail.glowMargin = params.glowMargin;
    dc->UpdateSubresource(trailConstantBuffer.Get(), 0, 0, &cbTrail, 0, 0);

    dc->VSSetConstantBuffers(0, 1, sceneConstantBuffer.GetAddressOf());
    dc->PSSetConstantBuffers(0, 1, trailConstantBuffer.GetAddressOf());
    dc->IASetPrimitiveTopology(D3D11_PRIMITIVE_TOPOLOGY_TRIANGLESTRIP);
    dc->IASetIndexBuffer(nullptr, DXGI_FORMAT_R32_UINT, 0);

    if (hasTrail)
    {
        dc->VSSetShader(vertexShader.Get(), nullptr, 0);
        dc->PSSetShader(pixelShader.Get(), nullptr, 0);
        dc->IASetInputLayout(inputLayout.Get());

        D3D11_MAPPED_SUBRESOURCE mapped;
        HRESULT hr = dc->Map(vertexBuffer.Get(), 0, D3D11_MAP_WRITE_DISCARD, 0, &mapped);
        _ASSERT_EXPR(SUCCEEDED(hr), HRTrace(hr));
        memcpy(mapped.pData, vertices.data(), sizeof(Vertex) * vertices.size());
        dc->Unmap(vertexBuffer.Get(), 0);

        UINT stride = sizeof(Vertex);
        UINT offset = 0;
        dc->IASetVertexBuffers(0, 1, vertexBuffer.GetAddressOf(), &stride, &offset);
        dc->Draw(static_cast<UINT>(vertices.size()), 0);
    }

    vertices.clear();

    RenderSparks(dc, view);
}

void SwordTrailRenderer::RenderSparks(ID3D11DeviceContext* dc, const DirectX::XMFLOAT4X4& view)
{
    if (sparks.empty())
    {
        return;
    }

    // ビュー行列の列からカメラの右と上を取り出し、粒を常にカメラへ向ける
    const DirectX::XMFLOAT3 right = {view._11, view._21, view._31};
    const DirectX::XMFLOAT3 up = {view._12, view._22, view._32};

    sparkVertices.clear();

    for (const Spark& spark : sparks)
    {
        const float lifeRatio = spark.life / spark.maxLife;
        const float twinkle = 0.75f + 0.25f * sinf(time * 40.0f + spark.twinkle);
        const float alpha = powf(lifeRatio, 1.5f) * twinkle;
        const float size = spark.size * (0.6f + 0.4f * lifeRatio);

        const DirectX::XMFLOAT3 r = {right.x * size, right.y * size, right.z * size};
        const DirectX::XMFLOAT3 u = {up.x * size, up.y * size, up.z * size};
        const DirectX::XMFLOAT3& p = spark.position;
        const SparkVertex v0 = {{p.x - r.x - u.x, p.y - r.y - u.y, p.z - r.z - u.z}, {0.0f, 1.0f}, alpha};
        const SparkVertex v1 = {{p.x - r.x + u.x, p.y - r.y + u.y, p.z - r.z + u.z}, {0.0f, 0.0f}, alpha};
        const SparkVertex v2 = {{p.x + r.x + u.x, p.y + r.y + u.y, p.z + r.z + u.z}, {1.0f, 0.0f}, alpha};
        const SparkVertex v3 = {{p.x + r.x - u.x, p.y + r.y - u.y, p.z + r.z - u.z}, {1.0f, 1.0f}, alpha};
        sparkVertices.push_back(v0);
        sparkVertices.push_back(v1);
        sparkVertices.push_back(v2);
        sparkVertices.push_back(v0);
        sparkVertices.push_back(v2);
        sparkVertices.push_back(v3);
    }

    dc->VSSetShader(sparkVertexShader.Get(), nullptr, 0);
    dc->PSSetShader(sparkPixelShader.Get(), nullptr, 0);
    dc->IASetInputLayout(sparkInputLayout.Get());
    dc->IASetPrimitiveTopology(D3D11_PRIMITIVE_TOPOLOGY_TRIANGLELIST);

    D3D11_MAPPED_SUBRESOURCE mapped;
    HRESULT hr = dc->Map(sparkVertexBuffer.Get(), 0, D3D11_MAP_WRITE_DISCARD, 0, &mapped);
    _ASSERT_EXPR(SUCCEEDED(hr), HRTrace(hr));
    memcpy(mapped.pData, sparkVertices.data(), sizeof(SparkVertex) * sparkVertices.size());
    dc->Unmap(sparkVertexBuffer.Get(), 0);

    UINT stride = sizeof(SparkVertex);
    UINT offset = 0;
    dc->IASetVertexBuffers(0, 1, sparkVertexBuffer.GetAddressOf(), &stride, &offset);
    dc->Draw(static_cast<UINT>(sparkVertices.size()), 0);
}
