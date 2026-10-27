# Walker

HP1、速度1m/sの歩行ロボット。接触ダメージ1。射撃・追跡・突進は行わず、壁と足場の端で折り返します。

カメラの画面内に入ると歩き始めます。画面端に一部だけ見えている場合も対象です。一度起動した後は画面外へ出ても移動を続けます。起動前はアニメーションを更新せず、モデルのGPUリソースは既存のModelManagerで共有します。

Tiledでは `Tiled/WalkerPlacements.tsj` のタイルを `Enemies` レイヤーに配置してください。`direction` は `Left` または `Right`。既定は左向きです。マップを開いている場合は再読み込みしてください。

試験用にLaboratoryCombinedCourseの序盤、世界座標 (10, -0.5) に1体追加しています。地形・カメラ・既存の敵はそのままです。変更前のTMJとstage.jsonはbefore-walker付きの名前で保存しています。

モデル: [Robot character / styloo](https://styloo.itch.io/robot-character)、CC0。使用アニメーションは `iddle` と `walking`。モデル一式は `Data/Model/Enemy/Walker/` にあります。
