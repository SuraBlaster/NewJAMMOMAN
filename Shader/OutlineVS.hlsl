#include "Skinning.hlsli"
#include "Model.hlsli"
#include "Outline.hlsli"

// –@ü•ûŒü‚É–c‚ç‚Ü‚¹‚½— –Ê‚ð•`‚«A‚Í‚Ýo‚µ‚½•”•ª‚ð—ÖŠsü‚É‚·‚é
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
	float3 world_normal = normalize(SkinningVector(normal, bone_weights, bone_indices));

	float4 clip_position = mul(position, cb_view_projection);
	float2 clip_normal = mul(float4(world_normal, 0.0f), cb_view_projection).xy;

	// w‚ðŠ|‚¯‚ÄAƒJƒƒ‰‚©‚ç‚Ì‹——£‚ÉŠÖŒW‚È‚­“¯‚¶‘¾‚³‚É‚·‚é
	float2 pixel_direction = clip_normal * cb_viewport_size;
	float length_in_pixels = length(pixel_direction);
	if (length_in_pixels > 1e-5f)
	{
		pixel_direction /= length_in_pixels;
		clip_position.xy += pixel_direction * (cb_outline_width * 2.0f / cb_viewport_size) * clip_position.w;
	}

	vout.vertex = clip_position;
	vout.texcoord = texcoord;
	vout.normal = world_normal;
	vout.position = position.xyz;

	return vout;
}
