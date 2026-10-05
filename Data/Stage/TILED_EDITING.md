# 研究所ステージの編集

Tiledで `LaboratoryCourse.tmj` を開いてください。タイルセットと画像は隣の `Tiled` フォルダーにあります。

1. 地形は `01_Switchback` ～ `07_BossArena` のタイルレイヤーで編集します。1マスはゲーム内1mです。
2. 敵は `Enemies` のタイルオブジェクトを移動・複製します。向きはカスタムプロパティ `direction` の `Left` / `Right` で設定します。
3. `FallRespawnZones` は落下検出、`BossApproachCameraZone` はボス前通路のカメラ、`BossEncounterZone` はボス戦開始範囲です。長方形で編集します。
4. JSONマップ（`.tmj`）、タイルデータはCSV/配列形式で保存します。
5. `ConvertLaboratory.cmd` をダブルクリックしてゲーム用 `ConvertedStage.stage.json` を更新し、ゲームを起動し直します。変換前のJSONは `ConvertedStage.before-tiled.stage.json` に保存されます。

StageConverterのGUIから変換する場合も、Pixels Per Unitは **32** にしてください。出力先はこのフォルダーの `ConvertedStage.stage.json` です。レイヤーを非表示にすると、そのレイヤーは変換から除外されます。

研究所の背景壁・柱・設備はゲーム内の **F6 背景エディタ** で編集し、Saveで `Background.stage.json` に保存します。Tiledの地形には当たり判定があり、背景壁にはありません。

背景エディタの `Sized object` から現行サイズの壁・柱・設備を選び、`Add sized object at camera` で配置できます。`Patterns / ready-made rooms` には研究設備・電力設備・資材置場・警備通路・ボス用設備・観察窓の6パターンを登録しています。`Base XYZ` のZ初期値は壁の奥行きと同じ14です。

自作パターンはCtrl+クリックまたは範囲選択で複数選択し、`Register selection as pattern` に名前を入力して `Register selected objects` を押します。位置関係・回転・大きさを維持したパターンが `Background.patterns.json` に即時保存され、再起動後も利用できます。同名の登録は拒否されます。パターン配置はまとめてUndo/Redoできます。登録したパターン自体は背景配置のUndo対象にはなりません。

ボス前通路とボス部屋は、旧 `BioLaboratoryTrainingStage` の形状を X +92m / Y +11m だけ移動して復元しました。通路の上下の壁、部屋の天井・左右の壁、左側の3mの入口、ボスの向きと部屋内の相対位置を保持しています。

開発用の `node Tools/build_switchback_course.cjs` はコースを生成し直し、TMJも上書きします。Tiledで編集した後は通常の変換コマンドだけを使ってください。
