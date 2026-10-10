#pragma once
#include "BackgroundBlocks.h"
#include "BaseEffect.h"
#include "BaseEnemy.h"
#include "BossEnemy.h"
#include "CameraController.h"
#include "Enemy.h"
#include "Fade.h"
#include "FinalBoss.h"
#include "Goal.h" // ★追加：Goalクラスのインクルード
#include "Item.h" // ★追加：Itemクラスのインクルード
#include "Kamataengine.h"
#include "LightManager.h"
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

struct BlockData {
	KamataEngine::WorldTransform* transform = nullptr;
	KamataEngine::Model* model = nullptr;
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

	// ★ 光の演出の更新（ラスボス接近で点灯、出現演出中に夜になる、追従スポットライトの配置）
	void UpdateLightEffects();

private:
	Phase phase_ = Phase::kFadeIn;
	bool finished_ = false;

	Fade* fade_ = nullptr;

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

	// 　ボス用モデルポインタ
	KamataEngine::Model* modelBossHead_ = nullptr;
	KamataEngine::Model* modelBossBody_ = nullptr;
	KamataEngine::Model* modelBossLeft_ = nullptr;
	KamataEngine::Model* modelBossRight_ = nullptr;

	KamataEngine::Model* modelDeathParticles_ = nullptr;
	KamataEngine::Model* modelHitEffect_ = nullptr;
	KamataEngine::Model* modelHammer_ = nullptr;

	KamataEngine::Model* modelPlayerHp_ = nullptr;
	KamataEngine::Model* modelItemHp_ = nullptr;      // ドロップアイテム用モデルポインタ
	KamataEngine::Model* modelGoal_ = nullptr;        // ゴール用モデルポインタ
	KamataEngine::Model* modelExplanation_ = nullptr; // 解説ブロック用モデルポインタ

	// ブロック用の各モデルポインタ
	KamataEngine::Model* modelBlock_ = nullptr;          // 通常ブロック
	KamataEngine::Model* modelBlockFall_ = nullptr;      // すり抜けブロックモデル
	KamataEngine::Model* modelBlockFallLeft_ = nullptr;  // すり抜けブロック左端モデル
	KamataEngine::Model* modelBlockFallRight_ = nullptr; // すり抜けブロック右端モデル
	KamataEngine::Model* modelBlockLeft_ = nullptr;      // 右端用
	KamataEngine::Model* modelBlockRight_ = nullptr;     // 左端用
	KamataEngine::Model* modelBlockAbove_ = nullptr;     // 地面用
	KamataEngine::Model* modelBlockBelow_ = nullptr;     // 天井用

	// クラスメンバ変数にモデルポインタを追加(ラスボス)
	KamataEngine::Model* modelFinalBossBody_ = nullptr;
	std::array<KamataEngine::Model*, FinalBoss::kNumHands> modelFinalBossHands_{};

	// 2次元配列の型を BlockData* に変更
	std::vector<std::vector<BlockData*>> blockDatas_;

	std::vector<KamataEngine::WorldTransform*> worldTransformExplanations_; // 解説ブロック用ワールドトランスフォーム配列

	std::unique_ptr<Skydome> skydome = nullptr;
	std::unique_ptr<Player> player_ = nullptr;
	std::unique_ptr<CameraController> cameraController_ = nullptr;
	std::unique_ptr<PlayerHp> playerHp_ = nullptr;
	std::unique_ptr<Goal> goal_ = nullptr; // ★追加：ゴール
	MapChipField* mapChipField_ = nullptr;

	std::list<BaseEnemy*> enemies_;
	std::list<BaseEffect*> effects_;
	std::list<Item*> items_; // ★追加：ドロップアイテムリスト

	std::unique_ptr<LightManager> lightManager_ = nullptr; // ★ 追加

	bool reloadRequested_ = false;

	// ★ ステージマネージャ参照用のポインタ
	StageManager* stageManager_ = nullptr;

	// ボス出現判定用フラグ
	bool isBossSpawned_ = false;

	// ★ ゴール到達判定用フラグ
	bool isGoalReached_ = false;

	// ★ ボス撃破後のゴール演出開始フラグ
	bool isBossDefeatedGoalPerfStarted_ = false;

	// ★ ラスボスに近づいて光の演出が始まったかどうかのフラグ
	bool isBossLightTriggered_ = false;

	// ★ ラスボスにこの距離（X方向）まで近づいたら光の演出を開始する
	float bossLightApproachDistance_ = 40.0f;

	// 背景ブロック管理クラスのインスタンス
	std::unique_ptr<BackgroundBlocks> backgroundBlocks_ = nullptr;
};