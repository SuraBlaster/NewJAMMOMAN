#include "ModelManager.h"
#include "LoadingProfile.h"
#include <memory>
#include <sstream>
#include <imgui.h>
#include <imgui_internal.h>
#include <algorithm>

#include "Framework.h"
#include "Graphics.h"
#include "Effect/EffectManager.h"
#include "Camera.h"
#include "InputManager.h"
#include "ImGuiRenderer.h"
#include "ModelViewerScene.h"
#include "GameScene.h"
#include "SceneManager.h"
#include "SceneTitle.h"
#include "SceneClear.h"
#include "SceneLoading.h"
#include "StylizedShading.h"

// 垂直同期間隔設定（性能計測のためSceneウインドウから切り替えられるようにしている）
static int syncInterval = 1;

#ifdef _DEBUG
// デバッグ用ウインドウがゲーム画面に重なって見づらいため、F1で隠し、F2でゲーム画面をパネル表示に切り替える
static bool showDebugGui = true;
static bool useEditorLayout = true;

static void UpdateDebugGuiToggle()
{
    ImGuiIO& io = ImGui::GetIO();

    if (io.WantTextInput)
    {
        return;
    }

    if (ImGui::IsKeyPressed(VK_F1, false))
    {
        showDebugGui = !showDebugGui;

        // 隠したウインドウがマウス操作を奪わないようにする
        if (showDebugGui)
        {
            io.ConfigFlags &= ~ImGuiConfigFlags_NoMouse;
        }
        else
        {
            io.ConfigFlags |= ImGuiConfigFlags_NoMouse;
        }
    }

    if (ImGui::IsKeyPressed(VK_F2, false))
    {
        useEditorLayout = !useEditorLayout;
    }
}

static void DrawEditorLayout()
{
    ImGuiViewport* viewport = ImGui::GetMainViewport();
    const ImGuiID dockspaceId = ImGui::DockSpaceOverViewport(viewport);

    // imgui.iniに配置が無いときだけ、左にゲーム画面、右にデバッグ用ウインドウを並べる
    static bool layoutChecked = false;

    if (!layoutChecked)
    {
        layoutChecked = true;
        ImGuiDockNode* node = ImGui::DockBuilderGetNode(dockspaceId);

        if (node == nullptr || !node->IsSplitNode())
        {
            ImGui::DockBuilderRemoveNode(dockspaceId);
            ImGui::DockBuilderAddNode(dockspaceId, ImGuiDockNodeFlags_DockSpace);
            ImGui::DockBuilderSetNodeSize(dockspaceId, viewport->Size);
            ImGuiID gameId = dockspaceId;
            const ImGuiID sideId =
                ImGui::DockBuilderSplitNode(gameId, ImGuiDir_Right, 0.28f, nullptr, &gameId);
            ImGui::DockBuilderDockWindow("Game", gameId);

            for (const char* name : {"Scene", "Debug", "Player", "Stylized Shading", "Bloom",
                                     "Background editor", "Distant City", "Background Fog"})
            {
                ImGui::DockBuilderDockWindow(name, sideId);
            }

            ImGui::DockBuilderFinish(dockspaceId);
        }
    }

    // ゲーム画面のパネルがキーボード操作を奪うと、タイトル画面などの入力が効かなくなるためNoNavにする
    ImGui::PushStyleVar(ImGuiStyleVar_WindowPadding, ImVec2(0.0f, 0.0f));

    if (ImGui::Begin("Game", nullptr,
                     ImGuiWindowFlags_NoScrollbar | ImGuiWindowFlags_NoScrollWithMouse |
                         ImGuiWindowFlags_NoNav))
    {
        // パネルの大きさに合わせ、縦横比を保ったまま中央に表示する
        const Graphics& graphics = Graphics::Instance();
        const float gameWidth = graphics.GetScreenWidth();
        const float gameHeight = graphics.GetScreenHeight();
        const ImVec2 available = ImGui::GetContentRegionAvail();
        const float scale = (std::min)(available.x / gameWidth, available.y / gameHeight);

        if (scale > 0.0f)
        {
            const ImVec2 size(gameWidth * scale, gameHeight * scale);
            const ImVec2 cursor = ImGui::GetCursorPos();
            ImGui::SetCursorPos(
                ImVec2(cursor.x + (available.x - size.x) * 0.5f, cursor.y + (available.y - size.y) * 0.5f));
            // HUDのスプライトがゲーム画面のアルファを書き換えるため、透けないよう不透明で描く
            ImDrawList* drawList = ImGui::GetWindowDrawList();
            drawList->AddCallback(
                [](const ImDrawList*, const ImDrawCmd*)
                {
                    Graphics& graphics = Graphics::Instance();
                    graphics.GetDeviceContext()->OMSetBlendState(
                        graphics.GetRenderState()->GetBlendState(BlendState::Opaque), nullptr, 0xFFFFFFFF);
                },
                nullptr);
            ImGui::Image(graphics.GetGameViewShaderResourceView(), size);
            drawList->AddCallback(ImDrawCallback_ResetRenderState, nullptr);
        }
    }

    ImGui::End();
    ImGui::PopStyleVar();
}
#endif

// コンストラクタ
Framework::Framework(HWND hWnd) : hWnd(hWnd)
{
    // グラフィックス初期化
    Graphics::Instance().Initialize(hWnd);
    InputManager::Instance().Initialize(hWnd);
    audio = std::make_unique<Audio>();
    EffectManager::Instance().Initialize();

    // IMGUI初期化
    ImGuiRenderer::Initialize(hWnd, Graphics::Instance().GetDevice(),
                              Graphics::Instance().GetDeviceContext());

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
    InputManager::Instance().GetGamePad().StopVibration();
    audio.reset();
    ImGuiRenderer::Finalize();
}

// 更新処理
void Framework::Update(float elapsedTime)
{
    InputManager::Instance().Update();

    // IMGUIフレーム開始処理
    ImGuiRenderer::NewFrame();

    // シーン更新処理
    SceneManager::Instance().Update(elapsedTime);

    if (!SceneManager::Instance().IsBackgroundEditing())
    {
        EffectManager::Instance().Update(elapsedTime);
    }
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
    {
        EffectManager::Instance().Render(Camera::Instance().GetView(), Camera::Instance().GetProjection());
    }

    Graphics& graphics = Graphics::Instance();
    bool editorLayout = false;
    bool hideDebugWindows = false;

#ifdef _DEBUG
    UpdateDebugGuiToggle();
    // 背景エディタはマウスで画面上を直接触るため、編集中はゲーム画面を全面表示に戻す
    editorLayout = showDebugGui && useEditorLayout && !SceneManager::Instance().IsBackgroundEditing();
    hideDebugWindows = !showDebugGui;
#endif

    // シーンにブルームをかけて書き出す。UIはにじませたくないのでこの後に描く
    ID3D11RenderTargetView* sceneOutput =
        editorLayout ? graphics.GetGameViewRenderTargetView() : graphics.GetBackBufferRenderTargetView();
    graphics.GetBloom()->Render(dc, graphics.GetSceneShaderResourceView(), sceneOutput,
                                graphics.GetViewport());
    graphics.SetBackBufferTarget();

#ifdef _DEBUG
    if (editorLayout)
    {
        const float clearColor[4] = {0.1f, 0.1f, 0.12f, 1.0f};
        dc->ClearRenderTargetView(graphics.GetBackBufferRenderTargetView(), clearColor);
        DrawEditorLayout();
    }
#endif

    // シーンGUI描画処理
    SceneManager::Instance().DrawGUI();

    // シーン切り替えGUI
    SceneSelectGUI();

#ifdef _DEBUG
    // スタイライズドシェーディング調整GUI
    StylizedShading::Instance().DrawGUI();
    graphics.GetBloom()->DrawGUI();
#endif
#if 0
    // IMGUIデモウインドウ描画（IMGUI機能テスト用）
    ImGui::ShowDemoWindow();
#endif
    // IMGUI描画
    ImGuiRenderer::Render(dc, editorLayout ? graphics.GetGameViewRenderTargetView() : nullptr,
                          graphics.GetBackBufferRenderTargetView(), hideDebugWindows);

    // 画面表示
    Graphics::Instance().Present(syncInterval);

    if (LoadingProfile::Enabled() && LoadingProfile::gameReady && !LoadingProfile::firstGameFrame)
    {
        LoadingProfile::firstGameFrame = true;
        LoadingProfile::Record("Loading.to_first_game_present",
                               LoadingProfile::Milliseconds(LoadingProfile::loadingStart));

        if (LoadingProfile::Automatic())
        {
            PostQuitMessage(0);
        }
    }

    LoadingProfile::Flush();
}

template <class T> void Framework::ChangeSceneButtonGUI(const char* name)
{
    if (ImGui::Button(name))
    {
        SceneManager::Instance().ChangeScene(
            []() { return std::make_shared<SceneLoading>([]() { return std::make_shared<T>(); }); });
    }
}

// シーン切り替えGUI
void Framework::SceneSelectGUI()
{
#ifdef _DEBUG
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

        bool vsync = syncInterval != 0;
        if (ImGui::Checkbox(u8"垂直同期", &vsync))
        {
            syncInterval = vsync ? 1 : 0;
        }
    }

    ImGui::End();

#endif
}

// フレームレート計算
void Framework::CalculateFrameStats()
{

    static int frames = 0;
    static float timeElapsed = 0.0f;

    frames++;

    if ((timer.TimeStamp() - timeElapsed) >= 1.0f)
    {
        float fps = static_cast<float>(frames);
        float mspf = 1000.0f / fps;
        std::ostringstream outs;
        outs.precision(6);
        outs << "FPS : " << fps << " / " << "Frame Time : " << mspf << " (ms)";
        SetWindowTextA(hWnd, outs.str().c_str());

        frames = 0;
        timeElapsed += 1.0f;
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
    if (msg == WM_MOUSEWHEEL)
    {
        InputManager::Instance().SetWheel(GET_WHEEL_DELTA_WPARAM(wParam));
    }

    if (ImGuiRenderer::HandleMessage(hWnd, msg, wParam, lParam))
    {
        return true;
    }

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
        if (wParam == VK_ESCAPE)
        {
            PostMessage(hWnd, WM_CLOSE, 0, 0);
        }

        break;
    case WM_ENTERSIZEMOVE:

        timer.Stop();
        break;
    case WM_EXITSIZEMOVE:

        timer.Start();
        break;
    default:
        return DefWindowProc(hWnd, msg, wParam, lParam);
    }

    return 0;
}
