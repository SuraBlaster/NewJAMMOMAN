struct SparkVsInput
{
    float4 position : POSITION;
    float2 texcoord : TEXCOORD;
    float alpha : ALPHA;
};

struct SparkVsOutput
{
    float4 position : SV_POSITION;
    float2 texcoord : TEXCOORD;
    float alpha : ALPHA;
};

cbuffer CbScene : register(b0)
{
    row_major float4x4 viewProjection;
};

cbuffer CbSwordTrail : register(b0)
{
    float4 cbCoreColor;
    float4 cbGlowColor;
    float4 cbRootColor;
    float cbTime;
    float cbFade;
    float cbGlowMargin;
    float cbPadding;
};
