#include "TimedCall.h"

TimedCall::TimedCall(std::function<void()> callback, uint32_t time) : callback_(callback), time_(time), isFinished_(false) {}

void TimedCall::Update() {
	// 完了なら処理を抜ける
	if (isFinished_) {
		return;
	}

	// カウントダウン
	if (time_ > 0) {
		--time_;
	}

	// 時間になったら（0になったら）
	if (time_ == 0) {
		isFinished_ = true; // 完了フラグを立てる

		// コールバック関数の呼び出し
		if (callback_) {
			callback_();
		}
	}
}