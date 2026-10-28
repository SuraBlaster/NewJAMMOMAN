# モデルの .cereal 変換

ゲームで使用する25種類のモデルは .cereal を参照します。元の .gltf、.bin、テクスチャは残しています。

モデルを更新したら、プロジェクトのルートで次を実行してください。

```bat
Tools\ConvertModels.cmd
```

Tools/ModelCerealSources.txt にある元データから .cereal を再生成し、Direct3D の WARP デバイスで読み戻して、メッシュ・ボーン・アニメーション構成、GPUバッファ、テクスチャファイルの存在を検証します。Walker/robot は従来どおり24fps、それ以外は60fpsで変換します。Visual Studio 2022 Community の C++ 開発環境が必要です。

.cereal にはモデルとアニメーションを保存し、テクスチャは相対パスで参照します。提出時は Data 内のテクスチャも必ず同梱してください。元データを除いた提出用コピーを作る場合も、Textures などの画像フォルダーは残してください。

新しい形式は JAMMDL01 ヘッダーで旧データと区別します。.cereal が欠落または非互換の場合は読み込みエラーになります。元の .gltf/.glb を明示指定したときは元データを直接読み込み、古い .cereal は利用しません。