#pragma once
#include "Model.h"
#include "Command/CommandManager.h"
#include <vector>
#include <string>
#include <unordered_map>
#include <DirectXCollision.h>

class ModelRenderer;
class BackgroundStateCommand;

// Static subset of the original EditorObject. History contains data, never GPU poses.
struct BackgroundObject
{
    int id = 0;
    int asset = 0;
    DirectX::XMFLOAT3 position{0,0,2};
    DirectX::XMFLOAT3 rotation{0,0,0}; // degrees, matching ImGuizmo
    DirectX::XMFLOAT3 scale{1,1,1};
};

class BackgroundEditor
{
public:
    explicit BackgroundEditor(ID3D11Device* device,
        std::string filename = "Data/Stage/Background.stage.json",
        std::string patternFilename = "Data/Stage/Background.patterns.json");
    bool Update(float elapsedTime);
    void Render(ModelRenderer* renderer);
    bool IsEditing() const { return editing; }
    void SetEditing(bool enabled);
    bool Save();
    bool Load();
    void Add(int asset, const DirectX::XMFLOAT3& position);
    bool AddPattern(int pattern, const DirectX::XMFLOAT3& position, float scale = 1.0f);
    size_t PatternCount() const { return patterns.size(); }
    bool RegisterSelectionPattern(const std::string& name);
    void Duplicate();
    void Delete();
    void Undo();
    void Redo();
    void Transform(int index, const BackgroundObject& value);
    const std::vector<BackgroundObject>& Objects() const { return objects; }
    bool IsDirty() const;
    size_t VisibleObjectCount() const { return visibleObjectCount; }
    int Selection() const { return selected; }
    void Select(int index, bool toggle = false);
    void SelectMany(const std::vector<int>& indices);
    const std::vector<int>& Selections() const { return selections; }
    void MoveSelection(const DirectX::XMFLOAT3& delta);
    void SelectRectangle(float x1, float y1, float x2, float y2, bool additive);

private:
    friend class BackgroundStateCommand;
    void Apply(const std::vector<BackgroundObject>& state, const std::vector<int>& selection);
    void Commit(const std::vector<BackgroundObject>& before, const std::vector<int>& oldSelection);
    void FinishEdit();
    void DrawUI();
    void DrawGizmoAndPick();
    void SyncModels();
    void LoadPatterns();
    struct Pattern
    {
        std::string name, description;
        float width = 1;
        bool userCreated = false;
        std::vector<BackgroundObject> objects;
    };
    std::vector<Pattern> patterns;
    int patternChoice = 0;
    int sizePresetChoice = 0;
    char newPatternName[128] = {};
    float patternScale = 1;
    DirectX::XMFLOAT3 patternPosition{16,0,14};
    bool advancePattern = true;
    std::string Serialize() const;
    ID3D11Device* device;
    std::string filename, patternFilename, status;
    std::vector<BackgroundObject> savedObjects;
    mutable bool dirtyCacheValid = false, dirtyCache = false;
    std::vector<DirectX::BoundingBox> worldBounds;
    size_t visibleObjectCount = 0;
    std::vector<BackgroundObject> objects, editBefore;
    std::unordered_map<int,std::shared_ptr<Model>> models;
    std::unordered_map<int,int> modelAssets;
    CommandManager commands;
    int selected = -1, nextId = 1, assetChoice = 0, operation = 0;
    std::vector<int> selections, editSelection;
    bool rectangleActive = false, rectangleAdditive = false;
    DirectX::XMFLOAT2 rectangleStart{};
    bool editing = false, dragging = false, snap = true;
    float grid = 1.0f, backgroundZ = 14.0f;
    DirectX::XMFLOAT3 savedEye{}, savedFocus{}, savedUp{};
};
