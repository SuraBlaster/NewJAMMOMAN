#pragma once
#include <DirectXMath.h>

struct ImFont;

// UTF-8 text in screen pixels. Call Draw during a scene's DrawGUI().
class TextRenderer
{
public:
    // Call once after ImGui context creation, before the first frame.
    static bool Initialize(const char* fontPath, float fontSize = 32.0f);
    static void Finalize();

    static DirectX::XMFLOAT2 Measure(const char* text, float size = 0.0f);

    // size = 0 uses the size specified in Initialize. Color components are 0..1.
    static void Draw(float x, float y, const char* text, float size = 0.0f,
        const DirectX::XMFLOAT4& color = {1, 1, 1, 1});

    // x is the horizontal center; y is the top of the text.
    static void DrawCentered(float x, float y, const char* text, float size = 0.0f,
        const DirectX::XMFLOAT4& color = {1, 1, 1, 1});

private:
    static void DrawText(float x, float y, const char* text, float size,
        const DirectX::XMFLOAT4& color, bool centered);
    static ImFont* font; // Owned by ImGui's font atlas.
};
