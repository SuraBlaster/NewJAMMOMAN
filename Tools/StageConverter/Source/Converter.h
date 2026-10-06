#pragma once

#include <cstddef>
#include <filesystem>
#include <iosfwd>
#include <optional>
#include <string>

namespace stage_converter
{
    // 1ゲーム単位を何ピクセルとして扱うかの既定値。
    inline constexpr double kDefaultPixelsPerUnit = 1.0;

    // コマンドラインから受け取る変換設定。
    struct ConverterOptions
    {
        std::filesystem::path inputPath;
        std::filesystem::path outputPath;
        double pixelsPerUnit = kDefaultPixelsPerUnit;
        std::optional<std::string> stageNameOverride;
        bool showHelp = false;
    };

    // 変換後に利用者へ報告する件数。
    struct ConversionResult
    {
        std::string stageName;
        std::size_t convertedObjectCount = 0; // 出力した地形配置数
        std::size_t convertedEnemyCount = 0;  // 出力した敵配置数
        std::size_t convertedFallRespawnZoneCount = 0; // 出力した落下復帰エリア数
        std::size_t convertedBossEncounterZoneCount = 0; // 出力したボス戦開始エリア数
        std::size_t convertedBossApproachCameraZoneCount = 0; // 出力したボス前通路カメラエリア数
        std::size_t convertedCameraBoundsCount = 0; // 出力したカメラ表示範囲数
        std::size_t skippedObjectCount = 0;   // 編集用マーカーなど、変換対象外だった配置数
    };

    // コマンドラインを解析する。失敗時はerrorMessageへ理由を格納する。
    bool ParseCommandLine(
        int argumentCount,
        char* arguments[],
        ConverterOptions& outputOptions,
        std::string& errorMessage);

    void PrintUsage(std::ostream& output);

    class TiledStageConverter
    {
    public:
        // TiledのJSONマップをゲーム用stage.jsonへ変換する。
        // 変換途中で問題が発生した場合はstd::runtime_errorを送出する。
        ConversionResult Convert(const ConverterOptions& options) const;
    };
}
