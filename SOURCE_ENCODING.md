# ソースの文字コード

自作のC++・ヘッダーはBOM付きUTF-8、HLSLシェーダーはFXCに合わせてBOMなしUTF-8に統一しています。
GameのDebug・Releaseには `/utf-8` を設定済みです。StageConverterにも同じ設定があります。
通常のGame.sln／Game.vcxprojを使ってビルドできます。一時的な文字コード変換用ソースコピーは不要です。

`.editorconfig` に保存形式を指定しているため、Visual Studioで編集する際もこの形式を維持してください。
External配下の外部ライブラリと、モデル・画像・音声・ステージデータは変換していません。

変換前の229ファイルとプロジェクト設定などは `output/encoding-backup/20261016-064305/original-files.zip` に保存しています。
同じフォルダのmanifest.jsonにはファイルごとの変換前の文字コードとハッシュを記録しています。
文字内容と改行コードを変換前後で比較し、すべて一致することを確認しています。

再確認用:

```text
python Tools/normalize_source_encoding.py
```

引数なしでは確認のみを行います。`--apply` を付けるとバックアップを作成して変換します。
厳密にUTF-8またはCP932として読み取れないファイルがある場合は、書き込み前に停止します。

確認済み:

- 通常のGameプロジェクトとStageConverterのDebug・Releaseリビルド
- コンパイル時の文字コードエラー・警告が出ないこと
- ゲーム初期化、FlyEnemyShotのリソース読み込みと消滅処理
- 日本語を含む出力先へのステージ変換と、現在の敵配置との一致
