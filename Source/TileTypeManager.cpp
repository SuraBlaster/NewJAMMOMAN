#include "TileTypeManager.h"

#include <algorithm>
#include <array>
#include <cctype>
#include <cmath>
#include <filesystem>
#include <fstream>
#include <iostream>
#include <string_view>
#include <system_error>
#include <utility>

namespace
{
    constexpr int SupportedFormatVersion = 1;

    constexpr std::array<std::string_view, 6> RequiredDefinitionFields
    {
        "typeId",
        "name",
        "category",
        "modelPath",
        "collision",
        "defaultScale"
    };

    constexpr std::array<std::string_view, 3> ScaleFields
    {
        "x",
        "y",
        "z"
    };

    constexpr std::array<std::string_view, 5> SupportedCategories
    {
        "Ground",
        "Wall",
        "Decoration",
        "Hazard",
        "CollisionOnly"
    };

    void PrintDefinitionError(const std::string& message)
    {
        std::cerr << "TileTypes definition error: " << message << std::endl;
    }
}

// Validate one element in the definitions array before converting it to TileInfo.
bool TileTypeManager::DefinitionsVerification(const json& item)
{
    if (!item.is_object())
    {
        PrintDefinitionError("each definition must be a JSON object.");
        return false;
    }

    for (const std::string_view fieldName : RequiredDefinitionFields)
    {
        if (item.count(std::string(fieldName)) == 0)
        {
            PrintDefinitionError(
                "required field is missing: " + std::string(fieldName));
            return false;
        }
    }

    if (!item.at("typeId").is_number_integer())
    {
        PrintDefinitionError("typeId must be an integer.");
        return false;
    }

    if (!item.at("name").is_string())
    {
        PrintDefinitionError("name must be a string.");
        return false;
    }

    if (!item.at("category").is_string())
    {
        PrintDefinitionError("category must be a string.");
        return false;
    }

    if (!item.at("modelPath").is_string())
    {
        PrintDefinitionError("modelPath must be a string.");
        return false;
    }

    if (!item.at("collision").is_boolean())
    {
        PrintDefinitionError("collision must be a boolean.");
        return false;
    }

    if (!item.at("defaultScale").is_object())
    {
        PrintDefinitionError("defaultScale must be a JSON object.");
        return false;
    }

    const int typeId = item.at("typeId").get<int>();
    if (typeId <= 0)
    {
        PrintDefinitionError("typeId must be greater than zero.");
        return false;
    }

    const std::string name = item.at("name").get<std::string>();
    if (name.empty())
    {
        PrintDefinitionError("name must not be empty.");
        return false;
    }

    const std::string category = item.at("category").get<std::string>();
    const bool isSupportedCategory =
        std::find(
            SupportedCategories.begin(),
            SupportedCategories.end(),
            category) != SupportedCategories.end();

    if (!isSupportedCategory)
    {
        PrintDefinitionError("category contains an unsupported value.");
        return false;
    }

    const std::string modelPath = item.at("modelPath").get<std::string>();
    if (modelPath.empty())
    {
        PrintDefinitionError("modelPath must not be empty.");
        return false;
    }

    const json& defaultScale = item.at("defaultScale");
    for (const std::string_view scaleField : ScaleFields)
    {
        const std::string fieldName(scaleField);

        if (defaultScale.count(fieldName) == 0)
        {
            PrintDefinitionError(
                "defaultScale is missing field: " + fieldName);
            return false;
        }

        const json& scaleValue = defaultScale.at(fieldName);
        if (!scaleValue.is_number())
        {
            PrintDefinitionError(
                "defaultScale." + fieldName + " must be a number.");
            return false;
        }

        const float scale = scaleValue.get<float>();
        if (!std::isfinite(scale))
        {
            PrintDefinitionError(
                "defaultScale." + fieldName + " must be finite.");
            return false;
        }

        if (scale <= 0.0f)
        {
            PrintDefinitionError(
                "defaultScale." + fieldName + " must be greater than zero.");
            return false;
        }
    }

    if (item.count("collisionModelPath") != 0)
    {
        if (!item.at("collisionModelPath").is_string() ||
            !ValidateModelPath(item.at("collisionModelPath").get<std::string>()))
        {
            PrintDefinitionError("collisionModelPath must be an existing model below Data/Model.");
            return false;
        }
    }
    return ValidateModelPath(modelPath);
}

// Accept only an existing glTF model below the project's Data/Model directory.
bool TileTypeManager::ValidateModelPath(const std::string& modelPath)
{
    if (modelPath.empty())
    {
        PrintDefinitionError("modelPath must not be empty.");
        return false;
    }

    const std::filesystem::path filePath(modelPath);

    if (filePath.is_absolute())
    {
        PrintDefinitionError("modelPath must not be an absolute path: " + modelPath);
        return false;
    }

    if (filePath.has_root_name())
    {
        PrintDefinitionError(
            "modelPath must not contain a drive or network root: " + modelPath);
        return false;
    }

    if (filePath.has_root_directory())
    {
        PrintDefinitionError(
            "modelPath must not begin at a root directory: " + modelPath);
        return false;
    }

    for (const std::filesystem::path& pathElement : filePath)
    {
        if (pathElement == "..")
        {
            PrintDefinitionError(
                "modelPath must not contain '..': " + modelPath);
            return false;
        }
    }

    const std::filesystem::path normalizedPath = filePath.lexically_normal();
    auto pathIterator = normalizedPath.begin();

    if (pathIterator == normalizedPath.end() || *pathIterator != "Data")
    {
        PrintDefinitionError(
            "modelPath must be below Data/Model: " + modelPath);
        return false;
    }

    ++pathIterator;
    if (pathIterator == normalizedPath.end() || *pathIterator != "Model")
    {
        PrintDefinitionError(
            "modelPath must be below Data/Model: " + modelPath);
        return false;
    }

    std::string extension = normalizedPath.extension().string();
    std::transform(
        extension.begin(),
        extension.end(),
        extension.begin(),
        [](unsigned char character)
        {
            return static_cast<char>(std::tolower(character));
        });

    if (extension != ".gltf" && extension != ".glb")
    {
        PrintDefinitionError(
            "modelPath extension must be .gltf or .glb: " + modelPath);
        return false;
    }

    std::error_code fileSystemError;
    const bool fileExists = std::filesystem::exists(normalizedPath, fileSystemError);
    if (fileSystemError)
    {
        PrintDefinitionError(
            "failed to check modelPath: " + modelPath);
        return false;
    }

    if (!fileExists)
    {
        PrintDefinitionError(
            "modelPath does not exist: " + modelPath);
        return false;
    }

    const bool isRegularFile =
        std::filesystem::is_regular_file(normalizedPath, fileSystemError);
    if (fileSystemError)
    {
        PrintDefinitionError(
            "failed to inspect modelPath: " + modelPath);
        return false;
    }

    if (!isRegularFile)
    {
        PrintDefinitionError(
            "modelPath must refer to a file: " + modelPath);
        return false;
    }

    return true;
}

// Load all definitions transactionally so a failed reload keeps the old data intact.
bool TileTypeManager::LoadDefinitions(const std::string& jsonPath)
{
    try
    {
        std::ifstream inputFile(jsonPath);
        if (!inputFile.is_open())
        {
            std::cerr << "Failed to open tile definition file: "
                << jsonPath << std::endl;
            return false;
        }

        json root;
        inputFile >> root;

        if (!root.is_object())
        {
            std::cerr << "Tile definition root must be a JSON object."
                << std::endl;
            return false;
        }

        if (root.count("formatVersion") == 0)
        {
            std::cerr << "Tile definition formatVersion is missing."
                << std::endl;
            return false;
        }

        if (!root.at("formatVersion").is_number_integer())
        {
            std::cerr << "Tile definition formatVersion must be an integer."
                << std::endl;
            return false;
        }

        const int formatVersion = root.at("formatVersion").get<int>();
        if (formatVersion != SupportedFormatVersion)
        {
            std::cerr << "Unsupported tile definition formatVersion: "
                << formatVersion << std::endl;
            return false;
        }

        if (root.count("definitions") == 0)
        {
            std::cerr << "Tile definition definitions array is missing."
                << std::endl;
            return false;
        }

        const json& definitions = root.at("definitions");
        if (!definitions.is_array())
        {
            std::cerr << "Tile definition definitions must be a JSON array."
                << std::endl;
            return false;
        }

        std::unordered_map<int, TileInfo> loadedDefinitions;
        loadedDefinitions.reserve(definitions.size());

        for (const json& item : definitions)
        {
            if (!DefinitionsVerification(item))
            {
                return false;
            }

            TileInfo tileInfo;
            tileInfo.typeId = item.at("typeId").get<int>();
            tileInfo.name = item.at("name").get<std::string>();
            tileInfo.category = item.at("category").get<std::string>();
            tileInfo.modelPath = item.at("modelPath").get<std::string>();
            tileInfo.collision = item.at("collision").get<bool>();
            tileInfo.collisionModelPath = item.value("collisionModelPath", std::string{});

            const json& defaultScale = item.at("defaultScale");
            tileInfo.defaultScale.x = defaultScale.at("x").get<float>();
            tileInfo.defaultScale.y = defaultScale.at("y").get<float>();
            tileInfo.defaultScale.z = defaultScale.at("z").get<float>();

            const auto insertionResult =
                loadedDefinitions.emplace(tileInfo.typeId, std::move(tileInfo));

            if (!insertionResult.second)
            {
                std::cerr << "Duplicate tile definition typeId: "
                    << item.at("typeId").get<int>() << std::endl;
                return false;
            }
        }

        idToInfoMap.swap(loadedDefinitions);
        return true;
    }
    catch (const json::parse_error& exception)
    {
        std::cerr << "Failed to parse tile definition JSON: "
            << exception.what() << std::endl;
        return false;
    }
    catch (const json::exception& exception)
    {
        std::cerr << "Invalid tile definition JSON: "
            << exception.what() << std::endl;
        return false;
    }
    catch (const std::exception& exception)
    {
        std::cerr << "Failed to load tile definitions: "
            << exception.what() << std::endl;
        return false;
    }
}
