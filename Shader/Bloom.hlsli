struct VsOutput
{
    float4 position : SV_POSITION;
    float2 texcoord : TEXCOORD;
};

cbuffer CbBloom : register(b0)
{
    float2 cbTexelSize; // 読み込むテクスチャの1ピクセルの大きさ
    float cbThreshold;
    float cbKnee;
    float cbIntensity;
    float3 cbPadding;
};

Texture2D sourceTexture : register(t0);
SamplerState linearSampler : register(s0);
