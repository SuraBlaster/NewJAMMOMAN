#include <DirectXMath.h>

#include <string>
#include <fstream>
#include "SetStage.h"

using json = nlohmann::json;

void SetStage::JsonOutput(std::string filename, outputData data)
{
    json j = {
        {"link", data.filename},
        {"id", data.id},
    };
    std::ofstream ofs(filename);
    ofs << j.dump(4);
    ofs.close();
}
