#pragma once
#include <cstdint>
#include <functional>

/// <summary>
/// 時限発動クラス
/// </summary>
class TimedCall {
public:
	// コンストラクタ（コールバック関数と発動までの時間を指定）
	TimedCall(std::function<void()> callback, uint32_t time);

	// 更新処理
	void Update();

	// 完了なら true を返す
	bool IsFinished() const { return isFinished_; }

private:
	// コールバック関数
	std::function<void()> callback_;

	// 残り時間
	uint32_t time_ = 0;

	// 完了フラグ
	bool isFinished_ = false;
};