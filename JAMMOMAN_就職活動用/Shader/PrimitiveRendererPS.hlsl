#include "PrimitiveRenderer.hlsli"

float4 main(VsOutput pin) : SV_TARGET
{
    return pin.color;
}
