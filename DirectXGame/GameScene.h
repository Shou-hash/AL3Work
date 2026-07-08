#pragma once
#include "CameraController.h"
#include "DeathParticles.h"
#include "Enemy.h"
#include "Kamataengine.h"
#include "MapChipField.h"
#include "Matrix4x4.h"
#include "Player.h"
#include "Skydome.h"
#include <list> // std::list を使用するため追加
#include <memory>
#include <vector>

enum class Phase {
	kPlay,
	kDeath,
};

class GameScene {
public:
	~GameScene();

	void Initialize();
	void Update();
	void Draw();
	KamataEngine::Camera& GetCamera() { return camera_; }

	void GenerateBlocks();

	// シーンが終了したかを取得
	bool isFinished() const { return finished_; }

private:
	// フェーズ管理用関数
	
	// フェーズの切り替え条件・処理を管理
	void ChangePhase();
	
	// ゲームプレイフェーズの毎フレーム処理
	void UpdatePlay();

	// デス演出フェーズの毎フレーム処理
	void UpdateDeath();

private:
	// フェイズの状態を管理する変数
	Phase phase_ = Phase::kPlay;

	bool finished_ = false;

	KamataEngine::Model* model_ = nullptr;
	std::vector<std::vector<KamataEngine::WorldTransform*>> worldTransformBlocks_;
	KamataEngine::Camera camera_{};
	KamataEngine::WorldTransform worldTransform_;
	bool isDebugCameraActive_ = false;
	KamataEngine::DebugCamera* debugCamera_ = nullptr;

	// 敵（Wizard）の3Dモデルポインタ
	KamataEngine::Model* modelEnemy_ = nullptr;

	// 単体のユニークポインタは複数管理では不要になるため削除、または互換性のために残す場合はそのまま
	// 今回はスライドの指示通り enemies_ リストで一括管理するため、単体用の enemy_ は使用しません。
	std::unique_ptr<Enemy> enemy_ = nullptr;

	std::unique_ptr<Skydome> skydome = nullptr;

	// ポインタではなく、安全な unique_ptr で管理
	std::unique_ptr<Player> player_ = nullptr;

	std::unique_ptr<CameraController> cameraController_ = nullptr;

	KamataEngine::Model* modelSkydome_ = nullptr;
	KamataEngine::Model* modelPlayer_ = nullptr;
	MapChipField* mapChipField_;

	std::list<Enemy*> enemies_; // 敵のリスト（複数の敵を管理）

	// デスパーティクルの3Dモデルポインタ
	KamataEngine::Model* modelDeathParticles_ = nullptr;
	// デスパーティクル（ユニークポインタ）
	std::unique_ptr<DeathParticles> deathParticles_;
};