#pragma once
#include <json.hpp>

class SetStage
{
public:
    struct outputData
    {
        std::string filename;
        int id;
    };

    static void JsonOutput(std::string filename, outputData data);  
};


