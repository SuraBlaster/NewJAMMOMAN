# 研究所ステージの編集

現在のゲーム用マップは **`LaboratoryCombinedCourse.tmj`** です。Tiledでこのファイルを開いてください。タイルセットと画像は隣の `Tiled` フォルダーにあります。

1. 地形は `Terrain` タイルレイヤーで編集します。1マスはゲーム内1mです。
2. 敵は `Enemies` のタイルオブジェクトを移動・複製します。向きはカスタムプロパティ `direction` の `Left` / `Right` で設定します。
3. `FallRespawnZones` は落下検出、`BossApproachCameraZone` はボス前通路のカメラ、`BossEncounterZone` はボス戦開始範囲です。長方形で編集します。
4. JSONマップ（`.tmj`）、タイルデータはCSV/配列形式で保存します。
5. `ConvertLaboratoryCombined.cmd` をダブルクリックしてゲーム用 `LaboratoryCombinedCourse.stage.json` を更新し、ゲームを起動し直します。変換前のJSONは `LaboratoryCombinedCourse.before-tiled.stage.json` に保存されます。

StageConverterのGUIから変換する場合も、Pixels Per Unitは **32** にしてください。出力先はこのフォルダーの `LaboratoryCombinedCourse.stage.json` です。レイヤーを非表示にすると、そのレイヤーは変換から除外されます。

研究所の背景壁・柱・設備はゲーム内の **F6 背景エディタ** で編集し、Saveで `Background.stage.json` に保存します。Tiledの地形には当たり判定があり、背景壁にはありません。

背景エディタの `Sized object` から現行サイズの壁・柱・設備を選び、`Add sized object at camera` で配置できます。`Patterns / ready-made rooms` には研究設備・電力設備・資材置場・警備通路・ボス用設備・観察窓の6パターンを登録しています。`Base XYZ` のZ初期値は壁の奥行きと同じ14です。

自作パターンはCtrl+クリックまたは範囲選択で複数選択し、`Register selection as pattern` に名前を入力して `Register selected objects` を押します。位置関係・回転・大きさを維持したパターンが `Background.patterns.json` に即時保存され、再起動後も利用できます。同名の登録は拒否されます。パターン配置はまとめてUndo/Redoできます。登録したパターン自体は背景配置のUndo対象にはなりません。

ボス前通路とボス部屋は、旧 `BioLaboratoryTrainingStage` の形状を X +92m / Y +11m だけ移動して復元しました。通路の上下の壁、部屋の天井・左右の壁、左側の3mの入口、ボスの向きと部屋内の相対位置を保持しています。

開発用の `node Tools/build_switchback_course.cjs` はコースを生成し直し、TMJも上書きします。Tiledで編集した後は通常の変換コマンドだけを使ってください。

## 敵の配置

`Tiled/EnemyPlacements.tsj` のタイルをオブジェクトレイヤー `Enemies` に配置してください。`Vacuum` はEnemyVacuumに対応します。1マスは32×32pxで、Mage・Bossは32×64px（1×2）、その他は32×32px（1×1）です。配置座標は下端中央を基準にします。Vacuumの初期方向は文字列プロパティ `direction` に `Left` または `Right` を指定します（省略時はRight）。

アイコン画像は `Tiled/EnemyPlacementIcons/` にあります。マップ・タイルセット・画像フォルダーをセットで保持してください。編集後は通常どおりConvertLaboratory.cmdで変換します。

## ゲートの配置

タイルレイヤー Gates を選び、タイルセット OneWayGates の右矢印／左矢印を描いてください。1マスは通常ブロックと同じ1mです。同じ向きのタイルを縦に並べると一緒に開閉し、プレイヤー全体が矢印方向に通過すると閉じて戻れなくなります。通路入口とボス部屋入口には幅1マス・高さ3マスの右向きゲートを配置済みです。保存後は通常の ConvertLaboratory.cmd で変換します。詳しい仕様は ../../BOSS_GATE.md を参照してください。

ゲームは `LaboratoryCombinedCourse.stage.json` を読み込みます。旧 `LaboratoryCourse.tmj` は `ConvertLaboratory.cmd` で `ConvertedStage.stage.json` に変換する別マップです。統合版の変換では旧版JSONを上書きしません。

## 地形ブロックのバリエーション

Tiledで `LaboratoryCombinedCourse.tmj` を開き、タイルセット **LaboratoryBlocks** を選びます。
左から「オレンジ床（101）」「補強パネル（102）」「換気口（103）」です。
既存の地形レイヤーで通常ブロックと同様に描画・置換し、保存後に `ConvertLaboratoryCombined.cmd` で変換してください。
パレットが表示されない場合は、マップを開き直してください。

全種類が1×1×1で、当たり判定は既存ブロックと同じCubeです。
オレンジ床は上面のみ（横方向につながるよう90度回転）、補強パネルは上面と正面、換気口は正面に模様があります。
地形の位置・数・サイズは維持し、床の上端をオレンジ、縦の端と支柱模様を補強パネル、内部の一部を換気口に置き換えています。
背景の装飾壁とは異なり、これらは足場・壁キックに使う地形として配置します。

再出力ツール `Tools/export_stage_tiled.cjs` もID 100～103を保持します。
統合版の新ブロックGIDは15～17です（14はWalker）。旧版・再出力ツールでは14～16です。既存の敵・ゲートのGIDは変更していません。

検証: `node Tests/LaboratoryStageSyncTests.cjs` で両マップと各JSONの一致・地形形状の維持を確認します。
