#include "Skinning.hlsli"
#include "Model.hlsli"
#include "Outline.hlsli"

// –@ü•ûŒü‚É–c‚ç‚Ü‚¹‚½— –Ê‚ð•`‚«A‚Í‚Ýo‚µ‚½•”•ª‚ð—ÖŠsü‚É‚·‚é
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
	float3 worldNormal = normalize(SkinningVector(normal, boneWeights, boneIndices));

	float4 clipPosition = mul(position, cbViewProjection);
	float2 clipNormal = mul(float4(worldNormal, 0.0f), cbViewProjection).xy;

	// w‚ðŠ|‚¯‚ÄAƒJƒƒ‰‚©‚ç‚Ì‹——£‚ÉŠÖŒW‚È‚­“¯‚¶‘¾‚³‚É‚·‚é
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
