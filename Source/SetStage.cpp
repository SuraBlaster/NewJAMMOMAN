#include "SetStage.h"
#include <fstream>
#include <string>
#include <DirectXMath.h>

using json = nlohmann::json;

void SetStage::JsonOutput(std::string filename, outputData data)
{
    json j =
    {
        { "link", data.filename},
        { "id", data.id},
    };
    std::ofstream ofs(filename);
    ofs << j.dump(4);
    ofs.close();
}
