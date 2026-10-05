#include "BackgroundEditor.h"
#include "LaboratoryAssets.h"
#include "ModelManager.h"
#include "ModelRenderer.h"
#include "Camera.h"
#include "ViewVolume.h"
#include <imgui.h>
#include <ImGuizmo.h>
#include <DirectXCollision.h>
#include <json.hpp>
#include <fstream>
#include <filesystem>
#include <set>
#include <cmath>
#include <algorithm>
#include <stdexcept>
#include <cfloat>

using namespace DirectX;
using json = nlohmann::json;
namespace
{
    using namespace LaboratoryAssets;
    bool Valid(const BackgroundObject& o)
    {
        const auto finite=[](const XMFLOAT3& v){return std::isfinite(v.x)&&std::isfinite(v.y)&&std::isfinite(v.z);};
        return o.id>0 && o.id<1000000000 && o.asset>=0 && o.asset<Count &&
            finite(o.position)&&finite(o.rotation)&&finite(o.scale)&&
            o.scale.x>=0.01f&&o.scale.y>=0.01f&&o.scale.z>=0.01f;
    }
    XMFLOAT4X4 Matrix(const BackgroundObject& o)
    {
        XMFLOAT4X4 m;
        ImGuizmo::RecomposeMatrixFromComponents(&o.position.x,&o.rotation.x,&o.scale.x,&m._11);
        return m;
    }
    json Encode(const std::vector<BackgroundObject>& objects)
    {
        json array=json::array();
        for(const auto& o:objects) array.push_back({{"id",o.id},{"asset",o.asset},
            {"position",{o.position.x,o.position.y,o.position.z}},
            {"rotation",{o.rotation.x,o.rotation.y,o.rotation.z}},
            {"scale",{o.scale.x,o.scale.y,o.scale.z}}});
        return {{"formatVersion",1},{"objects",array}};
    }
    // Adapted from the original StageEditor::CheckSelectionInRect:
    // project all eight bounds corners instead of testing only the origin.
    bool ScreenBounds(const BackgroundObject& o, ImVec2& low, ImVec2& high)
    {
        const auto& camera=Camera::Instance();const auto* vp=ImGui::GetMainViewport();
        const auto size=ImGui::GetIO().DisplaySize;
        const auto matrix=Matrix(o);
        const auto world=XMLoadFloat4x4(&matrix),view=XMLoadFloat4x4(&camera.GetView()),projection=XMLoadFloat4x4(&camera.GetProjection());
        const auto& bounds=LaboratoryAssets::bounds[o.asset];
        XMFLOAT3 corners[8];bounds.GetCorners(corners);
        low={FLT_MAX,FLT_MAX};high={-FLT_MAX,-FLT_MAX};bool visible=false;
        for(const auto& corner:corners)
        {
            const auto clip=XMVector4Transform(XMVectorSet(corner.x,corner.y,corner.z,1),world*view*projection);
            if(XMVectorGetW(clip)<=0.001f)continue;
            XMFLOAT3 screen;
            XMStoreFloat3(&screen,XMVector3Project(XMLoadFloat3(&corner),vp->Pos.x,vp->Pos.y,size.x,size.y,0,1,projection,view,world));
            if(screen.z<0||screen.z>1)continue;
            visible=true;
            low.x=(std::min)(low.x,screen.x);low.y=(std::min)(low.y,screen.y);
            high.x=(std::max)(high.x,screen.x);high.y=(std::max)(high.y,screen.y);
        }
        return visible;
    }
}

void BackgroundEditor::SelectRectangle(float x1,float y1,float x2,float y2,bool additive)
{
    auto indices=additive?selections:std::vector<int>{};
    for(int i=0;i<static_cast<int>(objects.size());++i)
    {
        ImVec2 low,high;
        if(ScreenBounds(objects[i],low,high)&&high.x>=(std::min)(x1,x2)&&low.x<=(std::max)(x1,x2)&&
            high.y>=(std::min)(y1,y2)&&low.y<=(std::max)(y1,y2))indices.push_back(i);
    }
    SelectMany(indices);
}

// Same Execute/Undo contract as the supplied Transform/Create/DeleteCommand.
// Reference the owning editor rather than the old StageEditor singleton.
class BackgroundStateCommand : public ICommand
{
public:
    BackgroundStateCommand(BackgroundEditor& editor,std::vector<BackgroundObject> before,
        std::vector<BackgroundObject> after,std::vector<int> oldSelection,std::vector<int> newSelection)
        : editor(editor),before(std::move(before)),after(std::move(after)),oldSelection(oldSelection),newSelection(newSelection) {}
    void Execute() override { editor.Apply(after,newSelection); }
    void Undo() override { editor.Apply(before,oldSelection); }
private:
    BackgroundEditor& editor;
    std::vector<BackgroundObject> before,after;
    std::vector<int> oldSelection,newSelection;
};

BackgroundEditor::BackgroundEditor(ID3D11Device* device,std::string filename,std::string patternFilename)
    : device(device),filename(std::move(filename)),patternFilename(std::move(patternFilename))
{
    if(std::filesystem::exists(this->filename)) Load();
    commands.Clear();
    LoadPatterns();
}
void BackgroundEditor::LoadPatterns()
{
    try
    {
        std::ifstream input(patternFilename);
        json root;input>>root;
        if(root.at("formatVersion").get<int>()!=1)throw std::runtime_error("Unsupported pattern format");
        std::vector<Pattern> loaded;
        for(const auto& entry:root.at("patterns"))
        {
            Pattern pattern;
            pattern.name=entry.at("name").get<std::string>();
            pattern.description=entry.at("description").get<std::string>();
            pattern.width=entry.at("width").get<float>();
            pattern.userCreated=entry.value("userCreated",false);
            if(!std::isfinite(pattern.width)||pattern.width<=0)throw std::runtime_error("Invalid pattern width");
            for(const auto& item:entry.at("objects"))
            {
                const auto xyz=[](const json& a){
                    if(!a.is_array()||a.size()!=3)throw std::runtime_error("Invalid pattern XYZ");
                    return XMFLOAT3{a[0].get<float>(),a[1].get<float>(),a[2].get<float>()};};
                BackgroundObject o;o.id=1;o.asset=item.at("asset").get<int>();
                o.position=xyz(item.at("position"));o.rotation=xyz(item.at("rotation"));o.scale=xyz(item.at("scale"));
                if(!Valid(o))throw std::runtime_error("Invalid pattern object");
                pattern.objects.push_back(o);
            }
            if(pattern.objects.empty()||pattern.objects.size()>1000)throw std::runtime_error("Invalid pattern size");
            loaded.push_back(std::move(pattern));
        }
        patterns=std::move(loaded);
        patternChoice=0;sizePresetChoice=0;
        for(int i=0;i<static_cast<int>(patterns.size());i++)if(patterns[i].objects.size()>1){patternChoice=i;break;}
    }
    catch(const std::exception& e){status=std::string("Patterns unavailable: ")+e.what();}
}
bool BackgroundEditor::RegisterSelectionPattern(const std::string& inputName)
{
    FinishEdit();
    const auto first=inputName.find_first_not_of(" \t\r\n"),last=inputName.find_last_not_of(" \t\r\n");
    if(first==std::string::npos||selections.empty()||selections.size()>1000){status="Select 1-1000 objects and enter a pattern name.";return false;}
    const std::string name=inputName.substr(first,last-first+1);
    if(name.size()>127||std::any_of(patterns.begin(),patterns.end(),[&](const Pattern& p){return p.name==name;})){
        status="Use a new name (up to 127 bytes). Existing patterns are kept.";return false;
    }
    try
    {
        XMFLOAT3 low{FLT_MAX,FLT_MAX,FLT_MAX},high{-FLT_MAX,-FLT_MAX,-FLT_MAX};
        for(int index:selections)
        {
            const auto& o=objects.at(index);const auto matrix=Matrix(o);
            BoundingBox world;LaboratoryAssets::bounds[o.asset].Transform(world,XMLoadFloat4x4(&matrix));
            XMFLOAT3 corners[8];world.GetCorners(corners);
            for(const auto& c:corners){
                low.x=(std::min)(low.x,c.x);low.y=(std::min)(low.y,c.y);low.z=(std::min)(low.z,c.z);
                high.x=(std::max)(high.x,c.x);high.y=(std::max)(high.y,c.y);high.z=(std::max)(high.z,c.z);
            }
        }
        const XMFLOAT3 origin{(low.x+high.x)*0.5f,low.y,(low.z+high.z)*0.5f};
        Pattern registered;registered.name=name;registered.description="Saved selection; base = bottom center of selection bounds.";
        registered.width=(std::max)(0.01f,high.x-low.x);registered.userCreated=true;
        for(int index:selections){auto o=objects.at(index);o.id=1;
            o.position={o.position.x-origin.x,o.position.y-origin.y,o.position.z-origin.z};registered.objects.push_back(o);}
        auto updated=patterns;updated.push_back(registered);
        json array=json::array();
        for(const auto& p:updated){auto entries=Encode(p.objects).at("objects");for(auto& o:entries)o.erase("id");
            array.push_back({{"name",p.name},{"description",p.description},{"width",p.width},{"userCreated",p.userCreated},{"objects",entries}});}
        const std::filesystem::path target(patternFilename),temp(patternFilename+".tmp");
        std::ofstream out(temp,std::ios::binary|std::ios::trunc);out<<json{{"formatVersion",1},{"patterns",array}}.dump(2)<<'\n';
        out.flush();if(!out)throw std::runtime_error("Cannot write pattern file");out.close();
        if(std::filesystem::exists(target)&&!CopyFileW(target.c_str(),std::filesystem::path(patternFilename+".bak").c_str(),FALSE))throw std::runtime_error("Cannot back up patterns");
        if(!MoveFileExW(temp.c_str(),target.c_str(),MOVEFILE_REPLACE_EXISTING|MOVEFILE_WRITE_THROUGH))throw std::runtime_error("Cannot save patterns");
        patterns=std::move(updated);patternChoice=static_cast<int>(patterns.size())-1;
        patternPosition=origin;patternScale=1;
        status="Registered and saved pattern: "+name;return true;
    }
    catch(const std::exception& e){status=std::string("Pattern registration failed: ")+e.what();return false;}
}
bool BackgroundEditor::AddPattern(int pattern,const XMFLOAT3& position,float scale)
{
    if(pattern<0||pattern>=static_cast<int>(patterns.size())||!std::isfinite(scale)||scale<0.01f)return false;
    const auto& source=patterns[pattern];
    if(objects.size()+source.objects.size()>10000)return false;
    std::vector<BackgroundObject> added;
    int id=nextId;
    for(auto o:source.objects)
    {
        o.id=id++;
        o.position={position.x+o.position.x*scale,position.y+o.position.y*scale,position.z+o.position.z*scale};
        o.scale={o.scale.x*scale,o.scale.y*scale,o.scale.z*scale};
        if(!Valid(o))return false;
        added.push_back(o);
    }
    FinishEdit();auto before=objects;auto old=selections;selections.clear();
    for(const auto& o:added){selections.push_back(static_cast<int>(objects.size()));objects.push_back(o);}
    selected=selections.back();Commit(before,old);
    status="Added pattern: "+source.name;return true;
}
std::string BackgroundEditor::Serialize() const { return Encode(objects).dump(2); }
bool BackgroundEditor::IsDirty() const
{
    if(!dirtyCacheValid)
    {
        const auto xyz=[](const XMFLOAT3& a,const XMFLOAT3& b){return a.x==b.x&&a.y==b.y&&a.z==b.z;};
        dirtyCache=objects.size()!=savedObjects.size() || !std::equal(objects.begin(),objects.end(),savedObjects.begin(),
            [&](const BackgroundObject& a,const BackgroundObject& b){
                return a.id==b.id&&a.asset==b.asset&&xyz(a.position,b.position)&&xyz(a.rotation,b.rotation)&&xyz(a.scale,b.scale);});
        dirtyCacheValid=true;
    }
    return dirtyCache;
}
void BackgroundEditor::Apply(const std::vector<BackgroundObject>& state,const std::vector<int>& selection)
{
    objects=state;
    selections=selection;
    selected=selections.empty()?-1:selections.back();
    for(const auto& o:objects) nextId=(std::max)(nextId,o.id+1);
    SyncModels();
}
void BackgroundEditor::SyncModels()
{
    dirtyCacheValid=false;
    worldBounds.resize(objects.size());
    size_t index=0;
    std::set<int> live;
    for(const auto& o:objects)
    {
        live.insert(o.id);
        auto& model=models[o.id];
        if(!model || modelAssets[o.id]!=o.asset)
        {
            model=ModelManager::Instance().CreateInstance(device,paths[o.asset]);
            modelAssets[o.id]=o.asset;
        }
        const auto matrix=Matrix(o);
        model->UpdateTransform(matrix);
        LaboratoryAssets::bounds[o.asset].Transform(worldBounds[index++],XMLoadFloat4x4(&matrix));
    }
    for(auto it=models.begin();it!=models.end();)
        if(!live.count(it->first)){modelAssets.erase(it->first);it=models.erase(it);} else ++it;
}
void BackgroundEditor::Commit(const std::vector<BackgroundObject>& before,const std::vector<int>& oldSelection)
{
    if(Encode(before)==Encode(objects)) return;
    commands.Execute(std::make_unique<BackgroundStateCommand>(*this,before,objects,oldSelection,selections));
}
void BackgroundEditor::FinishEdit()
{
    if(!dragging) return;
    dragging=false;
    Commit(editBefore,editSelection);
    editBefore.clear();
}
void BackgroundEditor::SelectMany(const std::vector<int>& indices)
{
    FinishEdit();selections.clear();
    for(int i:indices) if(i>=0&&i<static_cast<int>(objects.size())&&std::find(selections.begin(),selections.end(),i)==selections.end())selections.push_back(i);
    selected=selections.empty()?-1:selections.back();
}
void BackgroundEditor::Select(int index,bool toggle)
{
    auto indices=toggle?selections:std::vector<int>{};
    auto it=std::find(indices.begin(),indices.end(),index);
    if(it!=indices.end())indices.erase(it);else if(index>=0)indices.push_back(index);
    SelectMany(indices);
}
void BackgroundEditor::MoveSelection(const XMFLOAT3& delta)
{
    FinishEdit();auto before=objects;
    for(int i:selections){objects[i].position.x+=delta.x;objects[i].position.y+=delta.y;objects[i].position.z+=delta.z;}
    Commit(before,selections);
}
void BackgroundEditor::Add(int asset,const XMFLOAT3& position)
{
    FinishEdit();
    BackgroundObject o; o.id=nextId++;o.asset=asset;o.position=position;
    if(!Valid(o)) return;
    auto before=objects;auto old=selections;
    objects.push_back(o);selected=static_cast<int>(objects.size())-1;selections={selected};Commit(before,old);
}
void BackgroundEditor::Duplicate()
{
    FinishEdit();if(selections.empty())return;
    auto before=objects;auto old=selections;selections.clear();
    for(int i:old){auto copy=before[i];copy.id=nextId++;copy.position.x+=grid;objects.push_back(copy);selections.push_back(static_cast<int>(objects.size())-1);}
    selected=selections.back();Commit(before,old);
}
void BackgroundEditor::Delete()
{
    FinishEdit();if(selections.empty())return;
    auto before=objects;auto old=selections;auto sorted=selections;
    std::sort(sorted.rbegin(),sorted.rend());
    for(int i:sorted)objects.erase(objects.begin()+i);
    selections.clear();selected=-1;Commit(before,old);
}
void BackgroundEditor::Transform(int index,const BackgroundObject& value)
{
    FinishEdit();
    if(index<0||index>=static_cast<int>(objects.size())||!Valid(value)) return;
    if(value.id!=objects[index].id||value.asset!=objects[index].asset) return;
    auto before=objects;objects[index]=value;Commit(before,selections);
}
void BackgroundEditor::Undo() { FinishEdit();commands.Undo(); }
void BackgroundEditor::Redo() { FinishEdit();commands.Redo(); }
bool BackgroundEditor::Save()
{
    FinishEdit();
    try
    {
        const auto content=Serialize();
        const std::filesystem::path target(filename),temp(filename+".tmp");
        std::ofstream out(temp,std::ios::binary|std::ios::trunc);
        out<<content<<'\n';out.flush();if(!out) throw std::runtime_error("Cannot write temporary file");out.close();
        if(std::filesystem::exists(target)&&!CopyFileW(target.c_str(),std::filesystem::path(filename+".bak").c_str(),FALSE))
            throw std::runtime_error("Cannot create backup");
        if(!MoveFileExW(temp.c_str(),target.c_str(),MOVEFILE_REPLACE_EXISTING|MOVEFILE_WRITE_THROUGH))
            throw std::runtime_error("Cannot replace save file");
        savedObjects=objects;dirtyCache=false;dirtyCacheValid=true;status="Saved: "+filename;return true;
    }
    catch(const std::exception& e){status=std::string("Save failed: ")+e.what();return false;}
}
bool BackgroundEditor::Load()
{
    FinishEdit();
    try
    {
        std::ifstream input(filename);if(!input) throw std::runtime_error("File not found");
        json j;input>>j;
        if(!j.at("formatVersion").is_number_integer()||j.at("formatVersion").get<int>()!=1||!j.at("objects").is_array())
            throw std::runtime_error("Unsupported background format");
        if(j.at("objects").size()>10000) throw std::runtime_error("Too many background objects");
        std::vector<BackgroundObject> loaded;std::set<int> ids;
        const auto vector=[](const json& a){
            if(!a.is_array()||a.size()!=3) throw std::runtime_error("Expected XYZ array");
            for(const auto& v:a) if(!v.is_number()) throw std::runtime_error("Invalid XYZ number");
            return XMFLOAT3{a[0].get<float>(),a[1].get<float>(),a[2].get<float>()};};
        for(const auto& item:j.at("objects"))
        {
            BackgroundObject o;
            if(!item.at("id").is_number_integer()||!item.at("asset").is_number_integer()) throw std::runtime_error("Invalid ID");
            o.id=item.at("id").get<int>();o.asset=item.at("asset").get<int>();
            o.position=vector(item.at("position"));o.rotation=vector(item.at("rotation"));o.scale=vector(item.at("scale"));
            if(!Valid(o)||!ids.insert(o.id).second) throw std::runtime_error("Invalid transform, asset or duplicate ID");
            loaded.push_back(o);
        }
        auto before=objects;auto old=selections;
        // Loaded IDs may refer to different assets. Recreate instances on every load/undo.
        models.clear();objects=loaded;selected=-1;selections.clear();Commit(before,old);SyncModels();
        savedObjects=objects;dirtyCache=false;dirtyCacheValid=true;status="Loaded (Undo restores previous layout)";return true;
    }
    catch(const std::exception& e){status=std::string("Load failed: ")+e.what();return false;}
}
void BackgroundEditor::Render(ModelRenderer* renderer)
{
    // A cool, subdued tint separates scenery from playable platforms.
    const auto& camera=Camera::Instance();
    const ViewVolume worldFrustum(camera.GetView(),camera.GetProjection());
    visibleObjectCount=0;
    for(size_t i=0;i<objects.size();++i)
    {
        if(!worldFrustum.Intersects(worldBounds[i]))continue;
        ++visibleObjectCount;
        renderer->Draw(ShaderId::Model,models.at(objects[i].id),{0.78f,0.82f,0.88f,1},1);
    }
}

void BackgroundEditor::SetEditing(bool enabled)
{
    if(enabled==editing) return;
    FinishEdit();rectangleActive=false;editing=enabled;
    auto& camera=Camera::Instance();
    if(editing)
    {
        savedEye=camera.GetEye();savedFocus=camera.GetFocus();savedUp=camera.GetUp();
        // Keep the current viewing direction and pull back around the same focus.
        const auto offset=XMLoadFloat3(&savedEye)-XMLoadFloat3(&savedFocus);
        const float distance=XMVectorGetX(XMVector3Length(offset));
        const float overviewDistance=std::clamp(distance*2.0f,30.0f,200.0f);
        const auto direction=distance>0.001f?XMVector3Normalize(offset):XMVectorSet(0,0,-1,0);
        XMFLOAT3 eye;
        XMStoreFloat3(&eye,XMLoadFloat3(&savedFocus)+direction*overviewDistance);
        camera.SetLookAt(eye,savedFocus,savedUp);
    }
    else camera.SetLookAt(savedEye,savedFocus,savedUp);
}

bool BackgroundEditor::Update(float)
{
    auto& io=ImGui::GetIO();auto& camera=Camera::Instance();
    bool enabled=editing;
    const auto* viewport=ImGui::GetMainViewport();
    ImGui::SetNextWindowPos(ImVec2(viewport->Pos.x+io.DisplaySize.x-385,viewport->Pos.y+10),ImGuiCond_FirstUseEver);
    ImGui::SetNextWindowSize(ImVec2(330,100),ImGuiCond_FirstUseEver);
    if(ImGui::Begin("Background editor"))
    {
        ImGui::Checkbox("Edit background (F6)",&enabled);
        ImGui::TextUnformatted(IsDirty()?"Unsaved changes":"Saved");
        ImGui::Text("Background: %zu / %zu visible",visibleObjectCount,objects.size());
    }
    ImGui::End();
    if(!io.WantTextInput && ImGui::IsKeyPressed(VK_F6,false)) enabled=!editing;
    SetEditing(enabled);
    if(!editing) return false;
    if(!io.WantCaptureMouse && !ImGuizmo::IsUsing())
    {
        auto eye=camera.GetEye(),focus=camera.GetFocus();
        if(ImGui::IsMouseDragging(2))
        {
            const auto right=camera.GetRight(),up=camera.GetUp();
            const float distance=XMVectorGetX(XMVector3Length(XMLoadFloat3(&eye)-XMLoadFloat3(&focus)));
            const auto shift=XMLoadFloat3(&right)*(-io.MouseDelta.x*distance*0.0015f)+XMLoadFloat3(&up)*(io.MouseDelta.y*distance*0.0015f);
            XMStoreFloat3(&eye,XMLoadFloat3(&eye)+shift);XMStoreFloat3(&focus,XMLoadFloat3(&focus)+shift);
        }
        if(io.MouseWheel!=0)
        {
            auto offset=XMLoadFloat3(&eye)-XMLoadFloat3(&focus);
            const float distance=XMVectorGetX(XMVector3Length(offset));
            const float desired=std::clamp(distance*std::pow(0.85f,io.MouseWheel),1.0f,200.0f);
            XMStoreFloat3(&eye,XMLoadFloat3(&focus)+XMVector3Normalize(offset)*desired);
        }
        camera.SetLookAt(eye,focus,{0,1,0});
    }
    DrawUI();
    if(!io.WantTextInput&&!ImGui::IsAnyItemActive()&&!ImGuizmo::IsUsing())
    {
        if(io.KeyCtrl && ImGui::IsKeyPressed('Z',false)) Undo();
        if(io.KeyCtrl && ImGui::IsKeyPressed('Y',false)) Redo();
        if(io.KeyCtrl && ImGui::IsKeyPressed('D',false)) Duplicate();
        if(io.KeyCtrl && ImGui::IsKeyPressed('S',false)) Save();
        if(ImGui::IsKeyPressed(VK_DELETE,false)) Delete();
    }
    DrawGizmoAndPick();return true;
}
void BackgroundEditor::DrawUI()
{
    const auto* viewport=ImGui::GetMainViewport();
    ImGui::SetNextWindowPos(ImVec2(viewport->Pos.x+ImGui::GetIO().DisplaySize.x-385,viewport->Pos.y+120),ImGuiCond_FirstUseEver);
    ImGui::SetNextWindowSize(ImVec2(370,(std::max)(200.0f,ImGui::GetIO().DisplaySize.y-130.0f)),ImGuiCond_FirstUseEver);
    if(ImGui::Begin("Background placement"))
    {
        ImGui::TextWrapped("Middle drag: pan / Wheel: zoom. F6: return to game.");
        if(ImGui::CollapsingHeader("Patterns / ready-made rooms",ImGuiTreeNodeFlags_DefaultOpen))
        {
            if(patterns.empty())ImGui::TextWrapped("No patterns loaded. Check Background.patterns.json.");
            else
            {
                if(ImGui::BeginCombo("Pattern",patterns[patternChoice].name.c_str()))
                {
                    for(int i=0;i<static_cast<int>(patterns.size());++i)
                        if(ImGui::Selectable(patterns[i].name.c_str(),i==patternChoice))patternChoice=i;
                    ImGui::EndCombo();
                }
                const auto& p=patterns[patternChoice];
                ImGui::TextWrapped("%s",p.description.c_str());
                ImGui::Text("%zu objects / Width %.1f",p.objects.size(),p.width*patternScale);
                ImGui::SetNextItemWidth(185);
                ImGui::DragFloat3("Base XYZ",&patternPosition.x,0.1f);
                ImGui::DragFloat("Pattern size",&patternScale,0.05f,0.1f,10.0f);
                if(ImGui::Button("Use camera X"))patternPosition.x=Camera::Instance().GetFocus().x;
                ImGui::Checkbox("Advance X after adding",&advancePattern);
                if(ImGui::Button("Add pattern"))
                {
                    if(AddPattern(patternChoice,patternPosition,patternScale))
                    {
                        auto& c=Camera::Instance();auto focus=patternPosition;focus.y+=2.8f*patternScale;
                        XMFLOAT3 eye;XMStoreFloat3(&eye,XMLoadFloat3(&c.GetEye())+XMLoadFloat3(&focus)-XMLoadFloat3(&c.GetFocus()));
                        c.SetLookAt(eye,focus,{0,1,0});
                        if(advancePattern)patternPosition.x+=p.width*patternScale;
                    }
                    else status="Cannot add pattern: check position, size or object limit.";
                }
                ImGui::TextWrapped("Base = bottom center of pattern. Added parts are selected together. Undo removes one whole pattern.");
            }
        }
        if(ImGui::CollapsingHeader("Register selection as pattern",ImGuiTreeNodeFlags_DefaultOpen))
        {
            ImGui::Text("Selected objects: %zu",selections.size());
            ImGui::SetNextItemWidth(185);
            ImGui::InputText("New pattern name",newPatternName,sizeof(newPatternName));
            if(ImGui::Button("Register selected objects"))
                if(RegisterSelectionPattern(newPatternName))newPatternName[0]='\0';
            ImGui::TextWrapped("Ctrl+click or drag a selection rectangle, then register. Position, rotation and size are preserved. Patterns are saved immediately.");
        }
        ImGui::Separator();
        std::vector<int> sizePresets;
        for(int i=0;i<static_cast<int>(patterns.size());i++)if(patterns[i].objects.size()==1)sizePresets.push_back(i);
        if(!sizePresets.empty())
        {
            sizePresetChoice=(std::min)(sizePresetChoice,static_cast<int>(sizePresets.size())-1);
            if(ImGui::BeginCombo("Sized object",patterns[sizePresets[sizePresetChoice]].name.c_str())){
                for(int i=0;i<static_cast<int>(sizePresets.size());i++)
                    if(ImGui::Selectable(patterns[sizePresets[i]].name.c_str(),i==sizePresetChoice))sizePresetChoice=i;
                ImGui::EndCombo();
            }
            if(ImGui::Button("Add sized object at camera")){
                auto p=Camera::Instance().GetFocus();p.z=backgroundZ;
                if(snap){p.x=std::round(p.x/grid)*grid;p.y=std::round(p.y/grid)*grid;}
                AddPattern(sizePresets[sizePresetChoice],p);
            }
        }
        ImGui::Combo("Asset",&assetChoice,names,Count);
        ImGui::DragFloat("New object Z",&backgroundZ,0.1f);
        ImGui::Checkbox("Gizmo snap",&snap);ImGui::SameLine();
        if(ImGui::RadioButton("1",grid==1))grid=1;ImGui::SameLine();
        if(ImGui::RadioButton("0.5",grid==0.5f))grid=0.5f;
        if(ImGui::Button("Add at camera focus"))
        {
            auto p=Camera::Instance().GetFocus();p.z=backgroundZ;
            if(snap){p.x=std::round(p.x/grid)*grid;p.y=std::round(p.y/grid)*grid;}
            Add(assetChoice,p);
        }
        if(ImGui::Button("Duplicate")) Duplicate();ImGui::SameLine();
        if(ImGui::Button("Delete")) Delete();
        if(ImGui::Button("Undo")) Undo();ImGui::SameLine();
        if(ImGui::Button("Redo")) Redo();
        if(ImGui::Button("Save")) Save();ImGui::SameLine();
        if(ImGui::Button("Reload")) Load();
        ImGui::TextWrapped("%s",status.c_str());
        ImGui::BeginChild("Objects",ImVec2(0,140),true);
        ImGuiListClipper clipper;
        clipper.Begin(static_cast<int>(objects.size()));
        while(clipper.Step())for(int i=clipper.DisplayStart;i<clipper.DisplayEnd;++i)
        {
            ImGui::PushID(objects[i].id);
            const std::string label=std::to_string(objects[i].id)+": "+names[objects[i].asset];
            if(ImGui::Selectable(label.c_str(),std::find(selections.begin(),selections.end(),i)!=selections.end())) Select(i,ImGui::GetIO().KeyCtrl || ImGui::GetIO().KeyShift);
            ImGui::PopID();
        }
        ImGui::EndChild();
        ImGui::Text("Selected: %zu (Ctrl-click to toggle)",selections.size());
        ImGui::TextWrapped("Drag a rectangle to select. Shift-drag adds. Ctrl+D duplicates the selection.");
        ImGui::RadioButton("Move",&operation,0);ImGui::SameLine();
        if(selections.size()<=1)
        {
            ImGui::RadioButton("Rotate",&operation,1);ImGui::SameLine();
            ImGui::RadioButton("Scale",&operation,2);
        }
        else operation=0;
        if(selections.size()>1)ImGui::TextWrapped("Multiple selection: move with the gizmo; duplicate/delete apply to all.");
        if(selections.size()==1)
        {
            auto field=[&](const char* label,XMFLOAT3& value,float speed){
                const auto before=objects[selected];
                const bool changed=ImGui::DragFloat3(label,&value.x,speed);
                if(ImGui::IsItemActivated()){FinishEdit();editBefore=objects;editBefore[selected]=before;editSelection=selections;dragging=true;}
                if(changed)
                {
                    if(!Valid(objects[selected]))objects[selected]=before;
                    SyncModels();
                }
                if(ImGui::IsItemDeactivated())FinishEdit();
            };
            field("Position",objects[selected].position,0.05f);
            field("Rotation (deg)",objects[selected].rotation,0.5f);
            field("Scale XYZ",objects[selected].scale,0.01f);
            if(ImGui::Button("Focus selected"))
            {
                auto& c=Camera::Instance();auto p=objects[selected].position;
                const auto delta=XMLoadFloat3(&p)-XMLoadFloat3(&c.GetFocus());XMFLOAT3 eye;
                XMStoreFloat3(&eye,XMLoadFloat3(&c.GetEye())+delta);c.SetLookAt(eye,p,{0,1,0});
            }
        }
        if(ImGui::CollapsingHeader("Camera / depth"))
        {
            auto& c=Camera::Instance();auto eye=c.GetEye(),focus=c.GetFocus();
            bool changed=ImGui::DragFloat3("Eye",&eye.x,0.1f);
            changed|=ImGui::DragFloat3("Focus",&focus.x,0.1f);
            if(changed && std::hypot(eye.x-focus.x,eye.z-focus.z)>0.01f)c.SetLookAt(eye,focus,{0,1,0});
            if(ImGui::Button("Restore game view"))c.SetLookAt(savedEye,savedFocus,savedUp);
            ImGui::TextWrapped("Background is usually +Z. Column origin is its base; wall origin is its center.");
        }
    }
    ImGui::End();
}
void BackgroundEditor::DrawGizmoAndPick()
{
    const auto& c=Camera::Instance();const auto* viewport=ImGui::GetMainViewport();
    auto& io=ImGui::GetIO();
    ImGuizmo::SetOrthographic(false);ImGuizmo::SetDrawlist(ImGui::GetForegroundDrawList());
    ImGuizmo::SetRect(viewport->Pos.x,viewport->Pos.y,io.DisplaySize.x,io.DisplaySize.y);
    for(int i:selections)
    {
        ImVec2 low,high;
        if(ScreenBounds(objects[i],low,high))ImGui::GetForegroundDrawList()->AddRect(low,high,IM_COL32(255,205,75,230));
    }
    if(selected>=0)
    {
        auto matrix=Matrix(objects[selected]);const auto before=objects[selected];
        const bool multiple=selections.size()>1;
        if(multiple){XMFLOAT3 center{};for(int i:selections){center.x+=objects[i].position.x;center.y+=objects[i].position.y;center.z+=objects[i].position.z;}const float count=static_cast<float>(selections.size());XMStoreFloat4x4(&matrix,XMMatrixTranslation(center.x/count,center.y/count,center.z/count));}
        const auto previousMatrix=matrix;
        const ImGuizmo::OPERATION op=(multiple||operation==0)?ImGuizmo::TRANSLATE:operation==1?ImGuizmo::ROTATE:ImGuizmo::SCALE;
        float steps[3]={grid,grid,grid};if(!multiple&&operation==1)steps[0]=steps[1]=steps[2]=15;else if(!multiple&&operation==2)steps[0]=steps[1]=steps[2]=0.1f;
        const bool changed=ImGuizmo::Manipulate(&c.GetView()._11,&c.GetProjection()._11,op,(!multiple&&operation==2)?ImGuizmo::LOCAL:ImGuizmo::WORLD,&matrix._11,nullptr,snap?steps:nullptr);
        if(ImGuizmo::IsUsing()&&!dragging){editBefore=objects;editSelection=selections;dragging=true;}
        if(changed)
        {
            if(multiple){for(int i:selections){objects[i].position.x+=matrix._41-previousMatrix._41;objects[i].position.y+=matrix._42-previousMatrix._42;objects[i].position.z+=matrix._43-previousMatrix._43;}}
            else {auto& o=objects[selected];
            ImGuizmo::DecomposeMatrixToComponents(&matrix._11,&o.position.x,&o.rotation.x,&o.scale.x);
            if(!Valid(o))o=before;}SyncModels();
        }
        if(dragging&&!ImGuizmo::IsUsing()&&!ImGui::IsAnyItemActive())FinishEdit();
    }
    if(!io.WantCaptureMouse&&!ImGuizmo::IsOver()&&!ImGuizmo::IsUsing()&&ImGui::IsMouseClicked(0))
    {rectangleActive=true;rectangleStart={io.MousePos.x,io.MousePos.y};rectangleAdditive=io.KeyShift||io.KeyCtrl;}
    if(rectangleActive&&ImGui::IsMouseDown(0))
    {auto* draw=ImGui::GetForegroundDrawList();ImVec2 a((std::min)(rectangleStart.x,io.MousePos.x),(std::min)(rectangleStart.y,io.MousePos.y)),b((std::max)(rectangleStart.x,io.MousePos.x),(std::max)(rectangleStart.y,io.MousePos.y));draw->AddRectFilled(a,b,IM_COL32(60,160,255,35));draw->AddRect(a,b,IM_COL32(60,160,255,230));}
    if(rectangleActive&&ImGui::IsMouseReleased(0))
    {
        rectangleActive=false;
        if(std::hypot(io.MousePos.x-rectangleStart.x,io.MousePos.y-rectangleStart.y)>4){SelectRectangle(rectangleStart.x,rectangleStart.y,io.MousePos.x,io.MousePos.y,rectangleAdditive);return;}
        const auto v=XMLoadFloat4x4(&c.GetView()),p=XMLoadFloat4x4(&c.GetProjection());
        const float x=io.MousePos.x-viewport->Pos.x,y=io.MousePos.y-viewport->Pos.y;
        auto start=XMVector3Unproject(XMVectorSet(x,y,0,1),0,0,io.DisplaySize.x,io.DisplaySize.y,0,1,p,v,XMMatrixIdentity());
        auto end=XMVector3Unproject(XMVectorSet(x,y,1,1),0,0,io.DisplaySize.x,io.DisplaySize.y,0,1,p,v,XMMatrixIdentity());
        float nearest=FLT_MAX;int hit=-1;
        for(int i=0;i<static_cast<int>(objects.size());++i)
        {
            const auto matrix=Matrix(objects[i]);const auto inverse=XMMatrixInverse(nullptr,XMLoadFloat4x4(&matrix));
            const auto origin=XMVector3TransformCoord(start,inverse),direction=XMVector3Normalize(XMVector3TransformNormal(end-start,inverse));
            const int asset=objects[i].asset;
            const auto& box=LaboratoryAssets::bounds[asset];
            float distance;
            if(box.Intersects(origin,direction,distance))
            {
                const auto point=XMVector3TransformCoord(origin+direction*distance,XMLoadFloat4x4(&matrix));
                const float worldDistance=XMVectorGetX(XMVector3Length(point-start));
                if(worldDistance<nearest){nearest=worldDistance;hit=i;}
            }
        }
        Select(hit,rectangleAdditive);
    }
}
