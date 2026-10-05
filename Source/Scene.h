#pragma once

// シーン基底
class Scene
{
public:
	Scene() = default;
	virtual ~Scene() = default;

	virtual void Initialize() {}
	virtual void Finalize() {}

	// 更新処理
	virtual void Update(float elapsedTime) {}

	// 描画処理
	virtual void Render(float elapsedTime) {}

	// GUI描画処理
	virtual void DrawGUI() {}
    virtual bool IsBackgroundEditing() const { return false; }
};
