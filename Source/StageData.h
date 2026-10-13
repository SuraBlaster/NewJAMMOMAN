#pragma once
#include <vector>
#include <string>



// ステージJSONのenemyType文字列を、ゲーム内で安全に扱うための敵種別。
// Unknownは未設定または不正な状態を表し、実際の敵生成には使用しない。
enum class EnemyType
{
    Unknown,
    Wave,
    Scatter,
    Mage,
    Fan,
    Hover,
    Fly,
    Egg,
    Boss,
    Vacuum
};

// Tiled上で指定した敵の初期方向。現在はEnemyWaveの移動方向に使用する。
enum class EnemySpawnDirection
{
    Left,
    Right
};

// enemies配列の1件に対応する、敵1体分の検証済み配置データ。
// Z座標は2.5Dの共通レーンとして、敵生成時にプレイヤー位置から補う。
struct EnemySpawnData
{
    EnemyType enemyType = EnemyType::Unknown;                         // 生成する敵の種類
    float positionX = 0.0f;                                          // ゲーム座標系でのX位置
    float positionY = 0.0f;                                          // ゲーム座標系でのY位置
    EnemySpawnDirection enemySpawnDirection = EnemySpawnDirection::Right; // 省略時は右向き
};

// fallRespawnZones配列の1件を表す、検証済みの長方形範囲。
// 2.5Dステージ上の左・右・下・上の境界をゲーム座標で保持する。
struct FallRespawnZoneData
{
    float minX = 0.0f; // 落下復帰エリアの左端
    float maxX = 0.0f; // 落下復帰エリアの右端
    float minY = 0.0f; // 落下復帰エリアの下端
    float maxY = 0.0f; // 落下復帰エリアの上端
};

struct BossEncounterZoneData
{
    float minX = 0.0f; // 左端
    float maxX = 0.0f; // 右端
    float minY = 0.0f; // 下端
    float maxY = 0.0f; // 上端
};

struct BossApproachCameraZoneData
{
    float minX = 0.0f; // 左端
    float maxX = 0.0f; // 右端
    float minY = 0.0f; // 下端
    float maxY = 0.0f; // 上端
};

struct CameraLimitZoneData
{
    float minX = 0.0f; // 左端
    float maxX = 0.0f; // 右端
    float minY = 0.0f; // 下端
    float maxY = 0.0f; // 上端
};

// ステージJSONのobjects配列に含まれる、配置1件分の読み込みデータ
// DirectXやModelには依存せず、ファイルから得た値だけを保持する
struct StageObjectData
{
    int typeId = 0;                    // TileTypes.jsonの定義と対応付けるID
    float positionX = 0.0f;            // ゲーム座標系でのX位置
    float positionY = 0.0f;            // ゲーム座標系でのY位置。Zは実行時にゲーム側で固定する
    float rotationDegrees = 0.0f;      // 画面平面上の回転角度（度数法）
    float scaleX = 1.0f;               // タイル定義のdefaultScaleへ掛けるX倍率
    float scaleY = 1.0f;               // タイル定義のdefaultScaleへ掛けるY倍率
};

// ステージJSON全体を表す読み込みデータ
struct StageData
{
    int formatVersion = 0;                     // 読み込んだステージ形式のバージョン
    std::string stageName;                     // ステージの識別・表示に使用する名前
    std::vector<StageObjectData> objects;       // ステージに配置されたオブジェクト一覧
    std::vector<EnemySpawnData> enemies;         // Tiledから読み込んだ敵の初期配置一覧
    std::vector<FallRespawnZoneData> fallRespawnZones; // Tiledで指定した落下復帰エリア一覧
    std::vector<BossEncounterZoneData> bossEncounterZones;
    std::vector<BossApproachCameraZoneData> bossApproachCameraZones;
    std::vector<CameraLimitZoneData> cameraLimitZones;
};
    
