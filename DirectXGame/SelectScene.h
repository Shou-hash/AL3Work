#pragma once
#include "Fade.h"
#include "Kamataengine.h"
#include "Skydome.h"
#include <memory>

class StageManager;

class SelectScene {

public:
	enum class Phase {
		FadeIn,  // フェードイン中
		Normal,  // 通常（キー入力待ち）
		FadeOut, // フェードアウト中
	};

	~SelectScene();

	void Initialize(StageManager* stageDataManager);
	void Update();
	void Draw();

	bool IsFinished() const { return finished_; }

private:
	bool finished_ = false;
	Phase phase_ = Phase::FadeIn;

	Fade* fade_ = nullptr;

	KamataEngine::Camera camera_{};

	// モデルポインタ
	KamataEngine::Model* modelSkydome_ = nullptr;
	std::unique_ptr<Skydome> skydome = nullptr;

	// ステージマネージャ参照用のポインタ
	StageManager* stageManager_ = nullptr;

	// 選択中のステージインデックス
	int32_t currentSelectIndex_ = 0;
};