struct VsInput
{
    float4 position : POSITION;
    float2 texcoord : TEXCOORD;
};

struct VsOutput
{
    float4 position : SV_POSITION;
    float2 texcoord : TEXCOORD;
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
    float cbFade;       // 0:表示中 1:消えきった
    float cbGlowMargin; // 刃先の外側ににじむ光の幅
    float cbPadding;
};
