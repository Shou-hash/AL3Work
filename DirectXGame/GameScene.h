#pragma once
#include "BaseEffect.h"
#include "BaseEnemy.h"
#include "BossEnemy.h"
#include "CameraController.h"
#include "Enemy.h"
#include "Fade.h"
#include "Item.h" // ★追加：Itemクラスのインクルード
#include "Kamataengine.h"
#include "MapChipField.h"
#include "Matrix4x4.h"
#include "Player.h"
#include "PlayerHp.h"
#include "ShieldEnemy.h"
#include "Skydome.h"
#include <list>
#include <memory>
#include <vector>

// ★ クラスの前方宣言
class StageManager;

enum class Phase {
	kFadeIn,  // フェードイン
	kPlay,    // ゲームプレイ
	kDeath,   // デス演出
	kFadeOut, // フェードアウト
};

class GameScene {
public:
	~GameScene();

	// ★ 初期化関数で StageManager のポインタを受け取るように変更
	void Initialize(StageManager* stageDataManager);
	void Update();
	void Draw();
	KamataEngine::Camera& GetCamera() { return camera_; }

	void GenerateFieldObjects();
	void GenerateEnemy(uint32_t xIndex, uint32_t yIndex);

	bool isFinished() const { return finished_; }
	bool IsReloadRequested() const { return reloadRequested_; }

	void ChangePhase();
	void UpdateFadeIn();
	void UpdatePlay();
	void UpdateDeath();
	void UpdateFadeOut();

private:
	Phase phase_ = Phase::kFadeIn;
	bool finished_ = false;

	Fade* fade_ = nullptr;

	KamataEngine::Model* model_ = nullptr;
	std::vector<std::vector<KamataEngine::WorldTransform*>> worldTransformBlocks_;
	KamataEngine::Camera camera_{};
	KamataEngine::WorldTransform worldTransform_;
	bool isDebugCameraActive_ = false;
	KamataEngine::DebugCamera* debugCamera_ = nullptr;

	// モデルポインタ
	KamataEngine::Model* modelEnemy_ = nullptr;
	KamataEngine::Model* modelShieldEnemy_ = nullptr;
	KamataEngine::Model* modelSkydome_ = nullptr;

	// 4つの各部位のOBJファイル用モデルポインタに変更
	KamataEngine::Model* modelPlayerHead_ = nullptr;
	KamataEngine::Model* modelPlayerBody_ = nullptr;
	KamataEngine::Model* modelPlayerLeft_ = nullptr;
	KamataEngine::Model* modelPlayerRight_ = nullptr;

	// ★追加：ボス用モデルポインタ
	KamataEngine::Model* modelBossHead_ = nullptr;
	KamataEngine::Model* modelBossBody_ = nullptr;
	KamataEngine::Model* modelBossLeft_ = nullptr;
	KamataEngine::Model* modelBossRight_ = nullptr;

	KamataEngine::Model* modelDeathParticles_ = nullptr;
	KamataEngine::Model* modelHitEffect_ = nullptr;
	KamataEngine::Model* modelHammer_ = nullptr;

	KamataEngine::Model* modelPlayerHp_ = nullptr;
	KamataEngine::Model* modelItemHp_ = nullptr; // ★追加：ドロップアイテム用モデルポインタ

	std::unique_ptr<Skydome> skydome = nullptr;
	std::unique_ptr<Player> player_ = nullptr;
	std::unique_ptr<CameraController> cameraController_ = nullptr;
	std::unique_ptr<PlayerHp> playerHp_ = nullptr;
	MapChipField* mapChipField_ = nullptr;

	std::list<BaseEnemy*> enemies_;
	std::list<BaseEffect*> effects_;
	std::list<Item*> items_; // ★追加：ドロップアイテムリスト

	bool reloadRequested_ = false;

	// ★ ステージマネージャ参照用のポインタ
	StageManager* stageManager_ = nullptr;

	// ボス出現判定用フラグ
	bool isBossSpawned_ = false;
};