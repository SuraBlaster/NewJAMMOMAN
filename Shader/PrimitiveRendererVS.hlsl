#include "PrimitiveRenderer.hlsli"

VsOutput main(VsInput vin)
{
	VsOutput vout;
	vout.position = mul(vin.position, viewProjection);
	vout.color = vin.color;
	vout.texcoord = vin.texcoord;

	return vout;
}
