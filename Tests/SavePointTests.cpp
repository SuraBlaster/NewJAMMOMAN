#include <cassert>
#include <fstream>
#include <iostream>
#include "StageLoader.h"
#include "TileTypeManager.h"
#include "Converter.h"

int main()
{
    stage_converter::ConverterOptions options;
    options.inputPath = "Tools/StageConverter/Sample/SavePointsSample.tmj";
    options.outputPath = "obj/SavePoints.stage.json";
    options.pixelsPerUnit = 32;
    stage_converter::TiledStageConverter converter;
    const auto result = converter.Convert(options);
    assert(result.convertedSavePointCount == 1);
    std::ifstream input(options.outputPath);
    json root; input >> root;
    const auto& point = root.at("savePoints").at(0);
    assert(point.at("minX") == 2 && point.at("maxX") == 4);
    assert(point.at("minY") == 0 && point.at("maxY") == 2);
    std::ifstream mapInput(options.inputPath);
    json map; mapInput >> map;
    map["layers"].back()["layers"][0]["objects"][0]["rotation"] = 15;
    options.inputPath = "obj/InvalidSavePoint.tmj";
    // External tileset paths are relative, so keep the original base directory.
    map["tilesets"] = json::array();
    map["layers"].erase(map["layers"].begin(), map["layers"].end() - 1);
    { std::ofstream out(options.inputPath); out << map; }
    bool rejected = false;
    try { converter.Convert(options); } catch (const std::exception&) { rejected = true; }
    assert(rejected);
    // Hidden bounds must not leave unusable section triggers in the output.
    json trigger = {{"id", 9010}, {"x", 0}, {"y", 0}, {"width", 32}, {"height", 32},
                    {"properties", json::array({{{"name", "cameraSection"}, {"type", "string"}, {"value", "test"}}})}};
    json boundsLayer = {{"type", "objectgroup"}, {"name", "CameraBounds"},
                        {"visible", false}, {"objects", json::array({trigger})}};
    json triggerLayer = {{"type", "objectgroup"}, {"name", "CameraSectionTriggers"},
                         {"objects", json::array({trigger})}};
    map["layers"] = json::array({boundsLayer, triggerLayer});
    { std::ofstream out(options.inputPath); out << map; }
    assert(converter.Convert(options).convertedCameraBoundsCount == 0);
    map["layers"][0]["visible"] = true;
    { std::ofstream out(options.inputPath); out << map; }
    assert(converter.Convert(options).convertedCameraBoundsCount == 2);
    map["layers"][0]["objects"][0]["properties"][0]["value"] = "other";
    { std::ofstream out(options.inputPath); out << map; }
    rejected = false;
    try { converter.Convert(options); } catch (const std::exception&) { rejected = true; }
    assert(rejected);
    TileTypeManager types;
    assert(types.LoadDefinitions("Data/Stage/TileTypes.json"));
    // Use real terrain definitions while keeping the converted save points.
    std::ifstream actual("Data/Stage/LaboratoryCombinedCourse.stage.json");
    json stage; actual >> stage;
    stage["savePoints"] = root["savePoints"];
    stage["cameraBounds"] = json::array();
    auto write = [&]() { std::ofstream out(options.outputPath); out << stage; };
    StageData loaded;
    write(); assert(StageLoader::Load(options.outputPath.string(), types, loaded));
    assert(loaded.savePoints.size() == 1 && loaded.savePoints[0].minX == 2);
    stage["savePoints"][0]["maxX"] = 2;
    write(); assert(!StageLoader::Load(options.outputPath.string(), types, loaded));
    assert(loaded.savePoints.size() == 1);
    stage["savePoints"] = json::object();
    write(); assert(!StageLoader::Load(options.outputPath.string(), types, loaded));
    stage.erase("savePoints");
    write(); assert(StageLoader::Load(options.outputPath.string(), types, loaded));
    assert(loaded.savePoints.empty());
    std::cout << "PASS: save point conversion, offsets, Y flip, visibility, validation and legacy loading.\n";
}
