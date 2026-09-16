#include "Skinning.hlsli"
#include "Model.hlsli"

VS_OUT main(
	float4 position		: POSITION,
	float4 bone_weights	: BONE_WEIGHTS,
	uint4  bone_indices	: BONE_INDICES,
	float2 texcoord		: TEXCOORD,
	float3 normal		: NORMAL,
	float4 tangent		: TANGENT)
{
	VS_OUT vout = (VS_OUT)0;

	position = SkinningPosition(position, bone_weights, bone_indices);
	vout.vertex = mul(position, cb_view_projection);
	vout.texcoord = texcoord;
	vout.normal = SkinningVector(normal, bone_weights, bone_indices);
	vout.tangent.xyz = SkinningVector(tangent.xyz, bone_weights, bone_indices);
    vout.tangent.w = tangent.w;

	return vout;
}
