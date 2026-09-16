#include "Basic.hlsli"

cbuffer CbMesh : register(b0)
{
	//float4		materialColor;
};

cbuffer CbTransparency : register(b9)
{
    float4 cb_transparency_color;
    float  cb_transparency_alpha;
    float3 cb_transparency_padding;
};

Texture2D DiffuseMap		: register(t0);
SamplerState LinearSampler	: register(s0);

float4 main(VS_OUT pin) : SV_TARGET
{
    float4 color = DiffuseMap.Sample(LinearSampler, pin.texcoord);
    
    color.rgb *= cb_transparency_color.rgb;
    color.a   *= cb_transparency_color.a * cb_transparency_alpha;
    
	return color;
}
