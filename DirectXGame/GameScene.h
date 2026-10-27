#pragma once
#include "BackgroundBlocks.h"
#include "BaseEffect.h"
#include "BaseEnemy.h"
#include "BossEnemy.h"
#include "CameraController.h"
#include "CrystalArrow.h" // ★ 追加: 攻撃できるクリスタルを指す矢印クラスのインクルード
#include "Enemy.h"
#include "Fade.h"
#include "FinalBoss.h"
#include "Font.h"
#include "Goal.h"
#include "Item.h"
#include "Kamataengine.h"
#include "LightManager.h"
#include "MapChipField.h"
#include "Matrix4x4.h"
#include "Pause.h" // ★ 追加: ポーズクラスのインクルード
#include "Player.h"
#include "PlayerHp.h"
#include "PlayerTutorial.h" // ★ 追加: チュートリアルクラスのインクルード
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
	// ★ 追加: 雲のふわふわ揺れアニメーション用パラメータ
	KamataEngine::Vector3 basePosition = {0.0f, 0.0f, 0.0f}; // 揺れの基準位置
	float offsetY = 0.0f;                                    // 現在のYオフセット
	float velocityY = 0.0f;                                  // 揺れの速度
};

class GameScene {
public:
	~GameScene();

	// ★ 初期化関数で StageManager のポインタを受け取るように変更
	void Initialize(StageManager* stageDataManager);
	void Update();
	void Draw();
	KamataEngine::Camera& GetCamera() { return camera_; }

	// ★ 軽量化: ゲームで使うモデルの事前読み込み（タイトル・セレクト画面の待ち時間に毎フレーム呼ぶと、1 フレームにつき 1 個ずつ読み込む）
	static void PreloadModelsStep();

	void GenerateFieldObjects();
	void GenerateEnemy(uint32_t xIndex, uint32_t yIndex);

	bool isFinished() const { return finished_; }
	bool IsReloadRequested() const { return reloadRequested_; }
	bool IsSelectRequested() const { return isSelectRequested_; } // ★ 追加: ポーズからのセレクト画面遷移要求フラグを取得
	bool IsTitleRequested() const { return isTitleRequested_; }   // ★ 追加: ポーズからのタイトル画面遷移要求フラグを取得
	bool IsExitRequested() const { return isExitRequested_; }     // ★ 追加: ポーズからのゲーム終了要求フラグを取得

	void ChangePhase();
	void UpdateFadeIn();
	void UpdatePlay();
	void UpdateDeath();
	void UpdateFadeOut();

	// ★ 光の演出の更新（ラスボス接近で点灯、出現演出中に夜になる、追従スポットライトの配置）
	void UpdateLightEffects();

	// ★ 追加: 雲のふわふわ揺れアニメーション更新関数
	void UpdateClouds();

	// ★ 追加: 攻撃できるクリスタルを指す矢印の更新
	void UpdateCrystalArrow();

	// ★ 中ボスの処刑演出（撃破後にカメラがズームインし、Eキー連打で連撃 ➔ ハンマーでホームラン）
	void StartExecution(BossEnemy* boss);
	void UpdateExecution();

	// ★ ラスボスの処刑演出（撃破後にカメラが大きく引き、Eキー長押しで力を溜め ➔ 一撃でクリスタルごと砕く）
	void StartFinalExecution(FinalBoss* boss);
	void UpdateFinalExecution();

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

	// ボス用モデルポインタ
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

	// ステージタイルの雲用モデルポインタ (cloud3)
	KamataEngine::Model* modelCloud_ = nullptr;

	// ★ 追加: 攻撃できるクリスタルを指す矢印のモデルポインタ（ラスボスのステージだけ読み込む）
	KamataEngine::Model* modelArrow_ = nullptr;

	// クラスメンバ変数にモデルポインタを追加(ラスボス)
	KamataEngine::Model* modelFinalBossBody_ = nullptr;
	std::array<KamataEngine::Model*, FinalBoss::kNumHands> modelFinalBossHands_{};
	std::array<KamataEngine::Model*, FinalBoss::kNumCrystals> modelFinalBossCrystals_{}; // ★ 追加 (22個)
	KamataEngine::Model* modelFinalBossCrystal22_ = nullptr;                             // ★ 攻撃専用 finalBossCrystal_22
	bool crystal22AttackConsumed_ = false;                                               // ★ 1回の攻撃判定で多重ヒットしないためのフラグ
	bool bossHitConsumed_ = false;                                                       // ★ 追加: 中ボスへの1回の攻撃での多重ヒット防止フラグ

	// 2次元配列の型を BlockData* に変更
	std::vector<std::vector<BlockData*>> blockDatas_;

	// C0から生成されるステージ配置用雲配列（通常サイズ）
	std::vector<BlockData*> cloudDatas_;

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

	// ★ 追加: フォント演出専用クラスのインスタンス
	std::unique_ptr<Font> comboFont_ = nullptr;

	// ★ 追加: ポーズUI・看板演出制御専用クラスのインスタンス
	std::unique_ptr<Pause> pause_ = nullptr;

	// ★ 追加: 攻撃できるクリスタルを指す矢印のインスタンス（ラスボスのステージだけ）
	std::unique_ptr<CrystalArrow> crystalArrow_ = nullptr;

	bool reloadRequested_ = false;
	bool isSelectRequested_ = false; // ★ 追加: ポーズからのセレクト画面遷移要求フラグ
	bool isTitleRequested_ = false;  // ★ 追加: ポーズからのタイトル画面遷移要求フラグ
	bool isExitRequested_ = false;   // ★ 追加: ポーズからのゲーム終了要求フラグ

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

	// ★ 処刑演出の進行
	enum class ExecutionPhase {
		kNone,         // 通常（演出なし）
		kZoomIn,       // 中ボスへズームイン
		kHammering,    // Eキー連打で連撃
		kFinaleCharge, // 最後の一撃のため（カメラがさらに寄り、中ボスが震える）
		kFinaleSwing,  // ハンマーを振りかぶって思い切り叩く
		kLaunch,       // 中ボスがホームランで空へ飛んでいく
	};

	ExecutionPhase executionPhase_ = ExecutionPhase::kNone;
	BossEnemy* executionBoss_ = nullptr;  // 処刑される中ボス
	float executionTimer_ = 0.0f;         // 各段階の経過時間
	float executionIdleTimer_ = 0.0f;     // Eキーが押されていない時間（放置しても進むようにする）
	int32_t executionHitCount_ = 0;       // 連撃が当たった回数
	bool executionInputBuffered_ = false; // 先行入力（スイング中に押された分を 1 回だけ保持）
	float executionPushDirX_ = 1.0f;      // 中ボスが吹き飛ぶ向き（+1 右 / -1 左）

	// ★ ラスボスの処刑演出の進行（中ボスの連打とは違い、長押しで溜めて一撃で砕く）
	enum class FinalExecutionPhase {
		kNone,     // 通常（演出なし）
		kPullBack, // カメラが大きく引いて、ラスボスとプレイヤーを同時に映す
		kCharge,   // Eキー長押しで力を溜める（クリスタルが本体へ吸い込まれる）
		kStrike,   // 溜めきったらハンマーを振り下ろす
		kShatter,  // クリスタルが砕け散り、カメラがさらに引いていく
	};
	FinalExecutionPhase finalExecutionPhase_ = FinalExecutionPhase::kNone;
	FinalBoss* executionFinalBoss_ = nullptr; // 処刑されるラスボス
	float finalExecutionTimer_ = 0.0f;        // 各段階の経過時間
	float finalExecutionIdleTimer_ = 0.0f;    // Eキーが押されていない時間（放置しても進むようにする）
	float finalExecutionCharge_ = 0.0f;       // 溜めの進行度（0.0 ~ 1.0）
	float finalExecutionRumbleTimer_ = 0.0f;  // 溜めている間のカメラの揺れの間隔用タイマー
	float finalExecutionPushDirX_ = 1.0f;     // ラスボスがのけぞる向き（+1 右 / -1 左）
	bool isFinalExecutionDone_ = false;       // ラスボスの処刑演出が終わったか（二度始めない）

	// 背景ブロック管理クラスのインスタンス
	std::unique_ptr<BackgroundBlocks> backgroundBlocks_ = nullptr;

	// ★ 追加: 操作解説用チュートリアル管理クラス
	std::unique_ptr<PlayerTutorial> playerTutorial_ = nullptr;

	// ★ 追加: ポーズ看板からの選択項目保持用変数
	Pause::MenuType pendingPauseMenu_ = Pause::MenuType::Count;
};