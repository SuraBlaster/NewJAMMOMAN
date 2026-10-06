#pragma once
#include <string>
#include "StageData.h"

// Loadの引数では参照として扱うだけなので、ヘッダーの依存を減らすため前方宣言する
class TileTypeManager;

// ステージJSONを検証し、描画処理に依存しないStageDataへ変換するクラス
class StageLoader
{
public:
    // ステージJSONを読み込む。
    // 成功時はoutputStageDataを新しい内容へ更新してtrue、失敗時は変更せずfalseを返す。
    static bool Load(
        const std::string& jsonPath,
        const TileTypeManager& tileTypeManager,
        StageData& outputStageData);
};
