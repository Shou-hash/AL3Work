#pragma once
#include "BackgroundBlocks.h" // ★ 追加
#include "Fade.h"
#include "Kamataengine.h"
#include "LightManager.h" // ★ 追加
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
	// ★ 軽量化: カメラの周辺の列のブロックだけ WorldTransform を作る（まだ作っていない列だけ作る）
	void EnsureBlockTransforms(float centerX);

	bool finished_ = false;
	Phase phase_ = Phase::FadeIn;

	Fade* fade_ = nullptr;

	// タイトル演出用プレイヤーとカメラ
	Player* player_ = nullptr;
	KamataEngine::Camera camera_{};

	// ★ タイトルステージ描画用の変数群を追加
	MapChipField* mapChipField_ = nullptr;
	KamataEngine::Model* model_ = nullptr; // 通常ブロック
	std::vector<std::vector<KamataEngine::WorldTransform*>> worldTransformBlocks_;
	std::vector<bool> columnReady_; // ★ 軽量化: 列ごとに WorldTransform を作成済みか

	// ★ ブロックの向き別モデル（GameScene と同じ判定で使い分ける）
	KamataEngine::Model* modelBlockAbove_ = nullptr;             // 地面用（上が空白）
	KamataEngine::Model* modelBlockBelow_ = nullptr;             // 天井用（下が空白）
	KamataEngine::Model* modelBlockLeft_ = nullptr;              // 右壁用（左が空白）
	KamataEngine::Model* modelBlockRight_ = nullptr;             // 左壁用（右が空白）
	std::vector<std::vector<KamataEngine::Model*>> blockModels_; // 各ブロックが使うモデル

	// ★ 奥行き背景と朝のライティング
	std::unique_ptr<BackgroundBlocks> backgroundBlocks_ = nullptr;
	std::unique_ptr<LightManager> lightManager_ = nullptr;

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
