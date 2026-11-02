#include "Sprite.hlsli"

// 頂点シェーダーエントリポイント
VsOutput main(float4 position : POSITION, float4 color : COLOR, float2 texcoord : TEXCOORD)
{
    VsOutput vout;
    vout.position = position;
    vout.color = color;
    vout.texcoord = texcoord;

    return vout;
}
