#include "Bloom.hlsli"

// 頂点バッファを使わず、画面全体を覆う1枚の三角形を作る
VsOutput main(uint vertexId : SV_VertexID)
{
    VsOutput vout;
    vout.texcoord = float2((vertexId << 1) & 2, vertexId & 2);
    vout.position = float4(vout.texcoord * float2(2.0f, -2.0f) + float2(-1.0f, 1.0f), 0.0f, 1.0f);

    return vout;
}
