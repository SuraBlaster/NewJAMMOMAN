# シーン移植

タイトルは後から画像付きの演出へ改造しました。現在のタイトルの仕様・操作は
[TITLE_ANIMATION.md](TITLE_ANIMATION.md)を参照してください。
以下の「表示素材」に記載したタイトルの簡易表示は、今回の改造前の内容です。

- 起動時は SceneTitle。Enter、ゲームパッドの Start / A / B / X / Y、または Start Game で開始します。
- タイトルの開始演出2秒 → SceneLoadingの表示4秒 → GameScene。
- SceneClear は文字送りを表示し、8秒後、Enter、Start / A、またはボタンでロード画面を経由してタイトルへ戻ります。
- 右側の Scene ウィンドウから Title / Clear (preview) / モデルビューア / ゲームを選択できます。
- ゲーム本体のクリア条件は未定義のため、自動クリア遷移は追加していません。条件成立時に次を呼び出してください。

```cpp
#include "SceneClear.h"
#include "SceneManager.h"

SceneManager::Instance().ChangeScene([] {
    return std::make_shared<SceneClear>();
});
```

## 管理とロード

Framework の更新・描画・GUI・終了を SceneManager に統一しました。
Scene に既定の Initialize / Finalize を追加しています。既存の GameScene と
ModelViewerScene はコンストラクター・デストラクターで初期化と解放を続けます。

ChangeScene は遷移を予約し、次の Update の冒頭で古いシーンを終了・破棄してから
ファクトリーで次のシーンを生成します。特に GameScene は生成時に共有の
EnemyManager を変更するため、生成済みオブジェクトを渡さずラムダを渡してください。
これにより、古い GameScene の破棄が新しい敵を消去する問題を防ぎます。

SceneLoading もシーン生成用のラムダを受け取ります。
最低1フレームの画面表示後、通常0.5秒・特殊4秒の表示時間を待って遷移を予約します。
実際の生成・読み込みはメインスレッドで行うため、その間の表示アニメーションは停止します。
バックグラウンド読み込みには、GameSceneのデータ読み込みとGPU・共有状態の初期化の分離が必要です。

## 表示素材

移植元の TitlePlayer / LoadingPlayer / LoadingBoss / ClearPlayer / StageMain、
タイトル・クリア用画像、SpriteFont / SpriteBatch、音声システムはこのプロジェクトにありません。
画面表示は既存の ImGui とフォントを使用する構成に置き換えています。
元の3D演出・画像・BGMは再現していません。
SceneUI.h が各画面の背景・見出し・配置の共通処理です。

## 確認

Tests/SceneManagerTests.cpp は、遷移の予約、終了・破棄・生成の順序、
描画・GUIの呼び出し、Clearによる予約キャンセルを確認するテストです。
Visual Studio の x64 Native Tools Command Prompt でプロジェクト直下から実行できます。

```bat
cl /nologo /EHsc /std:c++17 /I Source Tests\SceneManagerTests.cpp Source\SceneManager.cpp /Fe:obj\SceneManagerTests.exe /Fo:obj\
obj\SceneManagerTests.exe
```
