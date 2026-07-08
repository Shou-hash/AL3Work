#pragma once
#include <KamataEngine.h>

class Fade {
public:
	// フェードの状態
	enum class Status {
		None,    // 非表示状態
		FadeIn,  // フェードイン中（徐々に明るくなる・アルファ値が減る）
		FadeOut, // フェードアウト中（徐々に暗くなる・アルファ値が増える）
	};

	void Initialize();
	void Update();
	void Draw();

	// フェードを開始する
	void Start(Status status, float duration);

	// フェードが終了しているかどうか
	bool IsFinished() const;

	// 現在の状態を取得
	Status GetStatus() const { return status_; }

private:
	KamataEngine::Sprite* sprite_ = nullptr;

	// フェードの状態を管理する変数
	Status status_ = Status::None;

	// フェードの経過時間タイマー
	float counter_ = 0.0f;

	// フェードにかける合計時間
	float duration_ = 0.0f;
};