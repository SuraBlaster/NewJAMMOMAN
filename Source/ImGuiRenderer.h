#pragma once

#include <Windows.h>
#include <d3d11.h>

class ImGuiRenderer
{
public:
    // 初期化
    static void Initialize(HWND hWnd, ID3D11Device* device, ID3D11DeviceContext* dc);

    // 終了化
    static void Finalize();

    // フレーム開始処理
    static void NewFrame();

    // 描画実行
    // hudTargetを指定すると、HUDだけをそこへ描いてからmainTargetへGUIを描く
    // hideWindowsがtrueのときは、デバッグ用のウインドウを描かずHUDだけを描く
    static void Render(ID3D11DeviceContext* context, ID3D11RenderTargetView* hudTarget = nullptr,
                       ID3D11RenderTargetView* mainTarget = nullptr, bool hideWindows = false);

    // WIN32メッセージハンドラー
    static LRESULT HandleMessage(HWND hWnd, UINT msg, WPARAM wParam, LPARAM lParam);
};
