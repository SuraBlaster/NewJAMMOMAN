# 敵15体でのDebug版の処理落ち調査

計測日: 2026-10-15

## 原因と修正

`Character::UpdateGroundPhysics` は、足元の地形AABBに加えて `CollisionManager::Raycast` でモデルの三角形も確認する。
`StageObject` は当たり判定のある各ブロックをCollisionMeshとして登録するため、敵1体の接地判定ごとにステージ全体の3,000個以上のモデルを走査していた。
旧処理では遠く離れたモデルにも逆行列計算と三角形交差判定を行い、地上敵が増えるとCPU更新時間が大きく増えていた。
Walkerは低FPS時に物理更新を分割するため、この負荷も増幅されていた。

修正では、StageObjectが既に持っているワールド座標の当たり判定AABBをCollisionMeshから参照する。
Raycastは判定線を囲むAABBと地形AABBが交差するかを最初に確認し、離れたモデルを精密判定の前に除外する。
候補となったモデルの三角形判定、最寄りのヒット選択、bounds未指定の従来のCollisionMeshの動作は維持する。

## 実ゲームでの比較

`Tests/EnemyFramePerformanceBench.cpp` から実際のDebug版オブジェクトをリンクし、1280×720の非表示ウィンドウでGameSceneを実行。
10フレームのウォームアップ後45フレームを計測。GPU時間はD3D11のtimestamp queryを使用。
画面表示待ち（Present／VSync）は含まないため、数値をそのまま実プレイのFPSには換算しない。

| 条件 | CPU更新・修正前 | CPU更新・修正後 | 描画CPU・修正前→後 | GPU・修正前→後 |
|---|---:|---:|---:|---:|
| 現在の混在ステージ | 41.7493ms | 0.915931ms | 3.17676→3.06883ms | 1.76476→2.01626ms |
| 同じステージ・敵なし | 0.200958ms | 0.203111ms | 2.57372→2.74378ms | 1.43483→1.18807ms |
| Walker15体を同時起動 | 95.1008ms | 0.759504ms | 3.39763→2.83724ms | 2.94097→1.52273ms |

混在ステージは開始時にボス以外15体。通常の消滅処理があるため、この計測区間では14体。
Walker負荷テストは計測中も15体を維持。
GPU・描画時間には計測時の変動があり、今回の改善箇所はCPU更新処理。

ログ:

- `obj/enemy-frame-before.log`
- `obj/enemy-frame-after.log`
- 再計測: `node obj/prepare-frame-bench.cjs` の後、VS開発環境で `obj/bench-enemy-frame.cmd <ログ名>` を実行。

## 正しさの確認

`Tests/CollisionMeshBoundsTests.cpp` で回転・拡縮した24個の実モデルに2,000本の判定線を使用。
boundsによる除外あり／なしで、衝突の有無・最寄り距離・床高さが一致。うち855本で衝突を確認。
長さ0の判定線も確認。

モデルやテクスチャの削減、敵の数・配置・行動の変更は行っていない。
