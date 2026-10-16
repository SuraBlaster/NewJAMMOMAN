#include "Skinning.hlsli"
#include "Basic.hlsli"

VsOutput main(float4 position : POSITION, float4 boneWeights : BONE_WEIGHTS, uint4 boneIndices : BONE_INDICES,
              float2 texcoord : TEXCOORD)
{
    VsOutput vout = (VsOutput)0;

    position = SkinningPosition(position, boneWeights, boneIndices);
    vout.vertex = mul(position, cbViewProjection);
    vout.texcoord = texcoord;

    return vout;
}
