#include "BossGate.h"
#include "StageLoader.h"
#include "TileTypeManager.h"
#include <cassert>
#include <filesystem>
#include <fstream>
#include <iostream>

int main()
{
    const AABB bounds{ { -0.5f, 0.0f, -0.5f }, { 0.5f, 3.0f, 0.5f } };
    const AABB leftBody{ { -1.4f, 0.0f, -0.4f }, { -0.6f, 1.4f, 0.4f } };
    const AABB rightBody{ { 0.6f, 0.0f, -0.4f }, { 1.4f, 1.4f, 0.4f } };
    const AABB crossingBody{ { -0.1f, 0.0f, -0.4f }, { 0.7f, 1.4f, 0.4f } };
    const AABB farLeftBody{ { -8.0f, 0.0f, -0.4f }, { -7.2f, 1.4f, 0.4f } };

    BossGate gate(bounds);
    assert(gate.IsSolid());
    gate.Update(0.35f, rightBody);
    assert(gate.GetOpenness() == 0.0f); // Wrong side never opens the gate.
    gate.Update(0.175f, leftBody);
    assert(gate.IsSolid() && gate.GetOpenness() > 0.0f && gate.GetOpenness() < 1.0f);
    gate.Update(0.35f, leftBody);
    assert(!gate.IsSolid());
    gate.Update(0.35f, farLeftBody);
    assert(!gate.IsLocked() && gate.GetOpenness() == 0.0f);
    gate.Update(0.35f, leftBody);
    gate.Update(0.0f, crossingBody);
    assert(!gate.IsLocked() && !gate.IsSolid()); // Partial crossing cannot seal onto the body.
    gate.Update(0.0f, rightBody);
    assert(gate.IsLocked() && gate.IsSolid() && gate.GetOpenness() == 1.0f);
    gate.Update(0.35f, rightBody);
    assert(gate.GetOpenness() == 0.0f);
    gate.Update(10.0f, leftBody);
    assert(gate.IsSolid() && gate.GetOpenness() == 0.0f);

    BossGate leftGate(bounds, BossGate::Direction::Left);
    leftGate.Update(0.35f, leftBody);
    assert(leftGate.IsSolid());
    leftGate.Update(0.35f, rightBody);
    assert(!leftGate.IsSolid());
    leftGate.Update(0.0f, leftBody);
    assert(leftGate.IsLocked());

    BossGate dashGate(bounds);
    dashGate.Update(0.35f, leftBody);
    dashGate.Update(0.0f, AABB{ { 3.0f, 0.0f, -0.4f }, { 3.8f, 1.4f, 0.4f } });
    assert(dashGate.IsLocked());

    BossGate highGate(bounds);
    highGate.Update(0.35f, AABB{ { -1.4f, 4.0f, -0.4f }, { -0.6f, 5.4f, 0.4f } });
    assert(highGate.GetOpenness() == 0.0f);

    std::vector<StageObjectData> cells;
    for (int x : { 210, 225 }) for (int y : { 36, 34, 35 })
    {
        StageObjectData cell;
        cell.typeId = BossGate::RIGHT_TYPE_ID;
        cell.positionX = static_cast<float>(x);
        cell.positionY = static_cast<float>(y);
        cells.push_back(cell);
    }
    cells.push_back(StageObjectData{});
    auto gates = BossGate::FromObjects(cells);
    assert(gates.size() == 2);
    assert(gates[0].GetBounds().max.x - gates[0].GetBounds().min.x == 1.0f);
    assert(gates[0].GetBounds().min.y == 33.5f && gates[0].GetBounds().max.y == 36.5f);
    gates[0].Update(0.35f, AABB{ { 208.6f, 34.0f, -0.4f }, { 209.4f, 35.4f, 0.4f } });
    gates[0].Update(0.0f, AABB{ { 210.6f, 34.0f, -0.4f }, { 211.4f, 35.4f, 0.4f } });
    assert(gates[0].IsLocked() && !gates[1].IsLocked());
    cells[0].positionY = 40.0f;
    assert(BossGate::FromObjects(cells).size() == 3);

    TileTypeManager tileTypes;
    assert(tileTypes.LoadDefinitions("Data/Stage/TileTypes.json"));
    assert(tileTypes.GetTileInfo(301)->category == "Gate");
    assert(tileTypes.GetTileInfo(302)->defaultScale.x == 1.0f);
    std::filesystem::create_directories("obj/BossGateTests");
    json root = { {"formatVersion", 1}, {"stageName", "GateFixture"},
        {"objects", json::array({ { {"typeId", 301}, {"position", {{"x", 0}, {"y", 1}}},
            {"rotationDegrees", 0}, {"scale", {{"x", 1}, {"y", 1}}} } })} };
    const auto writeFixture = [&]() {
        std::ofstream("obj/BossGateTests/loading.stage.json") << root.dump(2);
    };
    StageData loaded;
    writeFixture();
    assert(StageLoader::Load("obj/BossGateTests/loading.stage.json", tileTypes, loaded));
    assert(BossGate::FromObjects(loaded.objects).size() == 1);
    root["objects"][0]["scale"]["x"] = 2;
    writeFixture();
    assert(!StageLoader::Load("obj/BossGateTests/loading.stage.json", tileTypes, loaded));
    assert(loaded.objects[0].scaleX == 1.0f);
    root["objects"][0]["scale"]["x"] = 1;
    root["objects"][0]["rotationDegrees"] = 90;
    writeFixture();
    assert(!StageLoader::Load("obj/BossGateTests/loading.stage.json", tileTypes, loaded));
    std::cout << "PASS: one-way crossing, mirrored direction, dash, stacked cells, independent gates and stage loading.\n";
}
