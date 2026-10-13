#include "StageLoader.h"
#include <fstream>
#include <iostream>
#include <json.hpp>
#include <cstddef>
#include <cmath>
#include <utility>
#include <array>
#include <string_view>
#include "TileTypeManager.h"

using json = nlohmann::json;


// このCPP内だけで使用する、ステージJSON解析用の補助関数をまとめる
namespace
{
    // objects配列の1件を検証し、StageObjectDataへ変換する。
    // 成功時だけoutputObjectを更新するため、失敗時に途中までの値は残らない。
    bool ParseStageObject(const json& item, std::size_t objectIndex, const TileTypeManager& tileTypeManager, StageObjectData& outputObject)
    {
        // 省略可能な回転とスケールには、StageObjectDataの初期値を使用する
        StageObjectData loadedObjectData;

        // objects配列の各要素は、配置情報を持つJSONオブジェクトである必要がある
        if (!item.is_object())
        {
            std::cerr << "エラー: objects[" << objectIndex << "].typeIdが存在しません。" << std::endl;
            return false;
        }

        // タイル定義との対応付けに必要なtypeIdが存在するか確認する
        if (!item.count("typeId"))
        {
            std::cerr << "エラー: typeId が存在しません。" << std::endl;
            return false;
        }

        // 配置座標を表すpositionが存在するか確認する
        if (!item.count("position"))
        {
            std::cerr << "エラー: position が存在しません。" << std::endl;
            return false;
        }

        // typeIdは小数や文字列を許可せず、JSON整数に限定する
        if (!item.at("typeId").is_number_integer())
        {
            std::cerr << "エラー: typeIdが正しい型ではありません。" << std::endl;
            return false;
        }

        // 型を確認した後で、検証に使用するtypeIdを整数として取得する
        int typeId = item["typeId"].get<int>();

        // 0は未設定値として扱うため、typeIdは1以上に限定する
        if (typeId <= 0)
        {
            std::cerr << "エラー: typeIdが正しい値ではありません。" << std::endl;
            return false;
        }

        // TileTypes.jsonに登録されていないtypeIdは配置できない
        if (tileTypeManager.GetTileInfo(typeId) == nullptr)
        {
            std::cerr << "エラー: ステージJSONが存在しないタイルです。" << std::endl;
            return false;
        }

        // 検証済みのIDを一時データへ保存する
        loadedObjectData.typeId = typeId;

        // 2.5Dの配置座標として、positionはxとyを持つオブジェクトに限定する
        const json& position = item.at("position");

        if (!position.is_object())
        {
            std::cerr << "エラー: positionはオブジェクトではありません。" << std::endl;
            return false;
        }

        // positionではxとyの両方を必須とする。Zはゲーム側で固定するため読み込まない
        if (!position.count("x"))
        {
            std::cerr << "エラー: position.x が存在しません。" << std::endl;
            return false;
        }

        if (!position.count("y"))
        {
            std::cerr << "エラー: position.y が存在しません。" << std::endl;
            return false;
        }

        // 座標は整数表記と小数表記のどちらも許可する
        if (!position.at("x").is_number())
        {
            std::cerr << "エラー: position.x が正しい型ではありません。" << std::endl;
            return false;
        }

        // 座標は整数表記と小数表記のどちらも許可する
        if (!position.at("y").is_number())
        {
            std::cerr << "エラー: position.y が正しい型ではありません。" << std::endl;
            return false;
        }

        // 座標計算に使用できないNaNや無限大を拒否する
        if (!std::isfinite(position.at("x").get<float>()))
        {
            std::cerr << "エラー: position.x が有限値ではありません。" << std::endl;
            return false;
        }

        if (!std::isfinite(position.at("y").get<float>()))
        {
            std::cerr << "エラー: position.y が有限値ではありません。" << std::endl;
            return false;
        }

        // 検証済みの座標を、ゲーム側で扱う1件分のデータへ格納する
        loadedObjectData.positionX = position.at("x").get<float>();

        loadedObjectData.positionY = position.at("y").get<float>();

        // rotationDegreesは省略可能。省略時はStageObjectDataの初期値0度を使用する
        if (item.count("rotationDegrees"))
        {
            // 回転角度は整数表記と小数表記のどちらも許可する
            if (!item.at("rotationDegrees").is_number())
            {
                std::cerr << "エラー: rotationDegrees が正しい型ではありません。" << std::endl;
                return false;
            }

            float rotationDegrees = item.at("rotationDegrees").get<float>();

            // 回転計算に使用できないNaNや無限大を拒否する
            if (!std::isfinite(rotationDegrees))
            {
                std::cerr << "エラー: rotationDegrees が有限値ではありません。" << std::endl;
                return false;
            }

            loadedObjectData.rotationDegrees = rotationDegrees;
        }

        // scaleは省略可能。省略時はStageObjectDataの初期値1.0を使用する
        if (item.count("scale"))
        {
            const json& scale = item.at("scale");

            if (!scale.is_object())
            {
                std::cerr << "エラー: scaleはオブジェクトではありません。" << std::endl;
                return false;
            }

            // scaleを指定する場合はxとyの両方を必須とする
            if (!scale.count("x"))
            {
                std::cerr << "エラー: scale.x が存在しません。" << std::endl;
                return false;
            }

            if (!scale.count("y"))
            {
                std::cerr << "エラー: scale.y が存在しません。" << std::endl;
                return false;
            }

            // 拡大率は整数表記と小数表記のどちらも許可する
            if (!scale.at("x").is_number())
            {
                std::cerr << "エラー: scale.x が正しい型ではありません。" << std::endl;
                return false;
            }

            if (!scale.at("y").is_number())
            {
                std::cerr << "エラー: scale.y が正しい型ではありません。" << std::endl;
                return false;
            }

            // 拡大率として使用できないNaNや無限大を拒否する
            if (!std::isfinite(scale.at("x").get<float>()))
            {
                std::cerr << "エラー: scale.x が有限値ではありません。" << std::endl;
                return false;
            }

            if (!std::isfinite(scale.at("y").get<float>()))
            {
                std::cerr << "エラー: scale.y が有限値ではありません。" << std::endl;
                return false;
            }

            // 反転や大きさ0のモデルを意図せず作らないよう、正の値だけを許可する
            if (scale.at("x").get<float>() <= 0.0f)
            {
                std::cerr << "エラー: scale.x が0以下です。" << std::endl;
                return false;
            }

            if (scale.at("y").get<float>() <= 0.0f)
            {
                std::cerr << "エラー: scale.y が0以下です。" << std::endl;
                return false;
            }

            loadedObjectData.scaleX = scale.at("x").get<float>();
            loadedObjectData.scaleY = scale.at("y").get<float>();
        }

        // すべて成功した場合だけ出力先へ代入し、途中までのデータを残さない
        outputObject = loadedObjectData;

        return true;

    }

    // JSON上の敵名をEnemyTypeへ変換する。未対応名では出力を変更せずfalseを返す。
    bool TryParseEnemyType(std::string_view enemyTypeName,EnemyType& outputEnemyType)
    {
        constexpr std::array<std::pair<std::string_view, EnemyType>,9> enemyTypeMappings =
        { 
            {
                { "Wave",    EnemyType::Wave },
                { "Scatter", EnemyType::Scatter },
                { "Mage",    EnemyType::Mage },
                { "Fan",     EnemyType::Fan },
                { "Hover",   EnemyType::Hover },
                { "Fly",     EnemyType::Fly },
                { "Egg",     EnemyType::Egg },
                { "Boss",    EnemyType::Boss },
                { "Vacuum",  EnemyType::Vacuum }
            } 
        };
          
        for (auto maps : enemyTypeMappings)
        {
            if (maps.first == enemyTypeName)
            {
                outputEnemyType = maps.second;
                return true;
            }
        }

        return false;
    }

    // JSON上の方向名を配置用の列挙型へ変換し、表記揺れや不正値を拒否する。
    bool TryParseEnemySpawnDirection(std::string_view directionName, EnemySpawnDirection& outputDirection)
    {
        if (directionName == "Left")
        {
            outputDirection = EnemySpawnDirection::Left;
            return true;
        }

        if (directionName == "Right")
        {
            outputDirection = EnemySpawnDirection::Right;
            return true;
        }

        return false;
    }

    // enemies配列の1件を検証し、ゲーム側で生成に使えるEnemySpawnDataへ変換する。
    // 途中で失敗してもoutputEnemyを変更しないよう、一時データへ読み込んでから反映する。
    bool ParseEnemySpawn(const json& item, std::size_t enemyIndex, EnemySpawnData& outputEnemy)
    {
        EnemySpawnData loadedEnemyData;

        if (!item.is_object())
        {
            std::cerr << "エラー: enemies[" << enemyIndex << "]がJSONオブジェクトではありません。" << std::endl;
            return false;
        }

        if (item.count("enemyType") == 0)
        {
            std::cerr << "エラー: enemyTypeが存在しません。" << std::endl;
            return false;
        }

        if (!item.at("enemyType").is_string())
        {
            std::cerr << "エラー: enemyTypeが正しい型ではありません。" << std::endl;
            return false;
        }

        std::string enemyTypeName = item.at("enemyType").get<std::string>();

        if (!TryParseEnemyType(enemyTypeName, loadedEnemyData.enemyType))
        {
            std::cerr << "エラー: enemyIndex["<< enemyIndex << "]番目の" << enemyTypeName << "が未定義です。" << std::endl;
            return false;
        }

        if (item.count("position") == 0)
        {
            std::cerr << "エラー: enemies["<< enemyIndex << "].positionが存在しません。" << std::endl;
            return false;
        }

        const json& position = item.at("position");

        if (!position.is_object())
        {
            std::cerr << "positionがJSONオブジェクトではありません。" << std::endl;
            return false;
        }

        if (!position.count("x"))
        {
            std::cerr << "エラー: enemies[" << enemyIndex << "].position.xが存在しません。" << std::endl;
            return false;
        }

        if (!position.count("y"))
        {
            std::cerr << "エラー: enemies[" << enemyIndex << "].position.yが存在しません。" << std::endl;
            return false;
        }

        if (!position.at("x").is_number())
        {
            std::cerr << "エラー: position.xが正しい型ではありません。" << std::endl;
            return false;
        }

        if (!position.at("y").is_number())
        {
            std::cerr << "エラー: position.yが正しい型ではありません。" << std::endl;
            return false;
        }

        float positionX = position.at("x").get<float>();
        float positionY = position.at("y").get<float>();

        if (!std::isfinite(positionX))
        {
            std::cerr << "エラー: position.x が有限値ではありません。" << std::endl;
            return false;
        }

        if (!std::isfinite(positionY))
        {
            std::cerr << "エラー: position.y が有限値ではありません。" << std::endl;
            return false;
        }

        loadedEnemyData.positionX = positionX;
        loadedEnemyData.positionY = positionY;

        // directionは省略可能。省略時はEnemySpawnDataの既定値Rightを使用する。
        if (item.count("direction") > 0)
        {
            if (!item.at("direction").is_string())
            {
                std::cerr << "エラー: enemies[" << enemyIndex << "].directionが文字列ではありません。" << std::endl;
                return false;
            }

            std::string directionName = item.at("direction").get<std::string>();

            if (!TryParseEnemySpawnDirection(directionName, loadedEnemyData.enemySpawnDirection))
            {
                std::cerr << "エラー: enemies[" << enemyIndex << "].directionはLeftまたはRightである必要があります。" << std::endl;
                return false;
            }
        }

        // すべての項目が正常な場合だけ、呼び出し元の出力を更新する。
        outputEnemy = loadedEnemyData;
        return true;    
    }

    // fallRespawnZones配列の1件を検証し、ゲームで範囲判定に使えるデータへ変換する。
    // 全項目の検証に成功した場合だけoutputZoneを更新する。
    bool ParseFallRespawnZone(
        const json& item,
        std::size_t zoneIndex,
        FallRespawnZoneData& outputZone)
    {
        // 読み込み途中の失敗で呼び出し元へ不完全な値を残さないため、一時データを使う。
        FallRespawnZoneData loadedZoneData;

        // 配列の各要素は、4つの境界値を持つJSONオブジェクトである必要がある。
        if (!item.is_object())
        {
            std::cerr << "エラー: fallRespawnZones[" << zoneIndex << "]がJSONオブジェクトではありません。"
                << std::endl;

            return false;
        }

        // 同じ検証処理をまとめて行えるよう、必須となる境界名を一覧化する。
        constexpr std::array requiredFieldNames
        {
            std::string_view("minX"),
            std::string_view("maxX"),
            std::string_view("minY"),
            std::string_view("maxY")
        };

        // 4つの境界について、存在・数値型・有限値を順番に確認する。
        for (const std::string_view fieldName : requiredFieldNames)
        {
            if (item.count(std::string(fieldName)) == 0)
            {
                std::cerr << "エラー: fallRespawnZones[" << zoneIndex << "]." << fieldName << "が存在しません。"
                    << std::endl;

                return false;
            }

            // 存在確認後なので、atで安全に対象の値を参照できる。
            const json& fieldValue = item.at(std::string(fieldName));

            if (!fieldValue.is_number())
            {
                std::cerr << "エラー: fallRespawnZones[" << zoneIndex << "]." << fieldName << "が数値ではありません。"
                    << std::endl;

                return false;
            }

            // 座標計算に利用できないNaNや無限大を読み込み時点で拒否する。
            const float fieldNumber = fieldValue.get<float>();

            if (!std::isfinite(fieldNumber))
            {
                std::cerr << "エラー: fallRespawnZones[" << zoneIndex << "]." << fieldName << "が有限値ではありません。"
                    << std::endl;
                return false;
            }

        }

        // すべての値を検証してから、一時データへ境界を格納する。
        loadedZoneData.minX = item.at("minX").get<float>();
        loadedZoneData.maxX = item.at("maxX").get<float>();
        loadedZoneData.minY = item.at("minY").get<float>();
        loadedZoneData.maxY = item.at("maxY").get<float>();

        // 幅が0の範囲や、左右が逆転した範囲は長方形として扱えない。
        if (loadedZoneData.minX >= loadedZoneData.maxX)
        {
            std::cerr << "エラー: fallRespawnZones[" << zoneIndex << "]はminXよりmaxXが大きい必要があります。"
                << std::endl;

            return false;
        }

        // 高さが0の範囲や、上下が逆転した範囲も入力ミスとして拒否する。
        if (loadedZoneData.minY >= loadedZoneData.maxY)
        {
            std::cerr << "エラー: fallRespawnZones[" << zoneIndex << "]はminYよりmaxYが大きい必要があります。"
                << std::endl;

            return false;
        }

        // すべての検証を通過したデータだけを呼び出し元へ反映する。
        outputZone = loadedZoneData;

        return true;
    }

    bool ParseBossEncounterZone(
        const json& item,
        std::size_t zoneIndex,
        BossEncounterZoneData& outputZone)
    {
        // 読み込み途中の失敗で呼び出し元へ不完全な値を残さないため、一時データを使う。
        BossEncounterZoneData loadedZoneData;

        // 配列の各要素は、4つの境界値を持つJSONオブジェクトである必要がある。
        if (!item.is_object())
        {
            std::cerr << "エラー: bossEncounterZones[" << zoneIndex << "]がJSONオブジェクトではありません。"
                << std::endl;

            return false;
        }

        // 同じ検証処理をまとめて行えるよう、必須となる境界名を一覧化する。
        constexpr std::array requiredFieldNames
        {
            std::string_view("minX"),
            std::string_view("maxX"),
            std::string_view("minY"),
            std::string_view("maxY")
        };

        // 4つの境界について、存在・数値型・有限値を順番に確認する。
        for (const std::string_view fieldName : requiredFieldNames)
        {
            if (item.count(std::string(fieldName)) == 0)
            {
                std::cerr << "エラー: bossEncounterZones[" << zoneIndex << "]." << fieldName << "が存在しません。"
                    << std::endl;

                return false;
            }

            // 存在確認後なので、atで安全に対象の値を参照できる。
            const json& fieldValue = item.at(std::string(fieldName));

            if (!fieldValue.is_number())
            {
                std::cerr << "エラー: bossEncounterZones[" << zoneIndex << "]." << fieldName << "が数値ではありません。"
                    << std::endl;

                return false;
            }

            // 座標計算に利用できないNaNや無限大を読み込み時点で拒否する。
            const float fieldNumber = fieldValue.get<float>();

            if (!std::isfinite(fieldNumber))
            {
                std::cerr << "エラー: bossEncounterZones[" << zoneIndex << "]." << fieldName << "が有限値ではありません。"
                    << std::endl;
                return false;
            }

        }

        // すべての値を検証してから、一時データへ境界を格納する。
        loadedZoneData.minX = item.at("minX").get<float>();
        loadedZoneData.maxX = item.at("maxX").get<float>();
        loadedZoneData.minY = item.at("minY").get<float>();
        loadedZoneData.maxY = item.at("maxY").get<float>();

        // 幅が0の範囲や、左右が逆転した範囲は長方形として扱えない。
        if (loadedZoneData.minX >= loadedZoneData.maxX)
        {
            std::cerr << "エラー: bossEncounterZones[" << zoneIndex << "]はminXよりmaxXが大きい必要があります。"
                << std::endl;

            return false;
        }

        // 高さが0の範囲や、上下が逆転した範囲も入力ミスとして拒否する。
        if (loadedZoneData.minY >= loadedZoneData.maxY)
        {
            std::cerr << "エラー: bossEncounterZones[" << zoneIndex << "]はminYよりmaxYが大きい必要があります。"
                << std::endl;

            return false;
        }

        // すべての検証を通過したデータだけを呼び出し元へ反映する。
        outputZone = loadedZoneData;

        return true;
    }

    bool CameraLimitZone(
        const json& item,
        std::size_t zoneIndex,
        CameraLimitZoneData& outputZone)
    {
        // 読み込み途中の失敗で呼び出し元へ不完全な値を残さないため、一時データを使う。
        CameraLimitZoneData loadedZoneData;

        // 配列の各要素は、4つの境界値を持つJSONオブジェクトである必要がある。
        if (!item.is_object())
        {
            std::cerr << "エラー: cameraBounds[" << zoneIndex << "]がJSONオブジェクトではありません。"
                << std::endl;

            return false;
        }

        // 同じ検証処理をまとめて行えるよう、必須となる境界名を一覧化する。
        constexpr std::array requiredFieldNames
        {
            std::string_view("minX"),
            std::string_view("maxX"),
            std::string_view("minY"),
            std::string_view("maxY")
        };

        // 4つの境界について、存在・数値型・有限値を順番に確認する。
        for (const std::string_view fieldName : requiredFieldNames)
        {
            if (item.count(std::string(fieldName)) == 0)
            {
                std::cerr << "エラー: cameraBounds[" << zoneIndex << "]." << fieldName << "が存在しません。"
                    << std::endl;

                return false;
            }

            // 存在確認後なので、atで安全に対象の値を参照できる。
            const json& fieldValue = item.at(std::string(fieldName));

            if (!fieldValue.is_number())
            {
                std::cerr << "エラー: cameraBounds[" << zoneIndex << "]." << fieldName << "が数値ではありません。"
                    << std::endl;

                return false;
            }

            // 座標計算に利用できないNaNや無限大を読み込み時点で拒否する。
            const float fieldNumber = fieldValue.get<float>();

            if (!std::isfinite(fieldNumber))
            {
                std::cerr << "エラー: cameraBounds[" << zoneIndex << "]." << fieldName << "が有限値ではありません。"
                    << std::endl;
                return false;
            }

        }

        // すべての値を検証してから、一時データへ境界を格納する。
        loadedZoneData.minX = item.at("minX").get<float>();
        loadedZoneData.maxX = item.at("maxX").get<float>();
        loadedZoneData.minY = item.at("minY").get<float>();
        loadedZoneData.maxY = item.at("maxY").get<float>();

        // 幅が0の範囲や、左右が逆転した範囲は長方形として扱えない。
        if (loadedZoneData.minX >= loadedZoneData.maxX)
        {
            std::cerr << "エラー: cameraBounds[" << zoneIndex << "]はminXよりmaxXが大きい必要があります。"
                << std::endl;

            return false;
        }

        // 高さが0の範囲や、上下が逆転した範囲も入力ミスとして拒否する。
        if (loadedZoneData.minY >= loadedZoneData.maxY)
        {
            std::cerr << "エラー: cameraBounds[" << zoneIndex << "]はminYよりmaxYが大きい必要があります。"
                << std::endl;

            return false;
        }

        // すべての検証を通過したデータだけを呼び出し元へ反映する。
        outputZone = loadedZoneData;

        return true;
    }

    bool ParseBossApproachCameraZone(
        const json& item,
        std::size_t zoneIndex,
        BossApproachCameraZoneData& outputZone)
    {
        // 読み込み途中の失敗で呼び出し元へ不完全な値を残さないため、一時データを使う。
        BossApproachCameraZoneData loadedZoneData;

        // 配列の各要素は、4つの境界値を持つJSONオブジェクトである必要がある。
        if (!item.is_object())
        {
            std::cerr << "エラー: bossApproachCameraZones[" << zoneIndex << "]がJSONオブジェクトではありません。"
                << std::endl;

            return false;
        }

        // 同じ検証処理をまとめて行えるよう、必須となる境界名を一覧化する。
        constexpr std::array requiredFieldNames
        {
            std::string_view("minX"),
            std::string_view("maxX"),
            std::string_view("minY"),
            std::string_view("maxY")
        };

        // 4つの境界について、存在・数値型・有限値を順番に確認する。
        for (const std::string_view fieldName : requiredFieldNames)
        {
            if (item.count(std::string(fieldName)) == 0)
            {
                std::cerr << "エラー: bossApproachCameraZones[" << zoneIndex << "]." << fieldName << "が存在しません。"
                    << std::endl;

                return false;
            }

            // 存在確認後なので、atで安全に対象の値を参照できる。
            const json& fieldValue = item.at(std::string(fieldName));

            if (!fieldValue.is_number())
            {
                std::cerr << "エラー: bossApproachCameraZones[" << zoneIndex << "]." << fieldName << "が数値ではありません。"
                    << std::endl;

                return false;
            }

            // 座標計算に利用できないNaNや無限大を読み込み時点で拒否する。
            const float fieldNumber = fieldValue.get<float>();

            if (!std::isfinite(fieldNumber))
            {
                std::cerr << "エラー: bossApproachCameraZones[" << zoneIndex << "]." << fieldName << "が有限値ではありません。"
                    << std::endl;
                return false;
            }

        }

        // すべての値を検証してから、一時データへ境界を格納する。
        loadedZoneData.minX = item.at("minX").get<float>();
        loadedZoneData.maxX = item.at("maxX").get<float>();
        loadedZoneData.minY = item.at("minY").get<float>();
        loadedZoneData.maxY = item.at("maxY").get<float>();

        // 幅が0の範囲や、左右が逆転した範囲は長方形として扱えない。
        if (loadedZoneData.minX >= loadedZoneData.maxX)
        {
            std::cerr << "エラー: bossApproachCameraZones[" << zoneIndex << "]はminXよりmaxXが大きい必要があります。"
                << std::endl;

            return false;
        }

        // 高さが0の範囲や、上下が逆転した範囲も入力ミスとして拒否する。
        if (loadedZoneData.minY >= loadedZoneData.maxY)
        {
            std::cerr << "エラー: bossApproachCameraZones[" << zoneIndex << "]はminYよりmaxYが大きい必要があります。"
                << std::endl;

            return false;
        }

        // すべての検証を通過したデータだけを呼び出し元へ反映する。
        outputZone = loadedZoneData;

        return true;
    }

}


// ステージJSON全体を読み込み、全項目の検証に成功した場合だけ出力を更新する
bool StageLoader::Load(const std::string& jsonPath, const TileTypeManager& tileTypeManager, StageData& outputStageData)
{
    // ファイルを開けない場合はJSON解析へ進まない
    std::ifstream file(jsonPath);

    if (!file.is_open()) 
    {
        std::cerr << "don't open json" << jsonPath << std::endl;
        return false;
    }

    // JSONファイル全体を保持するルートオブジェクト
    json root;

    try
    {
        // 構文エラーが発生した場合は下のparse_errorで処理する
        file >> root;

        // ファイル全体は、formatVersion、stageName、objectsを持つJSONオブジェクトである必要がある
        if (!root.is_object()) 
        {
            std::cerr << "エラー: rootがJSONオブジェクトではありません。" << std::endl;
            return false;
        }

        // ステージ形式の判別に必要なformatVersionが存在するか確認する
        if (!root.count("formatVersion")) 
        {
            std::cerr << "エラー: formatVersion が存在しません。" << std::endl;
            return false;
        }

        // ステージを識別するstageNameが存在するか確認する
        if (!root.count("stageName")) 
        {
            std::cerr << "エラー: stageName が存在しません。" << std::endl;
            return false;
        }

        // 配置一覧を表すobjectsが存在するか確認する
        if (!root.count("objects")) 
        {
            std::cerr << "エラー: objects が存在しません。" << std::endl;
            return false;
        }

        // formatVersionはJSON整数である必要がある
        if (!root.at("formatVersion").is_number_integer())
        {
            std::cerr << "エラー: formatVersionが正しい型ではありません。" << std::endl;
            return false;
        }

        // stageNameはJSON文字列である必要がある
        if (!root.at("stageName").is_string())
        {
            std::cerr << "エラー: stageNameが正しい型ではありません。" << std::endl;
            return false;
        }

        // objectsは空の場合も含め、JSON配列である必要がある
        if (!root.at("objects").is_array())
        {
            std::cerr << "エラー: objectsが正しい型ではありません。" << std::endl;
            return false;
        }

        // enemiesは旧形式との互換性のため省略可能だが、存在する場合は必ず配列とする。
        if (root.count("enemies") > 0 && !root.at("enemies").is_array())
        {
            std::cerr << "エラー: enemiesがJSON配列ではありません。" << std::endl;
            return false;
        }

        // 旧ステージとの互換性のため省略可能だが、存在する場合はJSON配列に限定する。
        if (root.count("fallRespawnZones") > 0 && !root.at("fallRespawnZones").is_array())
        {
            std::cerr << "エラー: fallRespawnZonesがJSON配列ではありません。" << std::endl;
            return false;
        }

        if (root.count("bossEncounterZones") > 0 && !root.at("bossEncounterZones").is_array())
        {
            std::cerr << "エラー: bossEncounterZonesがJSON配列ではありません。" << std::endl;
            return false;
        }

        if (root.count("bossApproachCameraZones") > 0 && !root.at("bossApproachCameraZones").is_array())
        {
            std::cerr << "エラー: bossApproachCameraZonesがJSON配列ではありません。" << std::endl;
            return false;
        }

        // コピーせずに各配置を読み取るため、objects配列への定数参照を取得する
        if (root.count("cameraBounds") > 0 && !root.at("cameraBounds").is_array())
        {
            std::cerr << "cameraBounds must be a JSON array." << std::endl;
            return false;
        }

        const json& objects = root.at("objects");

        // 現在このローダーが対応しているステージ形式はバージョン1のみ
        if (root.at("formatVersion").get<int>() != 1)
        {
            std::cerr << "エラー: formatVersionが正しい値ではありません。" << std::endl;
            return false;
        }

        // 名前のないステージを作らないよう、空文字列を拒否する
        if (root.at("stageName") == "")
        {
            std::cerr << "エラー: stageNameが正しい値ではありません。" << std::endl;
            return false;
        }

        // 読み込み途中で失敗しても出力先を壊さないよう、一時データへ格納する
        StageData loadedStageData;

        // 検証済みのルート情報を一時ステージデータへ格納する
        loadedStageData.formatVersion = root.at("formatVersion").get<int>();

        loadedStageData.stageName = root.at("stageName").get<std::string>();

        // 配置件数分の容量を先に確保し、push_back時の再確保を減らす
        loadedStageData.objects.reserve(objects.size());

        // 配列番号を保持して、エラー時にobjects[番号]を表示できるようにする
        for (std::size_t objectIndex = 0; objectIndex < objects.size(); objectIndex++)
        {
            // 配置1件分も一時データへ読み込み、成功するまで一覧へ追加しない
            StageObjectData loadedObjectData;

            // 1件でも不正な配置があれば、ステージファイル全体を読み込み失敗にする
            if (!ParseStageObject(objects[objectIndex], objectIndex, tileTypeManager, loadedObjectData))
            {
                return false;
            }

            // 検証に成功した配置だけを一時ステージへ追加する
            loadedStageData.objects.push_back(loadedObjectData);
        }

        // 敵配置がある場合だけ読み込み、空または未定義なら敵なしステージとして扱う。
        if (root.count("enemies") > 0)
        {
            const json& enemies = root.at("enemies");

            // 配置数が事前に分かるため、vectorの再確保を避ける。
            loadedStageData.enemies.reserve(enemies.size());

            for (std::size_t enemyIndex = 0; enemyIndex < enemies.size(); enemyIndex++)
            {
                EnemySpawnData loadedEnemyData;

                // 1体でも不正なら、中途半端なステージデータを採用せず読み込み全体を失敗させる。
                if (!ParseEnemySpawn(enemies[enemyIndex], enemyIndex,loadedEnemyData))
                {
                    return false;
                }

                loadedStageData.enemies.push_back(loadedEnemyData);
            }
        }

        // 落下復帰エリアがある場合、各長方形を検証して読み込む。
        if (root.count("fallRespawnZones") > 0)
        {
            const json& fallRespawnZones = root.at("fallRespawnZones");

            // 要素数が分かっているため、追加時にvectorが何度も再確保されるのを防ぐ。
            loadedStageData.fallRespawnZones.reserve(fallRespawnZones.size());

            for (std::size_t zoneIndex = 0;
                zoneIndex < fallRespawnZones.size();
                zoneIndex++)
            {
                FallRespawnZoneData loadedZoneData;

                // 1件でも不正なら、中途半端なステージデータを採用しない。
                if (!ParseFallRespawnZone(
                    fallRespawnZones[zoneIndex],
                    zoneIndex,
                    loadedZoneData))
                {
                    return false;
                }

                loadedStageData.fallRespawnZones.push_back(loadedZoneData);
            }
        }

        if (root.count("bossEncounterZones") > 0)
        {
            const json& bossEncounterZones = root.at("bossEncounterZones");

            loadedStageData.bossEncounterZones.reserve(
                bossEncounterZones.size());

            for (std::size_t zoneIndex = 0;
                zoneIndex < bossEncounterZones.size();
                zoneIndex++)
            {
                BossEncounterZoneData loadedZoneData;

                if (!ParseBossEncounterZone(
                    bossEncounterZones[zoneIndex],
                    zoneIndex,
                    loadedZoneData))
                {
                    return false;
                }

                loadedStageData.bossEncounterZones.push_back(
                    loadedZoneData);
            }
        }

        if (root.count("bossApproachCameraZones") > 0)
        {
            const json& bossApproachCameraZones =
                root.at("bossApproachCameraZones");

            loadedStageData.bossApproachCameraZones.reserve(
                bossApproachCameraZones.size());

            for (std::size_t zoneIndex = 0;
                zoneIndex < bossApproachCameraZones.size();
                zoneIndex++)
            {
                BossApproachCameraZoneData loadedZoneData;

                if (!ParseBossApproachCameraZone(
                    bossApproachCameraZones[zoneIndex],
                    zoneIndex,
                    loadedZoneData))
                {
                    return false;
                }

                loadedStageData.bossApproachCameraZones.push_back(
                    loadedZoneData);
            }
        }

        if (root.count("cameraBounds") > 0)
        {
            const json& cameraLimitZones =
                root.at("cameraBounds");

            loadedStageData.cameraLimitZones.reserve(
                cameraLimitZones.size());

            for (std::size_t zoneIndex = 0;
                zoneIndex < cameraLimitZones.size();
                zoneIndex++)
            {
                CameraLimitZoneData loadedZoneData;

                if (!CameraLimitZone(
                    cameraLimitZones[zoneIndex],
                    zoneIndex,
                    loadedZoneData))
                {
                    return false;
                }

                loadedStageData.cameraLimitZones.push_back(
                    loadedZoneData);
            }
        }

        // 全配置の読み込み成功後にだけ、呼び出し元のデータを新しい内容へ置き換える
        outputStageData = std::move(loadedStageData);
    }

    // カンマや括弧など、JSON自体の構文が壊れている場合
    catch (const json::parse_error& e)
    {
        std::cerr << "JSON" << e.what() << std::endl;
        return false;
    }

    // JSON値の取得や変換中に発生した、その他のnlohmann::json例外
    catch (const json::exception& e)
    {
        std::cerr << "JSON処理エラー: "
            << e.what() << std::endl;
        return false;
    }

    // ルートから全配置まで問題なく読み込めた
    return true;
}
