#include "StageLoader.h"
#include "TileTypeManager.h"
#include <cassert>
#include <fstream>
#include <iostream>

int main()
{
    TileTypeManager tileTypes;
    assert(tileTypes.LoadDefinitions("Data/Stage/TileTypes.json"));
    StageData stage;
    assert(StageLoader::Load("Data/Stage/ConvertedStage.stage.json", tileTypes, stage));
    assert(stage.cameraLimitZones.size() == 1);
    const auto& zone = stage.cameraLimitZones.front();
    assert(zone.minX == -8.0f && zone.maxX == 36.0f);
    assert(zone.minY == -8.0f && zone.maxY == 20.0f);
    const auto originalObjectCount = stage.objects.size();

    std::ifstream input("Data/Stage/ConvertedStage.stage.json");
    json root;
    input >> root;
    const auto writeFixture = [&root]() {
        std::ofstream output("obj/CameraBoundsTests/loading.stage.json");
        output << root.dump(2);
    };
    root["cameraBounds"] = json::object();
    writeFixture();
    assert(!StageLoader::Load("obj/CameraBoundsTests/loading.stage.json", tileTypes, stage));
    assert(stage.objects.size() == originalObjectCount && stage.cameraLimitZones.size() == 1);
    root["cameraBounds"] = json::array();
    writeFixture();
    assert(StageLoader::Load("obj/CameraBoundsTests/loading.stage.json", tileTypes, stage));
    assert(stage.cameraLimitZones.empty());
    root.erase("cameraBounds");
    writeFixture();
    assert(StageLoader::Load("obj/CameraBoundsTests/loading.stage.json", tileTypes, stage));
    assert(stage.cameraLimitZones.empty());
    std::cout << "PASS: actual stage camera bounds load; invalid array rejected; empty and omitted bounds supported.\n";
}
