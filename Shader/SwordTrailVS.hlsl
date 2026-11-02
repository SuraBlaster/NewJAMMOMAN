#include "SwordTrail.hlsli"

VsOutput main(VsInput vin)
{
    VsOutput vout;
    vout.position = mul(vin.position, viewProjection);
    vout.texcoord = vin.texcoord;

    return vout;
}
