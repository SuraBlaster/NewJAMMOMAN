#ifndef OUTLINE_HLSLI
#define OUTLINE_HLSLI

cbuffer CbOutline : register(b1)
{
    float4 cb_outline_color;
    float  cb_outline_width;        // 線の太さ（ピクセル）
    float  cb_outline_albedo_mix;   // 0:線の色そのまま 1:モデルの色を暗くした色
    float2 cb_viewport_size;
};

#endif
