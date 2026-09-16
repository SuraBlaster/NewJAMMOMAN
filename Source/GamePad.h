#pragma once

using GamePadButton = unsigned int;

// ゲームパッド
class GamePad
{
public:
	static const GamePadButton BTN_UP				= (1 <<  0);
	static const GamePadButton BTN_RIGHT			= (1 <<  1);
	static const GamePadButton BTN_DOWN				= (1 <<  2);
	static const GamePadButton BTN_LEFT				= (1 <<  3);
	static const GamePadButton BTN_A				= (1 <<  4);
	static const GamePadButton BTN_B				= (1 <<  5);
	static const GamePadButton BTN_X				= (1 <<  6);
	static const GamePadButton BTN_Y				= (1 <<  7);
	static const GamePadButton BTN_START			= (1 <<  8);
	static const GamePadButton BTN_BACK				= (1 <<  9);
	static const GamePadButton BTN_LEFT_THUMB		= (1 << 10);
	static const GamePadButton BTN_RIGHT_THUMB		= (1 << 11);
	static const GamePadButton BTN_LEFT_SHOULDER	= (1 << 12);
	static const GamePadButton BTN_RIGHT_SHOULDER	= (1 << 13);
	static const GamePadButton BTN_LEFT_TRIGGER		= (1 << 14);
	static const GamePadButton BTN_RIGHT_TRIGGER	= (1 << 15);

private:
	GamePad() {}
	~GamePad() {}

public:
	static GamePad& Instance()
	{
		static GamePad instance;
		return instance;
	}

	// 更新
	void Update();

	// スロット設定
	void SetSlot(int slot) { this->slot = slot; }

	// ボタン入力状態の取得
	GamePadButton GetButton() const { return button_state[0]; }

	// ボタン押下状態の取得
	GamePadButton GetButtonDown() const { return button_down; }

	// ボタン押上状態の取得
	GamePadButton GetButtonUp() const { return button_up; }

	// 左スティックX軸入力状態の取得
	float GetAxisLX() const { return axis_lx; }

	// 左スティックY軸入力状態の取得
	float GetAxisLY() const { return axis_ly; }

	// 右スティックX軸入力状態の取得
	float GetAxisRX() const { return axis_rx; }

	// 右スティックY軸入力状態の取得
	float GetAxisRY() const { return axis_ry; }

	// 左トリガー入力状態の取得
	float GetTriggerL() const { return trigger_l; }

	// 右トリガー入力状態の取得
	float GetTriggerR() const { return trigger_r; }

private:
	GamePadButton		button_state[2] = { 0 };
	GamePadButton		button_down = 0;
	GamePadButton		button_up = 0;
	float				axis_lx = 0.0f;
	float				axis_ly = 0.0f;
	float				axis_rx = 0.0f;
	float				axis_ry = 0.0f;
	float				trigger_l = 0.0f;
	float				trigger_r = 0.0f;
	int					slot = 0;
};
