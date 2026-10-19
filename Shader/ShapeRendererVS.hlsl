#include "ShapeRenderer.hlsli"

VsOutput main(float4 position : POSITION)
{
	VsOutput vout;
	vout.position = mul(position, worldViewProjection);
	vout.color = color;

	return vout;
}
