#pragma once
#include "Fade.h"
#include "Kamataengine.h"
#include "MapChipField.h" // ★ 追加
#include "Player.h"
#include "Skydome.h"
#include <memory>
#include <vector> // ★ 追加

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

	// タイトル演出用プレイヤーとカメラ
	Player* player_ = nullptr;
	KamataEngine::Camera camera_{};

	// ★ タイトルステージ描画用の変数群を追加
	MapChipField* mapChipField_ = nullptr;
	KamataEngine::Model* model_ = nullptr;
	std::vector<std::vector<KamataEngine::WorldTransform*>> worldTransformBlocks_;

	// モデルポインタ
	KamataEngine::Model* modelPlayerHead_ = nullptr;
	KamataEngine::Model* modelPlayerBody_ = nullptr;
	KamataEngine::Model* modelPlayerLeft_ = nullptr;
	KamataEngine::Model* modelPlayerRight_ = nullptr;

	// ★ スカイドーム用変数を追加
	KamataEngine::Model* modelSkydome_ = nullptr;
	std::unique_ptr<Skydome> skydome = nullptr;

	// ★ タイトル名用変数を追加
	KamataEngine::Model* modelTitleName_ = nullptr;
	KamataEngine::WorldTransform worldTransformTitleName_;
};