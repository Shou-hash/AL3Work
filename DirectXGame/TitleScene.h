#pragma once
#include "Fade.h"

class TitleScene {

public:
	
	enum class Phase {
		FadeIn,  // フェードイン中
		Normal,  // 通常（キー入力待ち）
		FadeOut, // フェードアウト中
	};

	~TitleScene();

	void Initialize();
	void Update();
	void Draw();
	

	bool IsFinished() const { return finished_; }

private:

	bool finished_ = false;
	Phase phase_ = Phase::FadeIn;

	Fade* fade_ = nullptr;
};
