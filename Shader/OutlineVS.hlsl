#include "Skinning.hlsli"
#include "Model.hlsli"
#include "Outline.hlsli"

// 法線方向に膨らませた裏面を描き、はみ出した部分を輪郭線にする
VsOutput main(float4 position : POSITION, float4 boneWeights : BONE_WEIGHTS, uint4 boneIndices : BONE_INDICES,
              float2 texcoord : TEXCOORD, float3 normal : NORMAL, float4 tangent : TANGENT)
{
    VsOutput vout = (VsOutput)0;

    position = SkinningPosition(position, boneWeights, boneIndices);
    float3 worldNormal = normalize(SkinningVector(normal, boneWeights, boneIndices));

    float4 clipPosition = mul(position, cbViewProjection);
    float2 clipNormal = mul(float4(worldNormal, 0.0f), cbViewProjection).xy;

    // wを掛けて、カメラからの距離に関係なく同じ太さにする
    float2 pixelDirection = clipNormal * cbViewportSize;
    float lengthInPixels = length(pixelDirection);
    if (lengthInPixels > 1e-5f)
    {
        pixelDirection /= lengthInPixels;
        clipPosition.xy += pixelDirection * (cbOutlineWidth * 2.0f / cbViewportSize) * clipPosition.w;
    }

    vout.vertex = clipPosition;
    vout.texcoord = texcoord;
    vout.normal = worldNormal;
    vout.position = position.xyz;

    return vout;
}
