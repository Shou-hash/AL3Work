#pragma once
#include "Fade.h"
#include "Kamataengine.h"
#include "Skydome.h"
#include <memory>
#include <vector>

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

	// ★ ステージ選択モデル用（Stage1.obj, Stage2.obj, Stage3.obj）のモデルポインタ配列
	static inline const int32_t kNumStages = 3;
	KamataEngine::Model* stageSprites_[kNumStages] = {nullptr};
	KamataEngine::WorldTransform worldTransforms_[kNumStages];

	// ★ 演出用変数
	float animationTimers_[kNumStages] = {0.0f}; // 各ステージのスケール補間用タイマー
	float flashTimer_ = 0.0f;                    // 選択中ステージの明滅用タイマー

	// ★ 各ステージモデルのランダム回転速度（X, Y, Z軸）
	KamataEngine::Vector3 rotationSpeeds_[kNumStages] = {};
};