#include "Skinning.hlsli"
#include "Model.hlsli"

VsOutput main(
	float4 position		: POSITION,
	float4 boneWeights	: BONE_WEIGHTS,
	uint4  boneIndices	: BONE_INDICES,
	float2 texcoord		: TEXCOORD,
	float3 normal		: NORMAL,
	float4 tangent		: TANGENT)
{
	VsOutput vout = (VsOutput)0;

	position = SkinningPosition(position, boneWeights, boneIndices);
	vout.vertex = mul(position, cbViewProjection);
	vout.texcoord = texcoord;
	vout.normal = SkinningVector(normal, boneWeights, boneIndices);
	vout.tangent.xyz = SkinningVector(tangent.xyz, boneWeights, boneIndices);
    vout.tangent.w = tangent.w;

	return vout;
}
