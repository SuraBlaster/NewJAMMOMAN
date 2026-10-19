cbuffer CbScene : register(b7)
{
	row_major float4x4	cbViewProjection;
	float4				cbLightDirection;
	float4				cbLightColor;
	float4				cbCameraPosition;
    float3				cbSkyColor;
    float				cbHemisphereWeight;
    float3				cbGroundColor;
};
