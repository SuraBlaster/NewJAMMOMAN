# 新しい研究所ステージの土台

Tiledで `LaboratoryNewStage.tmj` を開いてください。

- 地形は `Terrain` に配置します。エリア別の分割はなく、上下左右に拡張できる無限マップです。レイヤーはすべてロック解除済みです。
- 開始地点用にゲーム座標 X=-4〜15、Y=-1 の床20ブロックだけを配置しています。床も自由に変更できます。
- 元のマップと同じ `Tiled/CubeBlockTileset.tsj`、`EnemyPlacements.tsj`、`GateTileset.tsj` を参照しています。1マス32px＝ゲーム内1mです。
- 敵は `Enemies` オブジェクトレイヤーに敵タイルを配置してください。地形・ゲートはタイルレイヤー、敵はオブジェクトレイヤーを使います。
- ゲートは `Gates` に配置します。
- 旧エリアの敵、ゲート、落下復帰範囲、カメラ制限、ボス戦範囲は空にしています。各範囲のレイヤーは必要になったら長方形を追加できます。

保存後、`ConvertLaboratoryNewStage.cmd` を実行すると変換し、現在のゲーム用JSONを `ConvertedStage.before-new-stage.stage.json` にバックアップしてから新しいステージへ切り替えます。このバックアップはコマンド実行のたびに更新されます。ゲームを再起動して確認してください。

StageConverterのGUIで変換する場合は Pixels Per Unit=32、出力先を `LaboratoryNewStage.stage.json` にします。この出力先だけではゲーム用ステージは切り替わりません。

元の `LaboratoryCourse.tmj` は保持しています。生成スクリプト `Tools/create_laboratory_template.cjs` は新規作成用で、既存の新ステージを上書きしません。`build_switchback_course.cjs` による旧エリア分割もこの新規マップには適用されません。
