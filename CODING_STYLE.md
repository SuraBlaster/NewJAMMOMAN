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
- 英語コメントは追加しない。日本語を含む既存コメントは残す。
- コンストラクタ・デストラクタ・初期化・更新・描画・終了処理などの基本関数には、直前に日本語の短い説明コメントを付ける。既存の説明と重複させない。
- コメントの末尾には「。」を付けない。
- C++・シェーダーの整形にはルートの `.clang-format` を使う。
