#include <windows.h>
#include <DirectXTex.h>
#include <imgui.h>
#include <cassert>
#include <fstream>
#include <filesystem>
#include <cstdio>
#include <cmath>
#include <chrono>
#include "BackgroundEditor.h"
#include "LaboratoryAssets.h"
#include "Graphics.h"
#include "ImGuiRenderer.h"
#include "RenderContext.h"
#include "ModelManager.h"

int main()
{
    CoInitializeEx(nullptr,COINIT_MULTITHREADED);
    WNDCLASSW wc={};wc.lpfnWndProc=DefWindowProcW;wc.hInstance=GetModuleHandleW(nullptr);wc.lpszClassName=L"BackgroundEditorTests";
    RegisterClassW(&wc);
    HWND window=CreateWindowW(wc.lpszClassName,L"Background editor tests",WS_POPUP,0,0,1280,720,nullptr,nullptr,wc.hInstance,nullptr);
    auto& g=Graphics::Instance();g.Initialize(window);
    ImGuiRenderer::Initialize(window,g.GetDevice(),g.GetDeviceContext());
    ImGui::GetIO().ConfigFlags&=~ImGuiConfigFlags_ViewportsEnable;
    ImGui::GetIO().IniFilename=nullptr;
    const char* file="obj/background-editor-test.json";
    std::filesystem::remove(file);
    {
        BackgroundEditor scene(g.GetDevice());
        const auto start=std::chrono::steady_clock::now();
        for(int i=0;i<120;++i)assert(!scene.IsDirty());
        const auto ms=std::chrono::duration<double,std::milli>(std::chrono::steady_clock::now()-start).count();
        printf("PERF: %zu saved objects, unchanged dirty check %.4f ms/frame\n",scene.Objects().size(),ms/120);
        auto& camera=Camera::Instance();
        camera.SetPerspectiveFov(DirectX::XMConvertToRadians(40),1280.0f/720,0.1f,1000);
        camera.SetLookAt({3,3,-13},{3,1.2f,2},{0,1,0});
        g.Clear(0.06f,0.08f,0.1f,1);g.SetRenderTargets();
        scene.Render(g.GetModelRenderer());
        printf("PERF: start view submits %zu / %zu background objects\n",scene.VisibleObjectCount(),scene.Objects().size());
        RenderContext rc;rc.deviceContext=g.GetDeviceContext();rc.renderState=g.GetRenderState();rc.camera=&camera;rc.lightManager=&LightManager::Instance();
        g.GetModelRenderer()->Render(rc);
    }
    {
        // Stress an extended stage without writing to the user's saved layout.
        const char* stressFile="obj/background-stress-test.json";
        {std::ofstream out(stressFile);out<<"{\"formatVersion\":1,\"objects\":[";
            for(int i=0;i<2000;++i)
            {if(i)out<<',';out<<"{\"id\":"<<i+1<<",\"asset\":0,\"position\":["<<i*2<<",0,10],\"rotation\":[0,0,0],\"scale\":[1,1,1]}";}
            out<<"]}";}
        BackgroundEditor stress(g.GetDevice(),stressFile);
        assert(stress.Objects().size()==2000&&!stress.IsDirty());
        stress.Render(g.GetModelRenderer());
        assert(stress.VisibleObjectCount()>0&&stress.VisibleObjectCount()<30);
        printf("PERF: extended stage submits %zu / 2000 objects\n",stress.VisibleObjectCount());
        RenderContext rc;rc.deviceContext=g.GetDeviceContext();rc.renderState=g.GetRenderState();rc.camera=&Camera::Instance();rc.lightManager=&LightManager::Instance();
        g.GetModelRenderer()->Render(rc);
        auto& camera=Camera::Instance();camera.SetLookAt({2000,3,-13},{2000,1.2f,2},{0,1,0});
        stress.Render(g.GetModelRenderer());assert(stress.VisibleObjectCount()>0&&stress.VisibleObjectCount()<30);
        g.GetModelRenderer()->Render(rc);
        camera.SetLookAt({3,3,-13},{3,1.2f,2},{0,1,0});
        std::filesystem::remove(stressFile);
    }
    {
        BackgroundEditor editor(g.GetDevice(),file);
        assert(editor.Objects().empty()&&!editor.IsDirty());
        assert(editor.PatternCount()>=16);
        // Each insertion is one command; selected parts keep their relative layout.
        for(int pattern=0;pattern<static_cast<int>(editor.PatternCount());++pattern)
        {
            assert(editor.AddPattern(pattern,{0,-1.2f,10.2f}));
            const auto count=editor.Objects().size();
            assert(count>=1&&editor.Selections().size()==count);
            const auto first=editor.Objects().front();
            editor.Undo();assert(editor.Objects().empty());
            editor.Redo();assert(editor.Objects().size()==count&&editor.Selections().size()==count);
            assert(editor.AddPattern(pattern,{20,-1.2f,10.2f}));
            assert(editor.Objects().size()==count*2);
            assert(editor.Objects()[count].id!=first.id);
            assert(std::abs(editor.Objects()[count].position.x-first.position.x-20)<0.001f);
            editor.Undo();assert(editor.Objects().size()==count);
            editor.Undo();assert(editor.Objects().empty());
        }
        assert(!editor.AddPattern(-1,{0,0,0}));
        assert(!editor.AddPattern(0,{0,0,0},0));
        assert(editor.Objects().empty());
        // Cull offscreen objects, but keep a large object whose center is outside.
        editor.Add(0,{10000,0,10});
        editor.Add(0,{50,0,10});
        auto wide=editor.Objects().back();wide.scale={120,3,1};wide.rotation.z=15;
        editor.Transform(1,wide);
        editor.Render(g.GetModelRenderer());assert(editor.VisibleObjectCount()==1);
        RenderContext cullRc;cullRc.deviceContext=g.GetDeviceContext();cullRc.renderState=g.GetRenderState();cullRc.camera=&Camera::Instance();cullRc.lightManager=&LightManager::Instance();
        g.GetModelRenderer()->Render(cullRc);
        editor.Undo();editor.Undo();editor.Undo();assert(editor.Objects().empty()&&!editor.IsDirty());
        editor.Add(0,{0,1,2});editor.Add(1,{2,0,2});
        assert(editor.Objects().size()==2 && editor.IsDirty());
        editor.Duplicate();assert(editor.Objects().size()==3);
        assert(editor.Objects()[1].id!=editor.Objects()[2].id);
        assert(editor.Objects()[2].position.x==3);
        auto change=editor.Objects()[2];change.position={5,3,4};change.rotation={0,45,0};change.scale={2,1,1};
        editor.Transform(2,change);
        editor.Undo();assert(editor.Objects()[2].position.x==3);
        editor.Redo();assert(editor.Objects()[2].position.x==5);
        editor.Delete();assert(editor.Objects().size()==2);
        editor.Undo();assert(editor.Objects().size()==3&&editor.Objects()[2].position.x==5);
        editor.Undo();assert(editor.Objects()[2].position.x==3);
        editor.Undo();assert(editor.Objects().size()==2);
        editor.Add(2,{-1,0,2});editor.Redo();assert(editor.Objects().size()==3); // redo branch invalidated
        assert(editor.Save()&&!editor.IsDirty());
        editor.Delete();assert(editor.IsDirty());editor.Undo();assert(!editor.IsDirty());
        BackgroundEditor reloaded(g.GetDevice(),file);
        assert(reloaded.Objects().size()==3&&reloaded.Objects()[1].asset==1);
        editor.Add(0,{7,2,3});assert(editor.Load()&&editor.Objects().size()==3);
        editor.Undo();assert(editor.Objects().size()==4&&editor.IsDirty());
        editor.Redo();assert(editor.Objects().size()==3&&!editor.IsDirty());
        // Invalid documents must not clear the scene or add a history entry.
        {std::ofstream out(file);out<<"{\"formatVersion\":1,\"objects\":[{\"id\":0}]}";}
        assert(!editor.Load()&&editor.Objects().size()==3);
        assert(editor.Save());
        auto invalid=editor.Objects()[0];invalid.scale.x=0;editor.Transform(0,invalid);
        assert(editor.Objects()[0].scale.x==1);
        // Same ID, different asset after load and undo must bind the right model.
        {std::ofstream out(file);out<<"{\"formatVersion\":1,\"objects\":[{\"id\":1,\"asset\":1,\"position\":[0,0,2],\"rotation\":[0,0,0],\"scale\":[1,1,1]}]}";}
        assert(editor.Load()&&editor.Objects()[0].asset==1);editor.Undo();assert(editor.Objects()[0].asset==0);
        auto& camera=Camera::Instance();
        camera.SetPerspectiveFov(DirectX::XMConvertToRadians(45),1280.0f/720,0.1f,1000);
        camera.SetLookAt({5,4,-9},{1,1,2},{0,1,0});
        editor.SelectMany({0,1});
        const auto original=editor.Objects();
        editor.Duplicate();
        assert(editor.Objects().size()==5&&editor.Selections().size()==2);
        assert(editor.Objects()[3].id!=original[0].id&&editor.Objects()[4].id!=original[1].id);
        assert(editor.Objects()[4].position.x-editor.Objects()[3].position.x==original[1].position.x-original[0].position.x);
        editor.MoveSelection({10,2,4});
        assert(editor.Objects()[3].position.x==original[0].position.x+11);
        assert(editor.Objects()[4].position.z==original[1].position.z+4);
        editor.Undo();assert(editor.Objects()[3].position.x==original[0].position.x+1);
        editor.Undo();assert(editor.Objects().size()==3&&editor.Selections()==std::vector<int>({0,1}));
        editor.Redo();assert(editor.Objects().size()==5&&editor.Selections()==std::vector<int>({3,4}));
        editor.Delete();assert(editor.Objects().size()==3&&editor.Selections().empty());
        editor.Undo();assert(editor.Objects().size()==5&&editor.Selections().size()==2);
        editor.Undo();assert(editor.Objects().size()==3);
        editor.Select(2,true);assert(editor.Selections().size()==3);
        editor.Select(2,true);assert(editor.Selections().size()==2);
        editor.SetEditing(true);editor.Select(1);
        for(int frame=0;frame<3;frame++)
        {
            ImGuiRenderer::NewFrame();
            if(frame==0)
            {
                const auto p=ImGui::GetMainViewport()->Pos;
                editor.SelectRectangle(p.x+1280,p.y+720,p.x,p.y,false);
                assert(editor.Selections().size()==3);
                editor.SelectRectangle(p.x-100,p.y-100,p.x-50,p.y-50,true);
                assert(editor.Selections().size()==3);
            }
            assert(editor.Update(1.0f/60));
            g.Clear(0.06f,0.08f,0.1f,1);g.SetRenderTargets();
            editor.Render(g.GetModelRenderer());
            RenderContext rc;rc.deviceContext=g.GetDeviceContext();rc.renderState=g.GetRenderState();rc.camera=&camera;rc.lightManager=&LightManager::Instance();
            g.GetModelRenderer()->Render(rc);
            ImGuiRenderer::Render(g.GetDeviceContext());
        }
        Microsoft::WRL::ComPtr<ID3D11RenderTargetView> target;
        g.GetDeviceContext()->OMGetRenderTargets(1,target.GetAddressOf(),nullptr);
        Microsoft::WRL::ComPtr<ID3D11Resource> resource;target->GetResource(resource.GetAddressOf());
        DirectX::ScratchImage image;
        assert(SUCCEEDED(DirectX::CaptureTexture(g.GetDevice(),g.GetDeviceContext(),resource.Get(),image)));
        assert(SUCCEEDED(DirectX::SaveToWICFile(*image.GetImage(0,0,0),DirectX::WIC_FLAGS_NONE,DirectX::GetWICCodec(DirectX::WIC_CODEC_PNG),L"obj/background-editor-preview.png")));
        camera.SetLookAt({20,4,-9},{20,1,2},{0,1,0});
        editor.SetEditing(false);assert(camera.GetEye().x==5);
        // All appended assets load, retain their IDs through save/reload, and render.
        for(int asset=3;asset<LaboratoryAssets::Count;++asset)
            editor.Add(asset,{float(asset-3)*3.0f,0,6});
        assert(editor.Save());
        BackgroundEditor expanded(g.GetDevice(),file);
        assert(expanded.Objects().size()==11);
        for(int asset=3;asset<LaboratoryAssets::Count;++asset)
            assert(expanded.Objects()[asset].asset==asset);
        camera.SetLookAt({10,10,-24},{10,1,6},{0,1,0});
        g.Clear(0.10f,0.12f,0.15f,1);g.SetRenderTargets();
        DirectionalLight light;light.direction={0.3f,-0.8f,0.5f};LightManager::Instance().SetDirectionalLight(light);
        expanded.Render(g.GetModelRenderer());
        RenderContext rc;rc.deviceContext=g.GetDeviceContext();rc.renderState=g.GetRenderState();rc.camera=&camera;rc.lightManager=&LightManager::Instance();
        g.GetModelRenderer()->Render(rc);
        DirectX::ScratchImage assetsImage;
        assert(SUCCEEDED(DirectX::CaptureTexture(g.GetDevice(),g.GetDeviceContext(),resource.Get(),assetsImage)));
        assert(SUCCEEDED(DirectX::SaveToWICFile(*assetsImage.GetImage(0,0,0),DirectX::WIC_FLAGS_NONE,DirectX::GetWICCodec(DirectX::WIC_CODEC_PNG),L"obj/background-assets-preview.png")));
        // Render every preset with the game's lighting; verify expanded save/load too.
        DirectionalLight stageLight;LightManager::Instance().SetDirectionalLight(stageLight);
        camera.SetLookAt({0,4,-12},{0,1.6f,10},{0,1,0});
        for(int pattern=10;pattern<16;++pattern)
        {
            std::filesystem::remove(file);
            BackgroundEditor preset(g.GetDevice(),file);
            assert(preset.AddPattern(pattern,{0,-1.2f,10.2f}));
            assert(preset.Save());
            BackgroundEditor restored(g.GetDevice(),file);
            assert(restored.Objects().size()==preset.Objects().size());
            for(size_t i=0;i<preset.Objects().size();++i)
            {
                assert(restored.Objects()[i].id==preset.Objects()[i].id);
                assert(restored.Objects()[i].position.x==preset.Objects()[i].position.x);
            }
            g.Clear(0.06f,0.08f,0.1f,1);g.SetRenderTargets();
            restored.Render(g.GetModelRenderer());g.GetModelRenderer()->Render(rc);
            DirectX::ScratchImage preview;
            assert(SUCCEEDED(DirectX::CaptureTexture(g.GetDevice(),g.GetDeviceContext(),resource.Get(),preview)));
            const auto path=L"obj/background-pattern-"+std::to_wstring(pattern)+L".png";
            assert(SUCCEEDED(DirectX::SaveToWICFile(*preview.GetImage(0,0,0),DirectX::WIC_FLAGS_NONE,DirectX::GetWICCodec(DirectX::WIC_CODEC_PNG),path.c_str())));
        }
    }
    {
        const std::string patternFile="obj/registered-background-patterns.json",sceneFile="obj/registered-background-scene.json";
        std::filesystem::copy_file("Data/Stage/Background.patterns.json",patternFile,std::filesystem::copy_options::overwrite_existing);
        std::filesystem::remove(sceneFile);
        BackgroundEditor editor(g.GetDevice(),sceneFile,patternFile);
        const auto initialCount=editor.PatternCount();
        assert(!editor.RegisterSelectionPattern("Empty selection"));
        editor.Add(0,{5,7,14});auto wall=editor.Objects()[0];wall.scale={6,4,1};wall.rotation.z=15;editor.Transform(0,wall);
        editor.Add(3,{7,8,12});auto terminal=editor.Objects()[1];terminal.scale={1.2f,1.4f,1.1f};terminal.rotation.y=20;editor.Transform(1,terminal);
        editor.SelectMany({1,0});
        const auto before=editor.Objects();
        assert(!editor.RegisterSelectionPattern("  "));
        assert(editor.RegisterSelectionPattern("Custom rotated equipment"));
        assert(editor.PatternCount()==initialCount+1&&editor.Objects().size()==2&&editor.Selections().size()==2);
        assert(!editor.RegisterSelectionPattern("Custom rotated equipment"));
        assert(editor.AddPattern(static_cast<int>(initialCount),{50,20,14},2));
        assert(editor.Objects().size()==4&&editor.Selections().size()==2);
        const auto& a=editor.Objects()[2];const auto& b=editor.Objects()[3];
        assert(a.asset==3&&b.asset==0&&a.id!=b.id);
        assert(std::abs((a.position.x-b.position.x)-(terminal.position.x-wall.position.x)*2)<.001f);
        assert(std::abs((a.position.y-b.position.y)-(terminal.position.y-wall.position.y)*2)<.001f);
        assert(std::abs((a.position.z-b.position.z)-(terminal.position.z-wall.position.z)*2)<.001f);
        assert(a.rotation.y==20&&b.rotation.z==15&&a.scale.y==terminal.scale.y*2&&b.scale.x==12);
        editor.Undo();assert(editor.Objects().size()==2&&editor.Selections().size()==2);
        editor.Redo();assert(editor.Objects().size()==4);
        BackgroundEditor restored(g.GetDevice(),sceneFile,patternFile);
        assert(restored.PatternCount()==initialCount+1);
        assert(restored.AddPattern(static_cast<int>(initialCount),{0,0,14}));
        assert(restored.Objects().size()==2&&restored.Objects()[0].rotation.y==20&&restored.Objects()[1].scale.x==6);
        std::filesystem::remove(patternFile);std::filesystem::remove(patternFile+".bak");
        puts("PASS: selected rotated/scaled objects register persistently, reload, place together, preserve XYZ offsets, and undo/redo as a group; duplicate names rejected.");
    }
    ImGuiRenderer::Finalize();ModelManager::Instance().Clear();DestroyWindow(window);
    std::filesystem::remove(file);std::filesystem::remove(std::string(file)+".bak");
    puts("PASS: create/duplicate/transform/delete undo-redo, redo invalidation, save/load, malformed load, camera restore, UI and gizmo render");
}
