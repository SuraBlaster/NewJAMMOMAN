#ifndef OUTLINE_HLSLI
#define OUTLINE_HLSLI

cbuffer CbOutline : register(b1)
{
    float4 cbOutlineColor;
    float cbOutlineWidth;     // 線の太さ（ピクセル）
    float cbOutlineAlbedoMix; // 0:線の色そのまま 1:モデルの色を暗くした色
    float2 cbViewportSize;
};

#endif
