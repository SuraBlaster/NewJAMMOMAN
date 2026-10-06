#include <windows.h>
#include <DirectXTex.h>
#include <cassert>
#include <cmath>
#include <cstdio>
#include "Graphics.h"
#include "Stage.h"
#include "StageLoader.h"
#include "ModelRenderer.h"
#include "RenderContext.h"
#include "BackgroundEditor.h"
#include "CameraController.h"
#include "ViewVolume.h"
#include "BackgroundFog.h"
#include "BackgroundCity.h"

int main()
{
    assert(SUCCEEDED(CoInitializeEx(nullptr, COINIT_MULTITHREADED)));
    WNDCLASSW wc = {};
    wc.lpfnWndProc = DefWindowProcW;
    wc.hInstance = GetModuleHandleW(nullptr);
    wc.lpszClassName = L"LaboratoryRenderTest";
    RegisterClassW(&wc);
    HWND window = CreateWindowW(wc.lpszClassName, L"Laboratory test", WS_POPUP,
        0, 0, 1280, 720, nullptr, nullptr, wc.hInstance, nullptr);
    assert(window);
    auto& graphics = Graphics::Instance();
    graphics.Initialize(window);
    TileTypeManager types;
    assert(types.LoadDefinitions("Data/Stage/TileTypes.json"));
    StageData data;
    assert(StageLoader::Load("Data/Stage/ConvertedStage.stage.json", types, data));
    Stage stage(graphics.GetDevice(), data, types);
    assert(stage.GetStageObjects().size() == data.objects.size());
    // Compare every collision bound against the previous centered unit cube,
    // including arbitrary placement rotation and scale.
    for (size_t i = 0; i < data.objects.size(); ++i)
    {
        const auto& d = data.objects[i];
        assert(d.typeId == 100);
        AABB b;
        assert(stage.GetStageObjects()[i]->GetCollisionAABB(b));
        const float angle = DirectX::XMConvertToRadians(d.rotationDegrees);
        const float x = (std::abs(std::cos(angle)*d.scaleX)+std::abs(std::sin(angle)*d.scaleY))*0.5f;
        const float y = (std::abs(std::sin(angle)*d.scaleX)+std::abs(std::cos(angle)*d.scaleY))*0.5f;
        assert(std::abs(b.min.x-(d.positionX-x)) < 0.0001f);
        assert(std::abs(b.max.x-(d.positionX+x)) < 0.0001f);
        assert(std::abs(b.min.y-(d.positionY-y)) < 0.0001f);
        assert(std::abs(b.max.y-(d.positionY+y)) < 0.0001f);
        assert(std::abs(b.min.z+0.5f) < 0.0001f && std::abs(b.max.z-0.5f)<0.0001f);
    }
    auto& camera = Camera::Instance();
    // The active unit-block course must retain exactly the same solid volume
    // after interval merging, including every gap and climbing shaft.
    double mergedVolume=0;
    for(const auto& box:stage.GetTerrainAABBs())
        mergedVolume+=(box.max.x-box.min.x)*(box.max.y-box.min.y)*(box.max.z-box.min.z);
    assert(std::abs(mergedVolume-data.objects.size())<0.001);
    for(const auto& d:data.objects)
    {
        int coverage=0;
        for(const auto& box:stage.GetTerrainAABBs())
            if(d.positionX>box.min.x&&d.positionX<box.max.x&&d.positionY>box.min.y&&d.positionY<box.max.y)++coverage;
        assert(coverage==1);
    }
    camera.SetPerspectiveFov(DirectX::XMConvertToRadians(40), 1280.0f/720, 0.1f, 1000);
    camera.SetLookAt({4.5f,3.8f,-7.5f},{0,0.9f,0},{0,1,0});
    DirectionalLight light;
    LightManager::Instance().SetDirectionalLight(light);
    RenderContext rc;
    rc.deviceContext=graphics.GetDeviceContext();
    rc.renderState=graphics.GetRenderState();
    rc.camera=&camera;
    rc.lightManager=&LightManager::Instance();
    auto* renderer=graphics.GetModelRenderer();
    graphics.Clear(0.055f,0.075f,0.1f,1);
    graphics.SetRenderTargets();
    std::vector<std::shared_ptr<Model>> models;
    auto add=[&](const char* file,float x,float y,float z) {
        auto model=std::make_shared<Model>(graphics.GetDevice(),file);
        DirectX::XMFLOAT4X4 transform;
        DirectX::XMStoreFloat4x4(&transform,DirectX::XMMatrixTranslation(x,y,z));
        model->UpdateTransform(transform);
        renderer->Draw(ShaderId::Model,model,{1,1,1,1},1);
        models.push_back(model);
    };
    for(int x=-2;x<=2;x++) add("Data/Model/laboratory/Generated/LabBlock_1m.gltf",float(x),0,0);
    add("Data/Model/laboratory/Generated/LabColumn_3m.gltf",-2,0.5f,0.6f);
    add("Data/Model/laboratory/Generated/LabColumn_3m.gltf",2,0.5f,0.6f);
    for(int x=-1;x<=1;x++) for(int y=1;y<=3;y++)
        add("Data/Model/laboratory/Generated/LabWall_1m.gltf",float(x),float(y),0.8f);
    renderer->Render(rc);
    auto save=[&](const wchar_t* filename) {
        Microsoft::WRL::ComPtr<ID3D11RenderTargetView> target;
        rc.deviceContext->OMGetRenderTargets(1,target.GetAddressOf(),nullptr);
        Microsoft::WRL::ComPtr<ID3D11Resource> resource;
        target->GetResource(resource.GetAddressOf());
        DirectX::ScratchImage capture;
        assert(SUCCEEDED(DirectX::CaptureTexture(graphics.GetDevice(),rc.deviceContext,resource.Get(),capture)));
        assert(SUCCEEDED(DirectX::SaveToWICFile(*capture.GetImage(0,0,0),DirectX::WIC_FLAGS_NONE,
            DirectX::GetWICCodec(DirectX::WIC_CODEC_PNG),filename)));
    };
    save(L"Data/Model/laboratory/Generated/preview.png");
    graphics.Clear(0.055f,0.075f,0.1f,1);
    camera.SetLookAt({8,5,-15},{8,3,0},{0,1,0});
    for(const auto& object:stage.GetStageObjects())
        renderer->Draw(ShaderId::Model,object->GetModel(),{1,1,1,1},1);
    renderer->Render(rc);
    save(L"Data/Model/laboratory/Generated/stage-preview.png");
    graphics.Clear(0.055f,0.075f,0.1f,1);
    camera.SetLookAt({3,3,-13},{3,1.2f,2},{0,1,0});
    BackgroundEditor background(graphics.GetDevice());
    background.Render(renderer);
    for(const auto& object:stage.GetStageObjects())
        renderer->Draw(ShaderId::Model,object->GetModel(),{1,1,1,1},1);
    renderer->Render(rc);
    save(L"Data/Model/laboratory/Generated/example-layout.png");
    BackgroundFog fog(graphics.GetDevice());
    fog.Render();
    save(L"obj/laboratory-fog.png");
    // Render again after scrolling and time advance; exercise model state
    // restoration and the offscreen SRV/RTV transition on consecutive frames.
    fog.Update(15.0f);
    graphics.Clear(0.055f,0.075f,0.1f,1);
    background.Render(renderer);
    for(const auto& object:stage.GetStageObjects())
        renderer->Draw(ShaderId::Model,object->GetModel(),{1,1,1,1},1);
    renderer->Render(rc);
    fog.Render();
    save(L"obj/laboratory-fog-animated.png");
    const DirectX::XMFLOAT3 courseViews[]={{16,12,-44},{74,23,-35},{232.5f,38,-13}};
    const wchar_t* courseImages[]={L"obj/course-wall-kick.png",L"obj/course-dash.png",L"obj/course-boss.png"};
    for(int i=0;i<3;++i)
    {
        const auto eye=courseViews[i];
        camera.SetLookAt(eye,{eye.x,eye.y,0},{0,1,0});
        graphics.Clear(0.055f,0.075f,0.1f,1);
        for(const auto& object:stage.GetStageObjects())
            renderer->Draw(ShaderId::Model,object->GetModel(),{1,1,1,1},1);
        renderer->Render(rc);save(courseImages[i]);
    }
    // Use the real Follow2D update and hybrid projection. Compare rendered pixels
    // against uncullled geometry, not just a second implementation of the planes.
    CameraController follow;follow.SetMode(CameraController::CameraMode::Follow2D);
    camera.SetPerspectiveFov(DirectX::XMConvertToRadians(45),1280.0f/720,0.1f,1000);
    const DirectX::XMFLOAT3 labViews[]={{31,5,0},{218,34,0},{232.5f,34,0}};
    const wchar_t* labImages[]={L"obj/laboratory-wall-shaft.png",L"obj/laboratory-boss-approach.png",L"obj/laboratory-boss-room.png"};
    for(int i=0;i<3;i++)
    {
        for(int frame=0;frame<180;frame++)
        {
            if(i==1) {
                const auto& z=data.bossApproachCameraZones[0];
                follow.UpdateFixed2D(1.0f/60,{(z.minX+z.maxX)*0.5f,(z.minY+z.maxY)*0.5f,0});
            } else if(i==2) {
                const auto& z=data.bossEncounterZones[0];
                follow.UpdateFixed2D(1.0f/60,{(z.minX+z.maxX)*0.5f,(z.minY+z.maxY)*0.5f,0});
            } else follow.Update(1.0f/60,labViews[i],{0,0,0});
        }
        graphics.Clear(0.055f,0.075f,0.1f,1);
        background.Render(renderer);
        for(const auto& object:stage.GetStageObjects())
            renderer->Draw(ShaderId::Model,object->GetModel(),{1,1,1,1},1);
        renderer->Render(rc);fog.Render();save(labImages[i]);
    }
    const DirectX::XMFLOAT3 positions[]={{0,0,0},{31,5,0},{20,12,0},{2,20,0},{48,24,0},{79,30,0},{136,22,0},{148,10,0},{173,29,0},{220,34,0}};
    size_t legacyDifference=0,totalCulled=0;
    auto capture=[&]() {
        Microsoft::WRL::ComPtr<ID3D11RenderTargetView> target;
        rc.deviceContext->OMGetRenderTargets(1,target.GetAddressOf(),nullptr);
        Microsoft::WRL::ComPtr<ID3D11Resource> resource;target->GetResource(resource.GetAddressOf());
        DirectX::ScratchImage result;
        assert(SUCCEEDED(DirectX::CaptureTexture(graphics.GetDevice(),rc.deviceContext,resource.Get(),result)));
        return result;
    };
    BackgroundCity city(graphics.GetDevice());
    // Verify that a background pass changes openings, never opaque world pixels.
    for(int step=0;step<180;step++)follow.Update(1.0f/60,{14,4,0},{0,0,0});
    graphics.Clear(0.055f,0.075f,0.1f,1);
    const auto clearImage=capture();
    auto cityScene=[&](bool includeCity){
        graphics.Clear(0.055f,0.075f,0.1f,1);
        if(includeCity)city.Render();
        background.Render(renderer);
        for(const auto& object:stage.GetStageObjects())
            renderer->Draw(ShaderId::Model,object->GetModel(),{1,1,1,1},1);
        renderer->Render(rc);
    };
    cityScene(false);const auto withoutCity=capture();
    cityScene(true);const auto withCity=capture();
    const auto* blank=clearImage.GetImage(0,0,0);
    const auto* a=withoutCity.GetImage(0,0,0);const auto* b=withCity.GetImage(0,0,0);
    size_t skylinePixels=0,overwrittenWorld=0;
    for(size_t y=0;y<a->height;y++)for(size_t x=0;x<a->width;x++){
        const size_t offset=y*a->rowPitch+x*4;
        if(memcmp(a->pixels+offset,b->pixels+offset,3)==0)continue;
        ++skylinePixels;
        if(memcmp(a->pixels+offset,blank->pixels+offset,3)!=0)++overwrittenWorld;
    }
    assert(skylinePixels>1000&&overwrittenWorld==0);
    fog.Render();save(L"obj/laboratory-city-window.png");
    graphics.Clear(0.055f,0.075f,0.1f,1);city.Render();
    save(L"obj/city-skyline.png");
    const auto cityStill=capture();
    city.Update(20);city.Render();const auto cityMoved=capture();
    assert(memcmp(cityStill.GetPixels(),cityMoved.GetPixels(),cityStill.GetPixelsSize())!=0);
    printf("PASS: skyline visible in %zu pixels, opaque world overwritten %zu; city animates.\n",skylinePixels,overwrittenWorld);
    for(int frame=0;frame<10;++frame)
    {
        for(int step=0;step<90;++step)follow.Update(1.0f/60,positions[frame],{frame==2?-6.0f:6.0f,0,0});
        const ViewVolume correct(camera.GetView(),camera.GetProjection());
        DirectX::BoundingFrustum legacyView,legacy;
        DirectX::BoundingFrustum::CreateFromMatrix(legacyView,DirectX::XMLoadFloat4x4(&camera.GetProjection()));
        legacyView.Transform(legacy,DirectX::XMMatrixInverse(nullptr,DirectX::XMLoadFloat4x4(&camera.GetView())));
        auto draw=[&](int mode){
            graphics.Clear(0.055f,0.075f,0.1f,1);
            for(const auto& object:stage.GetStageObjects())
            {
                AABB a;assert(object->GetCollisionAABB(a));
                const DirectX::BoundingBox box(a.GetCenter(),a.GetHalfSize());
                if(mode==1&&!correct.Intersects(box)){++totalCulled;continue;}
                if(mode==2&&!legacy.Intersects(box))continue;
                renderer->Draw(ShaderId::Model,object->GetModel(),{1,1,1,1},1);
            }
            renderer->Render(rc);
        };
        draw(0);auto reference=capture();
        draw(2);auto old=capture();
        draw(1);auto fixed=capture();
        const auto* a=reference.GetImage(0,0,0);const auto* b=fixed.GetImage(0,0,0);const auto* c=old.GetImage(0,0,0);
        size_t changed=0;
        for(size_t y=0;y<a->height;++y)for(size_t x=0;x<a->width*4;++x)
        {
            if(a->pixels[y*a->rowPitch+x]!=b->pixels[y*b->rowPitch+x])++changed;
            if(a->pixels[y*a->rowPitch+x]!=c->pixels[y*c->rowPitch+x])++legacyDifference;
        }
        assert(changed==0);
        if(frame==0)save(L"obj/follow2d-start-fixed.png");
        if(frame==1)save(L"obj/follow2d-shaft-fixed.png");
    }
    assert(totalCulled>0);
    printf("PASS: Follow2D 10 views match unculled pixels; legacy changed %zu channel values, fixed 0; %zu offscreen submissions skipped.\n",legacyDifference,totalCulled);
    printf("PASS: %zu original collision bounds; imported and rendered block, column, wall.\n",data.objects.size());
    DestroyWindow(window);
}
