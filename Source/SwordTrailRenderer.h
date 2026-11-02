#pragma once

#include <vector>
#include <random>
#include <wrl.h>
#include <d3d11.h>
#include <DirectXMath.h>

// 光学剣の軌跡と、そこから散る光の粒を描画する
class SwordTrailRenderer
{
  public:

    struct Params
    {
        float fade;       // 0:表示中 1:消えきった
        float glowMargin; // 刃先の外側ににじむ光の幅（刃の長さに対する割合）
    };

    SwordTrailRenderer(ID3D11Device* device);

    // 時間と光の粒を進める
    void Update(float elapsedTime);

    // 振っている間、刃先寄りの範囲から光の粒を散らす
    void EmitSwingSparks(const DirectX::XMFLOAT3& root, const DirectX::XMFLOAT3& tip,
                         const DirectX::XMFLOAT3& previousRoot, const DirectX::XMFLOAT3& previousTip,
                         float elapsedTime);

    // 刃の刃先寄りの範囲からまとめて光の粒を散らす
    void EmitBurst(const DirectX::XMFLOAT3& root, const DirectX::XMFLOAT3& tip, int count);

    // 軌跡の頂点を積み始める
    void Begin(const Params& params);

    // texcoord.x は根元0〜刃先1（1以上は刃先の外側）、texcoord.y は新しさ0〜古さ1
    void AddVertex(const DirectX::XMFLOAT3& position, const DirectX::XMFLOAT2& texcoord);

    // 積んだ軌跡と光の粒を描画する
    void Render(ID3D11DeviceContext* dc, const DirectX::XMFLOAT4X4& view,
                const DirectX::XMFLOAT4X4& projection);

  private:

    static const UINT VERTEX_CAPACITY = 4096;
    static const UINT MAX_SPARKS = 256;

    struct Vertex
    {
        DirectX::XMFLOAT3 position;
        DirectX::XMFLOAT2 texcoord;
    };

    struct SparkVertex
    {
        DirectX::XMFLOAT3 position;
        DirectX::XMFLOAT2 texcoord;
        float alpha;
    };

    struct Spark
    {
        DirectX::XMFLOAT3 position;
        DirectX::XMFLOAT3 velocity;
        float life;
        float maxLife;
        float size;
        float twinkle;
    };

    struct CbScene
    {
        DirectX::XMFLOAT4X4 viewProjection;
    };

    // Shader/SwordTrail.hlsli の CbSwordTrail と同じ並びにすること
    struct CbSwordTrail
    {
        DirectX::XMFLOAT4 coreColor;
        DirectX::XMFLOAT4 glowColor;
        DirectX::XMFLOAT4 rootColor;
        float time;
        float fade;
        float glowMargin;
        float padding;
    };

    void SpawnSpark(const DirectX::XMFLOAT3& position, const DirectX::XMFLOAT3& velocity);
    void RenderSparks(ID3D11DeviceContext* dc, const DirectX::XMFLOAT4X4& view);
    float Random(float min, float max);

    std::vector<Vertex> vertices;
    std::vector<Spark> sparks;
    std::vector<SparkVertex> sparkVertices;
    Params params{};
    float time = 0.0f;
    float sparkEmitAccumulator = 0.0f;
    std::mt19937 randomEngine{20261102};

    Microsoft::WRL::ComPtr<ID3D11VertexShader> vertexShader;
    Microsoft::WRL::ComPtr<ID3D11PixelShader> pixelShader;
    Microsoft::WRL::ComPtr<ID3D11InputLayout> inputLayout;
    Microsoft::WRL::ComPtr<ID3D11Buffer> vertexBuffer;
    Microsoft::WRL::ComPtr<ID3D11VertexShader> sparkVertexShader;
    Microsoft::WRL::ComPtr<ID3D11PixelShader> sparkPixelShader;
    Microsoft::WRL::ComPtr<ID3D11InputLayout> sparkInputLayout;
    Microsoft::WRL::ComPtr<ID3D11Buffer> sparkVertexBuffer;
    Microsoft::WRL::ComPtr<ID3D11Buffer> sceneConstantBuffer;
    Microsoft::WRL::ComPtr<ID3D11Buffer> trailConstantBuffer;
};
