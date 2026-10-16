#pragma once

// シーン基底
class Scene
{
  public:

    // コンストラクタ
    Scene() = default;

    // デストラクタ
    virtual ~Scene() = default;

    // 初期化処理
    virtual void Initialize()
    {
    }

    // 終了処理
    virtual void Finalize()
    {
    }

    // 更新処理
    virtual void Update(float elapsedTime)
    {
    }

    // 描画処理
    virtual void Render(float elapsedTime)
    {
    }

    // GUI描画処理
    virtual void DrawGUI()
    {
    }

    virtual bool IsBackgroundEditing() const
    {
        return false;
    }
};
