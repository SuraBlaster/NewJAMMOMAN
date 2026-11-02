#include "Scene.hlsli"

struct VsOutput
{
    float4 vertex : SV_POSITION;
    float2 texcoord : TEXCOORD;
    float3 normal : NORMAL;
    float3 position : POSITION;
    float4 tangent : TANGENT;
};
