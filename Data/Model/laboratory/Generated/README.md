# 研究所ステージ用モデル

元の FBX / glTF / bin は変更していません。`Tools/build_laboratory.cjs` から再生成できます。

```powershell
node Tools/build_laboratory.cjs
```

ゲーム座標で X=横、Y=高さ、Z=奥行き。生成モデルのノード変換は単位変換です。

| ファイル | 寸法 X / Y / Z | 原点 | タイルID |
|---|---|---|---|
| LabBlock_1m.gltf | 1 / 1 / 1 | 中心 | 100 |
| LabColumn_3m.gltf | 約0.460 / 3 / 約0.460 | 下端中央 | 201 |
| LabWall_1m.gltf | 1 / 1 / 0.13 | 中心 | 202 |

`1m` はこのプロジェクトの1単位を表す命名です。実世界のメートルとの対応を強制しません。

## 床

FloorTile_Basic のノード変換を頂点へ適用し、0.005倍して幅1にしています。
最上点をY=0.5にそろえ、Main / DarkGreyの色を使った側面と底面を追加しました。
全体の範囲は各軸 -0.5～0.5 です。元素材の法線を変換し、描画用接線も生成します。

TileTypes.json の typeId=100 が生成ブロックを参照します。既存のステージ配置ファイルは変更していません。
collisionModelPath は従来の Cube.gltf を参照し、AABBと三角形衝突の両方に使います。
装飾メッシュを衝突に使わないため、足場の高さと元の衝突形状を維持します。
現在の ConvertedStage は293個すべてscale=(1,1)です。各ブロックを繰り返すので模様は伸びません。
今後長い足場を作る場合も1マスずつ配置してください。scaleを増やすと表示モデルも伸びます。

## 柱と壁

柱はColumn_2を縦横比を保って高さ3へ正規化しました。
壁は湾曲素材を使わず、新規作成した平面パネルに床と同じ2色を使用しています。
201 / 202 は装飾用（collision=false）として登録済みですが、実ステージにはまだ配置していません。
柱の原点Yを床の上面に合わせます。プレビューの配置は組み合わせ例です。
既存StageはZ=0固定のため、背景として奥に配置する際には別途Z配置への対応が必要です。

## 描画と検証

ModelShaderのC++定数バッファに合わせてHLSLへmaterialColorを追加し、glTFのbaseColorFactorを反映しています。
従来のステージ一律0.5倍の色指定は白に変更し、素材の灰色を二重に暗くしないようにしました。
このシェーダー修正は他のモデルにも正しいマテリアル係数を適用します。

Tests/LaboratoryRenderTests.cpp は実際のモデルローダー・描画処理で素材を読み込み、
全293配置の衝突範囲を従来のCubeと比較します。
preview.png は組み合わせ例、stage-preview.png は実ステージ配置の描画です。
対話操作によるプレイテストは別途必要です。

元素材: Quaternius Modular Sci-Fi Megakit (CC0)
https://quaternius.com/packs/modularscifimegakit.html
