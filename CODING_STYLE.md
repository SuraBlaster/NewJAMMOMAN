# Naming conventions

- Types, functions, and enum values use `PascalCase`: `Player`, `UpdateVelocity`, `WallSlide`.
- Variables, members, and parameters use `camelCase`: `moveSpeed`, `elapsedTime`, `isGround`.
- Fixed constants use `UPPER_SNAKE_CASE`: `DASH_DURATION`, `SWORD_COLLISION_RADIUS`.
- Runtime values remain `camelCase` even when declared `const`.
- Use the same parameter names in declarations and definitions.
- Keep names required by external APIs, C++ conventions, and shader entry points: `serialize`, `main`, `WinMain`, standard-library methods, and external-library members.
- Preserve JSON keys, asset names, shader semantics, and serialized field names. When renaming a serialized member, explicitly preserve its stored key.
- Apply the same conventions to generated C++ code and update its generator together with the output.

自作のC++・ヘッダーはBOM付きUTF-8で保存し、既存の改行コードを維持する。C++コンパイラには `/utf-8` を指定する。HLSLシェーダーはFXCに合わせてBOMなしUTF-8とする。`External` の外部ライブラリは配布元の文字コード・形式を維持する。

## コードの配置とコメント

- 関連する宣言・処理をまとめ、関数や処理ブロックの間には空行を入れる。
- `if`、`else`、ループ、関数などの `{` と `}` はそれぞれ独立した行に置く。
- 処理を1行に詰めず、制御文の本体には波括弧を付ける。`return` だけの分岐は省略形でもよい。
- 関数の引数に渡す短いラムダ、特に `return` だけのものは、呼び出しと同じ行に簡潔に書いてよい。
- 中身が空の `{}` は、コンストラクタや空の関数なども含めて1行で書いてよい。
- `public:`・`private:`・`protected:` は、それを含むクラスの宣言行と同じ深さに揃える。ラベル直後には空行を入れない。
- 英語コメントは追加しない。日本語を含む既存コメントは残す。
- コンストラクタ・デストラクタ・初期化・更新・描画・終了処理などの基本関数には、直前に日本語の短い説明コメントを付ける。既存の説明と重複させない。
- コメントの末尾には「。」を付けない。
- C++・シェーダーの整形にはルートの `.clang-format` を使う。
- 時間管理には、可能な限り秒・ミリ秒を表す数値を使う。高精度な時刻の取得は共通関数にまとめ、利用側で複雑な時間型や変換を扱わない。
- `.cpp` の補助関数は、匿名 `namespace` で囲まず、ファイル内の `static` 関数として書く。

## include の配置

- 標準・OS・外部ライブラリは `<...>`、プロジェクト内のヘッダーは `"..."` とし、`<...>` のグループを上に置く。
- 外部ヘッダーは OS、DirectX、入力・音声、標準ライブラリ、データ形式、GUI・エフェクトなど、関連するものごとにまとめる。
- プロジェクト内では、その `.cpp` に対応するヘッダーを先頭に置き、基底クラス・基盤機能、シーン、キャラクター、衝突判定、ステージ、描画、補助機能の順を目安にまとめる。
- include 同士の間には空行を入れない。並び順で関連するものをまとめ、基盤となる依存先や中心となる機能を先に置く。
- ヘッダー自身が使用する標準ライブラリは直接 include し、別のヘッダーによる間接的な読み込みに頼らない。
- 条件付き include やライブラリ実装用の `#define` の境界をまたいで移動しない。
