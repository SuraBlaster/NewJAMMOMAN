#include "SwordSpark.hlsli"

SparkVsOutput main(SparkVsInput vin)
{
    SparkVsOutput vout;
    vout.position = mul(vin.position, viewProjection);
    vout.texcoord = vin.texcoord;
    vout.alpha = vin.alpha;

    return vout;
}
