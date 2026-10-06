#include "ModelManager.h"
#include "LoadingProfile.h"
#include <memory>
#include <sstream>
#include <imgui.h>

#include "Framework.h"
#include "Graphics.h"
#include "Effect/EffectManager.h"
#include "Camera.h"
#include "GamePad.h"
#include "ImGuiRenderer.h"
#include "ModelViewerScene.h"
#include "GameScene.h"
#include "SceneManager.h"
#include "SceneTitle.h"
#include "SceneClear.h"
#include "SceneLoading.h"

// 垂直同期間隔設定
static const int syncInterval = 1;

// コンストラクタ
Framework::Framework(HWND hWnd)
	: hWnd(hWnd)
{
	// グラフィックス初期化
	Graphics::Instance().Initialize(hWnd);
	audio = std::make_unique<Audio>();
	EffectManager::Instance().Initialize();

	// IMGUI初期化
	ImGuiRenderer::Initialize(hWnd, Graphics::Instance().GetDevice(), Graphics::Instance().GetDeviceContext());

	// シーン初期化
	SceneManager::Instance().ChangeScene([]() { return std::make_shared<SceneTitle>(); });
}

// デストラクタ
Framework::~Framework()
{
	// IMGUI終了化
	SceneManager::Instance().Clear();
	ModelManager::Instance().Clear();
	EffectManager::Instance().Finalize();
	audio.reset();
	ImGuiRenderer::Finalize();
}

// 更新処理
void Framework::Update(float elapsedTime)
{
	GamePad::Instance().Update();

	// IMGUIフレーム開始処理	
	ImGuiRenderer::NewFrame();

	// シーン更新処理
	SceneManager::Instance().Update(elapsedTime);
	if (!SceneManager::Instance().IsBackgroundEditing())
        EffectManager::Instance().Update(elapsedTime);
}

// 描画処理
void Framework::Render(float elapsedTime)
{
	ID3D11DeviceContext* dc = Graphics::Instance().GetDeviceContext();

	// 画面クリア
	Graphics::Instance().Clear(0.5f, 0.5f, 0.5f, 1);

	// レンダーターゲット設定
	Graphics::Instance().SetRenderTargets();

	// シーン描画処理
	SceneManager::Instance().Render(elapsedTime);
	if (!SceneManager::Instance().IsBackgroundEditing())
        EffectManager::Instance().Render(Camera::Instance().GetView(), Camera::Instance().GetProjection());

	// シーンGUI描画処理
	SceneManager::Instance().DrawGUI();

	// シーン切り替えGUI
	SceneSelectGUI();
#if 0
	// IMGUIデモウインドウ描画（IMGUI機能テスト用）
	ImGui::ShowDemoWindow();
#endif
	// IMGUI描画
	ImGuiRenderer::Render(dc);

	// 画面表示
	Graphics::Instance().Present(syncInterval);
    if (LoadingProfile::Enabled() && LoadingProfile::gameReady && !LoadingProfile::firstGameFrame)
    {
        LoadingProfile::firstGameFrame = true;
        LoadingProfile::Record("Loading.to_first_game_present", LoadingProfile::Milliseconds(LoadingProfile::loadingStart));
        if (LoadingProfile::Automatic()) PostQuitMessage(0);
    }
    LoadingProfile::Flush();
}

template<class T>
void Framework::ChangeSceneButtonGUI(const char* name)
{
	if (ImGui::Button(name))
	{
		SceneManager::Instance().ChangeScene([]() {
			return std::make_shared<SceneLoading>([]() { return std::make_shared<T>(); });
		});
	}
}

// シーン切り替えGUI
void Framework::SceneSelectGUI()
{
	ImVec2 displaySize = ImGui::GetIO().DisplaySize;
	ImVec2 pos = ImGui::GetMainViewport()->GetWorkPos();
	float width = 210;
	float height = 490;
	ImGui::SetNextWindowPos(ImVec2(pos.x + displaySize.x - width - 10, pos.y + 10), ImGuiCond_Once);
	ImGui::SetNextWindowSize(ImVec2(width, height), ImGuiCond_Once);

	if (ImGui::Begin("Scene"))
	{
		ChangeSceneButtonGUI<SceneTitle>("Title");
		ChangeSceneButtonGUI<SceneClear>("Clear (preview)");
		ChangeSceneButtonGUI<ModelViewerScene>(u8"モデルビューア");
		ChangeSceneButtonGUI<GameScene>(u8"ゲーム");
	}
	ImGui::End();
}

// フレームレート計算
void Framework::CalculateFrameStats()
{
	// Code computes the average frames per second, and also the 
	// average time it takes to render one frame.  These stats 
	// are appended to the window caption bar.
	static int frames = 0;
	static float time_tlapsed = 0.0f;

	frames++;

	// Compute averages over one second period.
	if ((timer.TimeStamp() - time_tlapsed) >= 1.0f)
	{
		float fps = static_cast<float>(frames); // fps = frameCnt / 1
		float mspf = 1000.0f / fps;
		std::ostringstream outs;
		outs.precision(6);
		outs << "FPS : " << fps << " / " << "Frame Time : " << mspf << " (ms)";
		SetWindowTextA(hWnd, outs.str().c_str());

		// Reset for next average.
		frames = 0;
		time_tlapsed += 1.0f;
	}
}

// アプリケーションループ
int Framework::Run()
{
	MSG msg = {};

	while (WM_QUIT != msg.message)
	{
		if (PeekMessage(&msg, NULL, 0, 0, PM_REMOVE))
		{
			TranslateMessage(&msg);
			DispatchMessage(&msg);
		}
		else
		{
			timer.Tick();
			CalculateFrameStats();

			float elapsedTime = timer.TimeInterval();
			Update(elapsedTime);
			Render(elapsedTime);
		}
	}
	return static_cast<int>(msg.wParam);
}

// メッセージハンドラ
LRESULT CALLBACK Framework::HandleMessage(HWND hWnd, UINT msg, WPARAM wParam, LPARAM lParam)
{
	if (ImGuiRenderer::HandleMessage(hWnd, msg, wParam, lParam))
		return true;

	switch (msg)
	{
	case WM_PAINT:
	{
		PAINTSTRUCT ps;
		HDC hdc;
		hdc = BeginPaint(hWnd, &ps);
		EndPaint(hWnd, &ps);
		break;
	}
	case WM_DESTROY:
		PostQuitMessage(0);
		break;
	case WM_CREATE:
		break;
	case WM_KEYDOWN:
		if (wParam == VK_ESCAPE) PostMessage(hWnd, WM_CLOSE, 0, 0);
		break;
	case WM_ENTERSIZEMOVE:
		// WM_EXITSIZEMOVE is sent when the user grabs the resize bars.
		timer.Stop();
		break;
	case WM_EXITSIZEMOVE:
		// WM_EXITSIZEMOVE is sent when the user releases the resize bars.
		// Here we reset everything based on the new window dimensions.
		timer.Start();
		break;
	default:
		return DefWindowProc(hWnd, msg, wParam, lParam);
	}
	return 0;
}
