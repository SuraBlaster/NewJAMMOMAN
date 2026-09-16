#pragma once

#include <string>
#include <unordered_map>
#include <DirectXMath.h>
#include <json.hpp>

using json = nlohmann::json;

// TileTypes.jsonの定義1件を、ゲーム内で扱いやすい形にしたデータ
struct TileInfo
{
    int typeId = 0;                                      // タイル種類を識別する一意のID
    std::string name = "NONE";                           // デバッグ表示などで使う名前
    std::string category = "";                          // Ground、Wallなどの分類
    std::string modelPath = "";                         // Data/Model以下にあるモデルの相対パス
    bool collision = true;                              // 当たり判定を作成するか
    DirectX::XMFLOAT3 defaultScale{ 1.0f,1.0f,1.0f };   // 配置データに指定がない場合の倍率
};

// タイル定義ファイルの読み込み、検証、typeIdによる検索を管理するクラス
class TileTypeManager
{
private:
    // typeIdをキーにして、対応するタイル定義を保持する
    std::unordered_map<int, TileInfo> idToInfoMap;

    // definitions配列の1件について、必須項目・型・値を検証する
    bool DefinitionsVerification(const json& item);

    // モデルパスがプロジェクト内の安全な実在ファイルを指すか検証する
    bool ValidateModelPath(const std::string& modelPath);
public:
    // JSONを検証して読み込む。成功時はtrue、失敗時は既存の定義を変更せずfalseを返す
    bool LoadDefinitions(const std::string& jsonPath);

    // typeIdに対応する定義を返す。未登録の場合はnullptrを返す
    const TileInfo* GetTileInfo(int id) const {
        auto it = idToInfoMap.find(id);
        if (it != idToInfoMap.end()) {
            return &(it->second);
        }
        return nullptr; // 未定義のIDの場合
    }
};

