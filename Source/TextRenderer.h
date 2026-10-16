#pragma once
#include <DirectXMath.h>

struct ImFont;

class TextRenderer
{
  public:

    // 初期化処理
    static bool Initialize(const char* fontPath, float fontSize = 32.0f);

    // 終了処理
    static void Finalize();

    static DirectX::XMFLOAT2 Measure(const char* text, float size = 0.0f);

    static void Draw(float x, float y, const char* text, float size = 0.0f,
                     const DirectX::XMFLOAT4& color = {1, 1, 1, 1});

    // DrawCentered の描画処理
    static void DrawCentered(float x, float y, const char* text, float size = 0.0f,
                             const DirectX::XMFLOAT4& color = {1, 1, 1, 1});

  private:

    // DrawText の描画処理
    static void DrawText(float x, float y, const char* text, float size, const DirectX::XMFLOAT4& color,
                         bool centered);
    static ImFont* font;
};
