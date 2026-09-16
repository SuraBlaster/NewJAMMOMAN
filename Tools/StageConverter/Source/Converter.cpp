#include "Converter.h"

#include <algorithm>
#include <array>
#include <cmath>
#include <cstdint>
#include <fstream>
#include <iomanip>
#include <limits>
#include <ostream>
#include <sstream>
#include <stdexcept>
#include <string_view>
#include <utility>
#include <unordered_map>
#include <vector>

#include <json.hpp>

namespace stage_converter
{
    namespace
    {
        using json = nlohmann::json;

        constexpr int kOutputFormatVersion = 1;
        constexpr int kOutputIndentSpaces = 2;
        constexpr int kMinimumTypeId = 1;
        constexpr double kDefaultRotationDegrees = 0.0;
        constexpr double kDefaultScale = 1.0;
        constexpr double kZeroNormalizationThreshold = 1.0e-12;
        constexpr std::uint32_t kEmptyGlobalTileId = 0;
        constexpr std::uint32_t kHorizontalFlipFlag = 0x80000000U;
        constexpr std::uint32_t kVerticalFlipFlag = 0x40000000U;
        constexpr std::uint32_t kDiagonalFlipFlag = 0x20000000U;
        constexpr std::uint32_t kHexagonalRotationFlag = 0x10000000U;
        constexpr std::uint32_t kTileTransformFlags =
            kHorizontalFlipFlag |
            kVerticalFlipFlag |
            kDiagonalFlipFlag |
            kHexagonalRotationFlag;
        constexpr std::uint32_t kGlobalTileIdMask = ~kTileTransformFlags;

        constexpr std::string_view kInputOption = "--input";
        constexpr std::string_view kOutputOption = "--output";
        constexpr std::string_view kPixelsPerUnitOption = "--pixels-per-unit";
        constexpr std::string_view kStageNameOption = "--stage-name";
        constexpr std::string_view kHelpOption = "--help";
        constexpr std::string_view kShortHelpOption = "-h";

        constexpr std::string_view kTypeIdProperty = "typeId";
        constexpr std::string_view kScaleXProperty = "scaleX";
        constexpr std::string_view kScaleYProperty = "scaleY";
        constexpr std::string_view kStageNameProperty = "stageName";
        constexpr std::string_view kEnemyTypeProperty = "enemyType";
        constexpr std::string_view kDirectionProperty = "direction";
        constexpr std::string_view kFallRespawnZonesLayerName = "FallRespawnZones";
        constexpr std::string_view kBossEncounterZoneLayerName = "BossEncounterZone";
        constexpr std::string_view kBossApproachCameraZoneLayerName = "BossApproachCameraZone";
        constexpr std::string_view kDefaultEnemyDirection = "Right";

        // ゲーム側に実装済みの敵名だけを許可し、入力ミスを変換時に検出する。
        constexpr std::array<std::string_view, 8> kSupportedEnemyTypes{
            "Wave",
            "Scatter",
            "Mage",
            "Fan",
            "Hover",
            "Fly",
            "Egg",
            "Boss"
        };

        // タイルセット内の1タイルから、ゲームへ出力する情報だけを保持する。
        struct TileDefinition
        {
            int typeId = 0;
            double scaleX = kDefaultScale;
            double scaleY = kDefaultScale;
        };

        // firstGidはマップ内のGIDをタイルセット内IDへ戻すために使用する。
        struct TilesetDefinition
        {
            std::uint32_t firstGid = 0;
            std::string name;
            std::unordered_map<std::uint32_t, TileDefinition> tilesByLocalId;
            // 敵用タイルは地形typeIdと分離し、オブジェクトレイヤー専用として扱う。
            std::unordered_map<std::uint32_t, std::string> enemyTypesByLocalId;
        };

        struct TileMapSettings
        {
            int tileWidth = 0;
            int tileHeight = 0;
            bool isOrthogonal = false;
        };

        // -0.0の出力を避け、比較しやすいJSONに揃える。
        double NormalizeZero(double value)
        {
            return std::abs(value) < kZeroNormalizationThreshold ? 0.0 : value;
        }

        json CreateOutputObject(
            int typeId,
            double gameX,
            double gameY,
            double rotationDegrees,
            double scaleX,
            double scaleY)
        {
            return json{
                { "typeId", typeId },
                { "position", {
                    { "x", NormalizeZero(gameX) },
                    { "y", NormalizeZero(gameY) }
                } },
                { "rotationDegrees", NormalizeZero(rotationDegrees) },
                { "scale", {
                    { "x", scaleX },
                    { "y", scaleY }
                } }
            };
        }

        // Tiledの敵オブジェクト1件を、ゲーム側のenemies配列形式へ整形する。
        json CreateOutputEnemy(
            const std::string& enemyType,
            double gameX,
            double gameY,
            const std::string& direction)
        {
            return json{
                { "enemyType", enemyType },
                { "position", {
                    { "x", NormalizeZero(gameX) },
                    { "y", NormalizeZero(gameY) }
                } },
                { "direction", direction }
            };
        }

        // Tiled上の長方形を、ゲーム座標系の上下左右の境界として保存する。
        json CreateOutputRectangleZone(
            double minimumX,
            double maximumX,
            double minimumY,
            double maximumY)
        {
            return json{
                { "minX", NormalizeZero(minimumX) },
                { "maxX", NormalizeZero(maximumX) },
                { "minY", NormalizeZero(minimumY) },
                { "maxY", NormalizeZero(maximumY) }
            };
        }

        std::string PathForMessage(const std::filesystem::path& path)
        {
            return path.u8string();
        }

        json ReadJsonFile(const std::filesystem::path& path)
        {
            std::ifstream input(path, std::ios::binary);
            if (!input.is_open())
            {
                throw std::runtime_error(
                    "入力ファイルを開けません。パス: " + PathForMessage(path));
            }

            try
            {
                json root;
                input >> root;
                return root;
            }
            catch (const json::parse_error& error)
            {
                throw std::runtime_error(
                    "Tiled JSONの構文が正しくありません。パス: " +
                    PathForMessage(path) + " 詳細: " + error.what());
            }
        }

        void WriteJsonFile(const std::filesystem::path& path, const json& root)
        {
            const std::filesystem::path parentPath = path.parent_path();
            if (!parentPath.empty())
            {
                std::error_code directoryError;
                std::filesystem::create_directories(parentPath, directoryError);
                if (directoryError)
                {
                    throw std::runtime_error(
                        "出力先ディレクトリを作成できません。パス: " +
                        PathForMessage(parentPath) + " 詳細: " + directoryError.message());
                }
            }

            std::ofstream output(path, std::ios::binary | std::ios::trunc);
            if (!output.is_open())
            {
                throw std::runtime_error(
                    "出力ファイルを開けません。パス: " + PathForMessage(path));
            }

            output << root.dump(kOutputIndentSpaces) << '\n';
            if (!output)
            {
                throw std::runtime_error(
                    "出力ファイルの書き込みに失敗しました。パス: " + PathForMessage(path));
            }
        }

        std::string ObjectDescription(const json& object)
        {
            std::ostringstream description;
            description << "object";

            const auto idIterator = object.find("id");
            if (idIterator != object.end() && idIterator->is_number_integer())
            {
                description << "[id=" << idIterator->get<int>() << ']';
            }

            const auto nameIterator = object.find("name");
            if (nameIterator != object.end() && nameIterator->is_string())
            {
                const std::string name = nameIterator->get<std::string>();
                if (!name.empty())
                {
                    description << "[name=" << name << ']';
                }
            }

            return description.str();
        }

        const json* FindPropertyValue(const json& owner, std::string_view propertyName)
        {
            const auto propertiesIterator = owner.find("properties");
            if (propertiesIterator == owner.end())
            {
                return nullptr;
            }
            if (!propertiesIterator->is_array())
            {
                throw std::runtime_error("propertiesがJSON配列ではありません。");
            }

            for (const json& property : *propertiesIterator)
            {
                if (!property.is_object())
                {
                    throw std::runtime_error("properties内にJSONオブジェクト以外の値があります。");
                }

                const auto nameIterator = property.find("name");
                if (nameIterator == property.end() || !nameIterator->is_string())
                {
                    throw std::runtime_error("プロパティに文字列のnameがありません。");
                }

                if (nameIterator->get<std::string>() == std::string(propertyName))
                {
                    const auto valueIterator = property.find("value");
                    if (valueIterator == property.end())
                    {
                        throw std::runtime_error(
                            "プロパティにvalueがありません。プロパティ名: " +
                            std::string(propertyName));
                    }
                    return &(*valueIterator);
                }
            }

            return nullptr;
        }

        // Tiledのカスタムプロパティから省略可能な文字列値を取得する。
        std::optional<std::string> ReadOptionalStringProperty(
            const json& owner,
            std::string_view propertyName,
            const std::string& context)
        {
            const json* value = FindPropertyValue(owner, propertyName);
            if (value == nullptr)
            {
                return std::nullopt;
            }
            if (!value->is_string())
            {
                throw std::runtime_error(
                    context + "の" + std::string(propertyName) +
                    "プロパティが文字列ではありません。");
            }
            return value->get<std::string>();
        }

        bool IsSupportedEnemyType(std::string_view enemyType)
        {
            return std::find(
                kSupportedEnemyTypes.begin(),
                kSupportedEnemyTypes.end(),
                enemyType) != kSupportedEnemyTypes.end();
        }

        // ゲーム側に生成処理がない敵名を、実行時ではなく変換時に検出する。
        void ValidateEnemyType(const std::string& enemyType, const std::string& context)
        {
            if (!IsSupportedEnemyType(enemyType))
            {
                throw std::runtime_error(
                    context + "のenemyTypeが未対応です。値: " + enemyType);
            }
        }

        void ValidateEnemyDirection(const std::string& direction, const std::string& context)
        {
            if (direction != "Left" && direction != "Right")
            {
                throw std::runtime_error(
                    context + "のdirectionはLeftまたはRightにしてください。");
            }
        }

        double ReadRequiredNumber(
            const json& owner,
            std::string_view key,
            const std::string& context)
        {
            const auto iterator = owner.find(std::string(key));
            if (iterator == owner.end() || !iterator->is_number())
            {
                throw std::runtime_error(
                    context + "の" + std::string(key) + "が数値ではありません。");
            }

            const double value = iterator->get<double>();
            if (!std::isfinite(value))
            {
                throw std::runtime_error(
                    context + "の" + std::string(key) + "が有限値ではありません。");
            }
            return value;
        }

        double ReadOptionalNumber(
            const json& owner,
            std::string_view key,
            double defaultValue,
            const std::string& context)
        {
            const auto iterator = owner.find(std::string(key));
            if (iterator == owner.end())
            {
                return defaultValue;
            }
            if (!iterator->is_number())
            {
                throw std::runtime_error(
                    context + "の" + std::string(key) + "が数値ではありません。");
            }

            const double value = iterator->get<double>();
            if (!std::isfinite(value))
            {
                throw std::runtime_error(
                    context + "の" + std::string(key) + "が有限値ではありません。");
            }
            return value;
        }

        std::int64_t ReadRequiredInteger(
            const json& owner,
            std::string_view key,
            const std::string& context)
        {
            const auto iterator = owner.find(std::string(key));
            if (iterator == owner.end() || !iterator->is_number_integer())
            {
                throw std::runtime_error(
                    context + "の" + std::string(key) + "が整数ではありません。");
            }

            if (iterator->is_number_unsigned())
            {
                const std::uint64_t value = iterator->get<std::uint64_t>();
                if (value > static_cast<std::uint64_t>(std::numeric_limits<std::int64_t>::max()))
                {
                    throw std::runtime_error(
                        context + "の" + std::string(key) + "が整数の有効範囲外です。");
                }
                return static_cast<std::int64_t>(value);
            }
            return iterator->get<std::int64_t>();
        }

        std::int64_t ReadOptionalInteger(
            const json& owner,
            std::string_view key,
            std::int64_t defaultValue,
            const std::string& context)
        {
            if (owner.find(std::string(key)) == owner.end())
            {
                return defaultValue;
            }
            return ReadRequiredInteger(owner, key, context);
        }

        double ReadOptionalNumberProperty(
            const json& owner,
            std::string_view propertyName,
            double defaultValue,
            const std::string& context)
        {
            const json* value = FindPropertyValue(owner, propertyName);
            if (value == nullptr)
            {
                return defaultValue;
            }
            if (!value->is_number())
            {
                throw std::runtime_error(
                    context + "の" + std::string(propertyName) +
                    "プロパティが数値ではありません。");
            }

            const double number = value->get<double>();
            if (!std::isfinite(number))
            {
                throw std::runtime_error(
                    context + "の" + std::string(propertyName) +
                    "プロパティが有限値ではありません。");
            }
            return number;
        }

        std::optional<int> ReadOptionalTypeId(const json& object, const std::string& context)
        {
            const json* value = FindPropertyValue(object, kTypeIdProperty);
            if (value == nullptr)
            {
                return std::nullopt;
            }
            if (!value->is_number_integer())
            {
                throw std::runtime_error(
                    context + "のtypeIdプロパティが整数ではありません。");
            }

            const std::int64_t typeId = value->get<std::int64_t>();
            if (typeId < kMinimumTypeId || typeId > std::numeric_limits<int>::max())
            {
                throw std::runtime_error(
                    context + "のtypeIdプロパティが有効範囲外です。");
            }
            return static_cast<int>(typeId);
        }

        bool IsVisible(const json& owner, const std::string& context)
        {
            const auto iterator = owner.find("visible");
            if (iterator == owner.end())
            {
                return true;
            }
            if (!iterator->is_boolean())
            {
                throw std::runtime_error(context + "のvisibleが真偽値ではありません。");
            }
            return iterator->get<bool>();
        }

        // 専用ゾーンレイヤー内の長方形1件を、ゲーム座標系の境界データへ変換する。
        std::optional<json> ConvertRectangleZone(
            const json& object,
            double accumulatedOffsetX,
            double accumulatedOffsetY,
            double pixelsPerUnit)
        {
            if (!object.is_object())
            {
                throw std::runtime_error("ゾーンレイヤーのobjects配列にJSONオブジェクト以外の値があります。");
            }

            const std::string context = ObjectDescription(object);
            if (!IsVisible(object, context))
            {
                return std::nullopt;
            }

            // タイル・点・楕円・多角形などを誤って置いた場合は、曖昧に変換せず入力ミスとして知らせる。
            constexpr std::array<std::string_view, 5> kNonRectangleShapeKeys{
                "gid", "ellipse", "point", "polygon", "polyline"
            };
            for (const std::string_view key : kNonRectangleShapeKeys)
            {
                if (object.find(std::string(key)) != object.end())
                {
                    throw std::runtime_error(
                        context + "は長方形ではありません。ゾーン用レイヤーには長方形を配置してください。");
                }
            }

            const double tiledX = ReadRequiredNumber(object, "x", context) + accumulatedOffsetX;
            const double tiledY = ReadRequiredNumber(object, "y", context) + accumulatedOffsetY;
            const double width = ReadRequiredNumber(object, "width", context);
            const double height = ReadRequiredNumber(object, "height", context);
            if (width <= 0.0 || height <= 0.0)
            {
                throw std::runtime_error(context + "のwidthとheightは0より大きい必要があります。");
            }

            const double rotation = ReadOptionalNumber(
                object,
                "rotation",
                kDefaultRotationDegrees,
                context);
            if (std::abs(rotation) > kZeroNormalizationThreshold)
            {
                throw std::runtime_error(context + "の長方形ゾーンは回転できません。");
            }

            const double tiledRight = tiledX + width;
            const double tiledBottom = tiledY + height;

            // TiledはYが下向きなので、上下を入れ替えながらゲームのY上向き座標へ変換する。
            return CreateOutputRectangleZone(
                tiledX / pixelsPerUnit,
                tiledRight / pixelsPerUnit,
                -tiledBottom / pixelsPerUnit,
                -tiledY / pixelsPerUnit);
        }

        std::optional<json> ConvertObject(
            const json& object,
            double accumulatedOffsetX,
            double accumulatedOffsetY,
            double pixelsPerUnit)
        {
            if (!object.is_object())
            {
                throw std::runtime_error("objects配列にJSONオブジェクト以外の値があります。");
            }

            const std::string context = ObjectDescription(object);
            if (!IsVisible(object, context))
            {
                return std::nullopt;
            }

            // typeIdを持たないオブジェクトは、編集用マーカーなどとして出力対象外にする。
            const std::optional<int> typeId = ReadOptionalTypeId(object, context);
            if (!typeId.has_value())
            {
                return std::nullopt;
            }

            const double tiledX = ReadRequiredNumber(object, "x", context) + accumulatedOffsetX;
            const double tiledY = ReadRequiredNumber(object, "y", context) + accumulatedOffsetY;
            const double tiledRotation = ReadOptionalNumber(
                object,
                "rotation",
                kDefaultRotationDegrees,
                context);

            const double scaleX = ReadOptionalNumberProperty(
                object,
                kScaleXProperty,
                kDefaultScale,
                context);
            const double scaleY = ReadOptionalNumberProperty(
                object,
                kScaleYProperty,
                kDefaultScale,
                context);

            if (scaleX <= 0.0 || scaleY <= 0.0)
            {
                throw std::runtime_error(
                    context + "のscaleXとscaleYは0より大きい必要があります。");
            }

            // TiledはYが下向き、ゲームはYが上向きなので符号を反転する。
            const double gameX = tiledX / pixelsPerUnit;
            const double gameY = -tiledY / pixelsPerUnit;

            // Y軸反転に合わせ、Tiledの時計回り角度をゲーム側の角度へ変換する。
            const double gameRotation = -tiledRotation;

            return CreateOutputObject(
                *typeId,
                gameX,
                gameY,
                gameRotation,
                scaleX,
                scaleY);
        }

        std::uint32_t ToUint32(
            std::int64_t value,
            std::int64_t minimum,
            const std::string& context)
        {
            if (value < minimum ||
                static_cast<std::uint64_t>(value) > std::numeric_limits<std::uint32_t>::max())
            {
                throw std::runtime_error(context + "が有効範囲外です。");
            }
            return static_cast<std::uint32_t>(value);
        }

        TileDefinition ReadTileDefinition(const json& tile, const std::string& context)
        {
            const std::optional<int> typeId = ReadOptionalTypeId(tile, context);
            if (!typeId.has_value())
            {
                throw std::runtime_error(context + "にtypeIdがありません。");
            }

            TileDefinition definition;
            definition.typeId = *typeId;
            definition.scaleX = ReadOptionalNumberProperty(
                tile,
                kScaleXProperty,
                kDefaultScale,
                context);
            definition.scaleY = ReadOptionalNumberProperty(
                tile,
                kScaleYProperty,
                kDefaultScale,
                context);
            if (definition.scaleX <= 0.0 || definition.scaleY <= 0.0)
            {
                throw std::runtime_error(
                    context + "のscaleXとscaleYは0より大きい必要があります。");
            }
            return definition;
        }

        TilesetDefinition ReadTilesetDefinition(
            const json& mapTileset,
            const std::filesystem::path& mapDirectory)
        {
            if (!mapTileset.is_object())
            {
                throw std::runtime_error("tilesets配列にJSONオブジェクト以外の値があります。");
            }

            const std::uint32_t firstGid = ToUint32(
                ReadRequiredInteger(mapTileset, "firstgid", "tileset"),
                1,
                "tilesetのfirstgid");

            json loadedTileset;
            const json* tilesetRoot = &mapTileset;
            const auto sourceIterator = mapTileset.find("source");
            if (sourceIterator != mapTileset.end())
            {
                if (!sourceIterator->is_string())
                {
                    throw std::runtime_error("tilesetのsourceが文字列ではありません。");
                }

                const std::filesystem::path sourcePath =
                    std::filesystem::u8path(sourceIterator->get<std::string>());
                if (sourcePath.extension() == L".tsx")
                {
                    throw std::runtime_error(
                        "XML形式の外部タイルセット(.tsx)には対応していません。"
                        "TiledでJSON形式(.tsj)として保存してください。パス: " +
                        PathForMessage(sourcePath));
                }

                loadedTileset = ReadJsonFile((mapDirectory / sourcePath).lexically_normal());
                if (!loadedTileset.is_object())
                {
                    throw std::runtime_error("外部タイルセットのルートがオブジェクトではありません。");
                }
                tilesetRoot = &loadedTileset;
            }

            TilesetDefinition result;
            result.firstGid = firstGid;
            const auto nameIterator = tilesetRoot->find("name");
            if (nameIterator != tilesetRoot->end())
            {
                if (!nameIterator->is_string())
                {
                    throw std::runtime_error("タイルセットのnameが文字列ではありません。");
                }
                result.name = nameIterator->get<std::string>();
            }

            const std::string context = "tileset[name=" + result.name + ']';
            const auto tilesIterator = tilesetRoot->find("tiles");
            if (tilesIterator == tilesetRoot->end())
            {
                return result;
            }
            if (!tilesIterator->is_array())
            {
                throw std::runtime_error(context + "のtilesがJSON配列ではありません。");
            }

            for (const json& tile : *tilesIterator)
            {
                if (!tile.is_object())
                {
                    throw std::runtime_error(context + "のtiles内にオブジェクト以外があります。");
                }

                const std::uint32_t localId = ToUint32(
                    ReadRequiredInteger(tile, "id", context + "のtile"),
                    0,
                    context + "のtile.id");
                const std::string tileContext =
                    context + "のtile[id=" + std::to_string(localId) + ']';

                const bool hasTypeId = FindPropertyValue(tile, kTypeIdProperty) != nullptr;
                const std::optional<std::string> enemyType = ReadOptionalStringProperty(
                    tile,
                    kEnemyTypeProperty,
                    tileContext);

                // 地形と敵の両方を表すタイルは用途が曖昧になるため禁止する。
                if (hasTypeId && enemyType.has_value())
                {
                    throw std::runtime_error(
                        tileContext + "にtypeIdとenemyTypeの両方が設定されています。");
                }

                if (hasTypeId)
                {
                    const auto insertResult = result.tilesByLocalId.emplace(
                        localId,
                        ReadTileDefinition(tile, tileContext));
                    if (!insertResult.second)
                    {
                        throw std::runtime_error(tileContext + "が重複しています。");
                    }
                }
                else if (enemyType.has_value())
                {
                    ValidateEnemyType(*enemyType, tileContext);
                    const auto insertResult = result.enemyTypesByLocalId.emplace(
                        localId,
                        *enemyType);
                    if (!insertResult.second)
                    {
                        throw std::runtime_error(tileContext + "が重複しています。");
                    }
                }
            }
            return result;
        }

        std::vector<TilesetDefinition> LoadTilesets(
            const json& mapRoot,
            const std::filesystem::path& mapPath)
        {
            const auto tilesetsIterator = mapRoot.find("tilesets");
            if (tilesetsIterator == mapRoot.end())
            {
                return {};
            }
            if (!tilesetsIterator->is_array())
            {
                throw std::runtime_error("Tiled JSONのtilesetsがJSON配列ではありません。");
            }

            std::vector<TilesetDefinition> tilesets;
            tilesets.reserve(tilesetsIterator->size());
            for (const json& mapTileset : *tilesetsIterator)
            {
                tilesets.push_back(ReadTilesetDefinition(
                    mapTileset,
                    mapPath.parent_path()));
            }

            std::sort(
                tilesets.begin(),
                tilesets.end(),
                [](const TilesetDefinition& left, const TilesetDefinition& right)
                {
                    return left.firstGid < right.firstGid;
                });

            for (std::size_t index = 1; index < tilesets.size(); ++index)
            {
                if (tilesets[index - 1].firstGid == tilesets[index].firstGid)
                {
                    throw std::runtime_error("tilesetのfirstgidが重複しています。");
                }
            }
            return tilesets;
        }

        const TileDefinition* FindTileDefinition(
            std::uint32_t globalTileId,
            const std::vector<TilesetDefinition>& tilesets,
            const std::string& context)
        {
            const auto upperIterator = std::upper_bound(
                tilesets.begin(),
                tilesets.end(),
                globalTileId,
                [](std::uint32_t gid, const TilesetDefinition& tileset)
                {
                    return gid < tileset.firstGid;
                });
            if (upperIterator == tilesets.begin())
            {
                throw std::runtime_error(context + "に対応するタイルセットがありません。");
            }

            const TilesetDefinition& tileset = *std::prev(upperIterator);
            const std::uint32_t localId = globalTileId - tileset.firstGid;
            const auto tileIterator = tileset.tilesByLocalId.find(localId);
            return tileIterator == tileset.tilesByLocalId.end()
                ? nullptr
                : &tileIterator->second;
        }

        const std::string* FindEnemyType(
            std::uint32_t globalTileId,
            const std::vector<TilesetDefinition>& tilesets,
            const std::string& context)
        {
            const auto upperIterator = std::upper_bound(
                tilesets.begin(),
                tilesets.end(),
                globalTileId,
                [](std::uint32_t gid, const TilesetDefinition& tileset)
                {
                    return gid < tileset.firstGid;
                });
            if (upperIterator == tilesets.begin())
            {
                throw std::runtime_error(context + "に対応するタイルセットがありません。");
            }

            const TilesetDefinition& tileset = *std::prev(upperIterator);
            const std::uint32_t localId = globalTileId - tileset.firstGid;
            const auto enemyIterator = tileset.enemyTypesByLocalId.find(localId);
            return enemyIterator == tileset.enemyTypesByLocalId.end()
                ? nullptr
                : &enemyIterator->second;
        }

        std::uint32_t ReadGlobalTileId(const json& value, const std::string& context)
        {
            if (!value.is_number_integer())
            {
                throw std::runtime_error(context + "のGIDが整数ではありません。");
            }

            std::uint64_t gid = 0;
            if (value.is_number_unsigned())
            {
                gid = value.get<std::uint64_t>();
            }
            else
            {
                const std::int64_t signedGid = value.get<std::int64_t>();
                if (signedGid < 0)
                {
                    throw std::runtime_error(context + "のGIDが負数です。");
                }
                gid = static_cast<std::uint64_t>(signedGid);
            }

            if (gid > std::numeric_limits<std::uint32_t>::max())
            {
                throw std::runtime_error(context + "のGIDが有効範囲外です。");
            }
            return static_cast<std::uint32_t>(gid);
        }

        // タイルオブジェクトのGIDから敵種別を特定し、配置座標と初期方向を変換する。
        // 敵用タイルでなければnulloptを返し、通常の地形オブジェクト変換へ処理を渡す。
        std::optional<json> ConvertEnemyObject(
            const json& object,
            double accumulatedOffsetX,
            double accumulatedOffsetY,
            double pixelsPerUnit,
            const std::vector<TilesetDefinition>& tilesets)
        {
            const auto gidIterator = object.find("gid");
            if (gidIterator == object.end())
            {
                return std::nullopt;
            }

            const std::string context = ObjectDescription(object);
            const std::uint32_t rawGid = ReadGlobalTileId(*gidIterator, context);
            if (rawGid == kEmptyGlobalTileId)
            {
                return std::nullopt;
            }
            if ((rawGid & kTileTransformFlags) != 0)
            {
                throw std::runtime_error(
                    context + "の敵タイルには反転または回転を使用できません。");
            }

            const std::string* enemyType = FindEnemyType(rawGid, tilesets, context);
            if (enemyType == nullptr)
            {
                return std::nullopt;
            }
            if (!IsVisible(object, context))
            {
                return std::nullopt;
            }

            const double tiledX = ReadRequiredNumber(object, "x", context) + accumulatedOffsetX;
            const double tiledY = ReadRequiredNumber(object, "y", context) + accumulatedOffsetY;
            const double gameX = tiledX / pixelsPerUnit;
            const double gameY = -tiledY / pixelsPerUnit;

            const std::string direction = ReadOptionalStringProperty(
                object,
                kDirectionProperty,
                context).value_or(std::string(kDefaultEnemyDirection));
            ValidateEnemyDirection(direction, context);

            return CreateOutputEnemy(*enemyType, gameX, gameY, direction);
        }

        void ConvertTileArray(
            const json& data,
            std::int64_t dataWidth,
            std::int64_t dataHeight,
            std::int64_t originTileX,
            std::int64_t originTileY,
            double accumulatedOffsetX,
            double accumulatedOffsetY,
            const TileMapSettings& mapSettings,
            double pixelsPerUnit,
            const std::vector<TilesetDefinition>& tilesets,
            const std::string& context,
            std::vector<json>& outputObjects,
            std::size_t& skippedObjectCount)
        {
            if (!data.is_array())
            {
                throw std::runtime_error(
                    context + "のdataがJSON配列ではありません。"
                    "Tiledのタイルレイヤー形式をCSVにしてください。");
            }
            if (dataWidth <= 0 || dataHeight <= 0)
            {
                throw std::runtime_error(context + "のwidthとheightは0より大きい必要があります。");
            }

            const std::uint64_t expectedCount =
                static_cast<std::uint64_t>(dataWidth) * static_cast<std::uint64_t>(dataHeight);
            if (expectedCount != data.size())
            {
                throw std::runtime_error(context + "のdata件数がwidth×heightと一致しません。");
            }

            for (std::size_t index = 0; index < data.size(); ++index)
            {
                const std::int64_t column =
                    static_cast<std::int64_t>(index % static_cast<std::size_t>(dataWidth));
                const std::int64_t row =
                    static_cast<std::int64_t>(index / static_cast<std::size_t>(dataWidth));
                const std::int64_t tileX = originTileX + column;
                const std::int64_t tileY = originTileY + row;
                const std::string cellContext =
                    context + "のcell[x=" + std::to_string(tileX) +
                    ",y=" + std::to_string(tileY) + ']';

                const std::uint32_t storedGid = ReadGlobalTileId(data[index], cellContext);
                if (storedGid == kEmptyGlobalTileId)
                {
                    continue;
                }
                if ((storedGid & kTileTransformFlags) != 0)
                {
                    throw std::runtime_error(
                        cellContext + "に反転または回転したタイルがあります。"
                        "現在は通常向きのタイルだけに対応しています。");
                }

                const std::uint32_t globalTileId = storedGid & kGlobalTileIdMask;
                const TileDefinition* tileDefinition = FindTileDefinition(
                    globalTileId,
                    tilesets,
                    cellContext);
                if (tileDefinition == nullptr)
                {
                    ++skippedObjectCount;
                    continue;
                }

                const double tiledX =
                    static_cast<double>(tileX) * mapSettings.tileWidth + accumulatedOffsetX;
                const double tiledY =
                    static_cast<double>(tileY) * mapSettings.tileHeight + accumulatedOffsetY;
                outputObjects.push_back(CreateOutputObject(
                    tileDefinition->typeId,
                    tiledX / pixelsPerUnit,
                    -tiledY / pixelsPerUnit,
                    kDefaultRotationDegrees,
                    tileDefinition->scaleX,
                    tileDefinition->scaleY));
            }
        }

        void ConvertTileLayer(
            const json& layer,
            double accumulatedOffsetX,
            double accumulatedOffsetY,
            const TileMapSettings& mapSettings,
            double pixelsPerUnit,
            const std::vector<TilesetDefinition>& tilesets,
            const std::string& context,
            std::vector<json>& outputObjects,
            std::size_t& skippedObjectCount)
        {
            if (!mapSettings.isOrthogonal)
            {
                throw std::runtime_error(
                    context + "はorthogonal以外のマップにあります。"
                    "タイルレイヤー変換はorthogonalマップだけに対応しています。");
            }
            if (tilesets.empty())
            {
                throw std::runtime_error(context + "を変換するためのtilesetsがありません。");
            }

            const std::int64_t layerTileX = ReadOptionalInteger(layer, "x", 0, context);
            const std::int64_t layerTileY = ReadOptionalInteger(layer, "y", 0, context);
            const auto chunksIterator = layer.find("chunks");
            if (chunksIterator != layer.end())
            {
                if (!chunksIterator->is_array())
                {
                    throw std::runtime_error(context + "のchunksがJSON配列ではありません。");
                }

                for (const json& chunk : *chunksIterator)
                {
                    if (!chunk.is_object())
                    {
                        throw std::runtime_error(context + "のchunks内にオブジェクト以外があります。");
                    }
                    const std::int64_t chunkX = ReadRequiredInteger(chunk, "x", context + "のchunk");
                    const std::int64_t chunkY = ReadRequiredInteger(chunk, "y", context + "のchunk");
                    const std::int64_t chunkWidth = ReadRequiredInteger(chunk, "width", context + "のchunk");
                    const std::int64_t chunkHeight = ReadRequiredInteger(chunk, "height", context + "のchunk");
                    const auto dataIterator = chunk.find("data");
                    if (dataIterator == chunk.end())
                    {
                        throw std::runtime_error(context + "のchunkにdataがありません。");
                    }
                    ConvertTileArray(
                        *dataIterator,
                        chunkWidth,
                        chunkHeight,
                        layerTileX + chunkX,
                        layerTileY + chunkY,
                        accumulatedOffsetX,
                        accumulatedOffsetY,
                        mapSettings,
                        pixelsPerUnit,
                        tilesets,
                        context,
                        outputObjects,
                        skippedObjectCount);
                }
                return;
            }

            const std::int64_t layerWidth = ReadRequiredInteger(layer, "width", context);
            const std::int64_t layerHeight = ReadRequiredInteger(layer, "height", context);
            const auto dataIterator = layer.find("data");
            if (dataIterator == layer.end())
            {
                throw std::runtime_error(context + "にdataがありません。");
            }
            ConvertTileArray(
                *dataIterator,
                layerWidth,
                layerHeight,
                layerTileX,
                layerTileY,
                accumulatedOffsetX,
                accumulatedOffsetY,
                mapSettings,
                pixelsPerUnit,
                tilesets,
                context,
                outputObjects,
                skippedObjectCount);
        }

        void ConvertLayers(
            const json& layers,
            double parentOffsetX,
            double parentOffsetY,
            const TileMapSettings& mapSettings,
            double pixelsPerUnit,
            const std::vector<TilesetDefinition>& tilesets,
            std::vector<json>& outputObjects,
            std::vector<json>& outputEnemies,
            std::vector<json>& outputFallRespawnZones,
            std::vector<json>& outputBossEncounterZones,
            std::vector<json>& outputBossApproachCameraZones,
            std::size_t& skippedObjectCount)
        {
            if (!layers.is_array())
            {
                throw std::runtime_error("layersがJSON配列ではありません。");
            }

            for (const json& layer : layers)
            {
                if (!layer.is_object())
                {
                    throw std::runtime_error("layers配列にJSONオブジェクト以外の値があります。");
                }

                const std::string layerName = layer.value("name", std::string("<unnamed>"));
                const std::string context = "layer[name=" + layerName + ']';
                if (!IsVisible(layer, context))
                {
                    continue;
                }

                const auto typeIterator = layer.find("type");
                if (typeIterator == layer.end() || !typeIterator->is_string())
                {
                    throw std::runtime_error(context + "のtypeが文字列ではありません。");
                }

                const double layerOffsetX = ReadOptionalNumber(
                    layer,
                    "offsetx",
                    0.0,
                    context);
                const double layerOffsetY = ReadOptionalNumber(
                    layer,
                    "offsety",
                    0.0,
                    context);
                const double accumulatedOffsetX = parentOffsetX + layerOffsetX;
                const double accumulatedOffsetY = parentOffsetY + layerOffsetY;

                const std::string layerType = typeIterator->get<std::string>();
                if (layerType == "group")
                {
                    const auto childLayers = layer.find("layers");
                    if (childLayers == layer.end())
                    {
                        throw std::runtime_error(context + "にlayersがありません。");
                    }
                    ConvertLayers(
                        *childLayers,
                        accumulatedOffsetX,
                        accumulatedOffsetY,
                        mapSettings,
                        pixelsPerUnit,
                        tilesets,
                        outputObjects,
                        outputEnemies,
                        outputFallRespawnZones,
                        outputBossEncounterZones,
                        outputBossApproachCameraZones,
                        skippedObjectCount);
                    continue;
                }

                // タイルレイヤーは、typeIdを持つ各セルを1つのゲームオブジェクトへ展開する。
                if (layerType == "tilelayer")
                {
                    ConvertTileLayer(
                        layer,
                        accumulatedOffsetX,
                        accumulatedOffsetY,
                        mapSettings,
                        pixelsPerUnit,
                        tilesets,
                        context,
                        outputObjects,
                        skippedObjectCount);
                    continue;
                }

                if (layerType != "objectgroup")
                {
                    continue;
                }

                const auto objectsIterator = layer.find("objects");
                if (objectsIterator == layer.end() || !objectsIterator->is_array())
                {
                    throw std::runtime_error(context + "のobjectsがJSON配列ではありません。");
                }

                // 専用名のレイヤーは、通常の地形ではなく用途別の長方形ゾーンとして出力する。
                const bool isFallRespawnZoneLayer = layerName == kFallRespawnZonesLayerName;
                const bool isBossEncounterZoneLayer = layerName == kBossEncounterZoneLayerName;
                const bool isBossApproachCameraZoneLayer = layerName == kBossApproachCameraZoneLayerName;
                if (isFallRespawnZoneLayer ||
                    isBossEncounterZoneLayer ||
                    isBossApproachCameraZoneLayer)
                {
                    std::vector<json>& outputZones = isFallRespawnZoneLayer
                        ? outputFallRespawnZones
                        : isBossEncounterZoneLayer
                            ? outputBossEncounterZones
                            : outputBossApproachCameraZones;

                    for (const json& object : *objectsIterator)
                    {
                        std::optional<json> convertedZone = ConvertRectangleZone(
                            object,
                            accumulatedOffsetX,
                            accumulatedOffsetY,
                            pixelsPerUnit);
                        if (convertedZone.has_value())
                        {
                            outputZones.push_back(std::move(*convertedZone));
                        }
                        else
                        {
                            ++skippedObjectCount;
                        }
                    }
                    continue;
                }

                for (const json& object : *objectsIterator)
                {
                    // 敵タイルを先に判定し、地形objectsとは別のenemies配列へ振り分ける。
                    std::optional<json> convertedEnemy = ConvertEnemyObject(
                        object,
                        accumulatedOffsetX,
                        accumulatedOffsetY,
                        pixelsPerUnit,
                        tilesets);
                    if (convertedEnemy.has_value())
                    {
                        outputEnemies.push_back(std::move(*convertedEnemy));
                        continue;
                    }

                    std::optional<json> convertedObject = ConvertObject(
                        object,
                        accumulatedOffsetX,
                        accumulatedOffsetY,
                        pixelsPerUnit);
                    if (convertedObject.has_value())
                    {
                        outputObjects.push_back(std::move(*convertedObject));
                    }
                    else
                    {
                        ++skippedObjectCount;
                    }
                }
            }
        }

        std::string DetermineStageName(const json& mapRoot, const ConverterOptions& options)
        {
            if (options.stageNameOverride.has_value())
            {
                return *options.stageNameOverride;
            }

            const json* stageNameProperty = FindPropertyValue(mapRoot, kStageNameProperty);
            if (stageNameProperty != nullptr)
            {
                if (!stageNameProperty->is_string())
                {
                    throw std::runtime_error("マップのstageNameプロパティが文字列ではありません。");
                }
                return stageNameProperty->get<std::string>();
            }

            return options.inputPath.stem().u8string();
        }

        bool ReadNextArgument(
            int argumentCount,
            char* arguments[],
            int& currentIndex,
            std::string_view optionName,
            std::string& outputValue,
            std::string& errorMessage)
        {
            if (currentIndex + 1 >= argumentCount)
            {
                errorMessage = std::string(optionName) + "の値がありません。";
                return false;
            }

            ++currentIndex;
            outputValue = arguments[currentIndex];
            return true;
        }
    }

    bool ParseCommandLine(
        int argumentCount,
        char* arguments[],
        ConverterOptions& outputOptions,
        std::string& errorMessage)
    {
        ConverterOptions parsedOptions;

        for (int index = 1; index < argumentCount; ++index)
        {
            const std::string_view argument = arguments[index];
            if (argument == kHelpOption || argument == kShortHelpOption)
            {
                parsedOptions.showHelp = true;
                continue;
            }

            std::string value;
            if (argument == kInputOption)
            {
                if (!ReadNextArgument(
                    argumentCount,
                    arguments,
                    index,
                    kInputOption,
                    value,
                    errorMessage))
                {
                    return false;
                }
                parsedOptions.inputPath = std::filesystem::u8path(value);
            }
            else if (argument == kOutputOption)
            {
                if (!ReadNextArgument(
                    argumentCount,
                    arguments,
                    index,
                    kOutputOption,
                    value,
                    errorMessage))
                {
                    return false;
                }
                parsedOptions.outputPath = std::filesystem::u8path(value);
            }
            else if (argument == kPixelsPerUnitOption)
            {
                if (!ReadNextArgument(
                    argumentCount,
                    arguments,
                    index,
                    kPixelsPerUnitOption,
                    value,
                    errorMessage))
                {
                    return false;
                }

                try
                {
                    std::size_t parsedLength = 0;
                    parsedOptions.pixelsPerUnit = std::stod(value, &parsedLength);
                    if (parsedLength != value.size())
                    {
                        errorMessage = "--pixels-per-unitに数値以外が含まれています。";
                        return false;
                    }
                }
                catch (const std::exception&)
                {
                    errorMessage = "--pixels-per-unitを数値として読み取れません。";
                    return false;
                }
            }
            else if (argument == kStageNameOption)
            {
                if (!ReadNextArgument(
                    argumentCount,
                    arguments,
                    index,
                    kStageNameOption,
                    value,
                    errorMessage))
                {
                    return false;
                }
                parsedOptions.stageNameOverride = value;
            }
            else
            {
                errorMessage = "未対応のオプションです: " + std::string(argument);
                return false;
            }
        }

        if (parsedOptions.showHelp)
        {
            outputOptions = std::move(parsedOptions);
            return true;
        }
        if (parsedOptions.inputPath.empty())
        {
            errorMessage = "--inputを指定してください。";
            return false;
        }
        if (parsedOptions.outputPath.empty())
        {
            errorMessage = "--outputを指定してください。";
            return false;
        }
        if (!std::isfinite(parsedOptions.pixelsPerUnit) || parsedOptions.pixelsPerUnit <= 0.0)
        {
            errorMessage = "--pixels-per-unitは0より大きい有限値を指定してください。";
            return false;
        }
        if (parsedOptions.stageNameOverride.has_value() && parsedOptions.stageNameOverride->empty())
        {
            errorMessage = "--stage-nameに空文字列は指定できません。";
            return false;
        }

        const std::filesystem::path absoluteInput =
            std::filesystem::absolute(parsedOptions.inputPath).lexically_normal();
        const std::filesystem::path absoluteOutput =
            std::filesystem::absolute(parsedOptions.outputPath).lexically_normal();
        if (absoluteInput == absoluteOutput)
        {
            errorMessage = "入力ファイルと出力ファイルに同じパスは指定できません。";
            return false;
        }

        outputOptions = std::move(parsedOptions);
        return true;
    }

    void PrintUsage(std::ostream& output)
    {
        output
            << "StageConverter - Tiled JSON object layer converter\n\n"
            << "Usage:\n"
            << "  StageConverter --input <map.tmj> --output <stage.json> [options]\n\n"
            << "Options:\n"
            << "  --pixels-per-unit <value>  Tiled pixels per one game unit (default: "
            << kDefaultPixelsPerUnit << ")\n"
            << "  --stage-name <name>         Override output stageName\n"
            << "  --help, -h                  Show this help\n";
    }

    ConversionResult TiledStageConverter::Convert(const ConverterOptions& options) const
    {
        const json mapRoot = ReadJsonFile(options.inputPath);
        if (!mapRoot.is_object())
        {
            throw std::runtime_error("Tiled JSONのルートがオブジェクトではありません。");
        }

        const auto mapTypeIterator = mapRoot.find("type");
        if (mapTypeIterator == mapRoot.end() ||
            !mapTypeIterator->is_string() ||
            mapTypeIterator->get<std::string>() != "map")
        {
            throw std::runtime_error("入力JSONはTiledのmap形式ではありません。");
        }

        const auto layersIterator = mapRoot.find("layers");
        if (layersIterator == mapRoot.end())
        {
            throw std::runtime_error("Tiled JSONにlayersがありません。");
        }

        const std::int64_t tileWidth = ReadRequiredInteger(mapRoot, "tilewidth", "map");
        const std::int64_t tileHeight = ReadRequiredInteger(mapRoot, "tileheight", "map");
        if (tileWidth <= 0 || tileHeight <= 0 ||
            tileWidth > std::numeric_limits<int>::max() ||
            tileHeight > std::numeric_limits<int>::max())
        {
            throw std::runtime_error("mapのtilewidthとtileheightが有効範囲外です。");
        }

        const auto orientationIterator = mapRoot.find("orientation");
        if (orientationIterator == mapRoot.end() || !orientationIterator->is_string())
        {
            throw std::runtime_error("mapのorientationが文字列ではありません。");
        }

        TileMapSettings mapSettings;
        mapSettings.tileWidth = static_cast<int>(tileWidth);
        mapSettings.tileHeight = static_cast<int>(tileHeight);
        mapSettings.isOrthogonal =
            orientationIterator->get<std::string>() == "orthogonal";

        const std::vector<TilesetDefinition> tilesets = LoadTilesets(
            mapRoot,
            options.inputPath);

        ConversionResult result;
        result.stageName = DetermineStageName(mapRoot, options);
        if (result.stageName.empty())
        {
            throw std::runtime_error("出力するstageNameが空です。");
        }

        // 出力先を分けることで、ゲーム側が地形生成と敵生成を独立して処理できる。
        std::vector<json> convertedObjects;
        std::vector<json> convertedEnemies;
        std::vector<json> convertedFallRespawnZones;
        std::vector<json> convertedBossEncounterZones;
        std::vector<json> convertedBossApproachCameraZones;
        ConvertLayers(
            *layersIterator,
            0.0,
            0.0,
            mapSettings,
            options.pixelsPerUnit,
            tilesets,
            convertedObjects,
            convertedEnemies,
            convertedFallRespawnZones,
            convertedBossEncounterZones,
            convertedBossApproachCameraZones,
            result.skippedObjectCount);
        result.convertedObjectCount = convertedObjects.size();
        result.convertedEnemyCount = convertedEnemies.size();
        result.convertedFallRespawnZoneCount = convertedFallRespawnZones.size();
        result.convertedBossEncounterZoneCount = convertedBossEncounterZones.size();
        result.convertedBossApproachCameraZoneCount = convertedBossApproachCameraZones.size();

        const json outputRoot{
            { "formatVersion", kOutputFormatVersion },
            { "stageName", result.stageName },
            { "objects", convertedObjects },
            { "enemies", convertedEnemies },
            { "fallRespawnZones", convertedFallRespawnZones },
            { "bossEncounterZones", convertedBossEncounterZones },
            { "bossApproachCameraZones", convertedBossApproachCameraZones }
        };

        WriteJsonFile(options.outputPath, outputRoot);
        return result;
    }
}
