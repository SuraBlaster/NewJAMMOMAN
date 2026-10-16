#include "../Source/StageLoader.h"
#include "../Source/TileTypeManager.h"
#include <json.hpp>
#include <fstream>
#include <cassert>
#include <iostream>

int main()
{
    TileTypeManager types;
    assert(types.LoadDefinitions("Data/Stage/TileTypes.json"));
    StageData stage;
    const char* path = "Data/Stage/LaboratoryCombinedCourse.stage.json";
    assert(StageLoader::Load(path, types, stage));
    assert(stage.cameraLimitZones.size() == 11);

    for (size_t i = 3; i < 7; ++i)
    {
        assert(stage.cameraLimitZones[i].cameraSection == "third");
    }

    for (size_t i = 7; i < 11; ++i)
    {
        assert(stage.cameraLimitZones[i].isSectionTrigger);
    }

    nlohmann::json root;
    std::ifstream(path) >> root;
    const auto checkInvalid = [&](const nlohmann::json& invalid)
    {
        const char* fixture = "obj/invalid-camera-section.stage.json";
        {
            std::ofstream output(fixture);
            output << invalid.dump();
        }

        assert(!StageLoader::Load(fixture, types, stage));
        assert(stage.cameraLimitZones.size() == 11);
    };
    auto invalid = root;
    invalid["cameraBounds"][0]["cameraSection"] = 3;
    checkInvalid(invalid);
    invalid = root;
    invalid["cameraBounds"][7]["cameraSection"] = "missing";
    checkInvalid(invalid);
    invalid = root;
    invalid["cameraBounds"][7]["isSectionTrigger"] = "true";
    checkInvalid(invalid);
    std::cout << "PASS: real stage sections loaded; invalid metadata and missing targets rejected\n";
}
