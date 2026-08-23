#pragma once
#include "Kamataengine.h"

class Player;
class BossEnemy;

struct Rect {
	float left = 0.0f;
	float right = 1.0f;
	float bottom = 0.0f;
	float top = 1.0f;
};

// カメラの移動モード
enum class CameraMode {
	kFollow,       // プレイヤー追従モード
	kForcedScroll, // 強制スクロールモード
};

class CameraController {
public:
	// 初期化時にカメラのポインタを受け取るように変更
	void Initialize(KamataEngine::Camera* camera);

	void Update();
	void Reset();

	// 追従対象の設定
	void SetTarget(Player* target) { target_ = target; }

	// ボスの設定
	void SetBoss(BossEnemy* boss) { boss_ = boss; }

	void SetMovableArea(const Rect& area) { movableArea_ = area; }

	// モード管理用のアクセッサ
	void SetMode(CameraMode mode) { mode_ = mode; }
	CameraMode GetMode() const { return mode_; }

	// 画面の左端座標を取得する（プレイヤーの押し出し・挟まれ判定用）
	float GetCameraLeftX() const;

	// ボス演出中かどうかの判定
	bool IsBossPerformance() const { return isBossPerformance_; }

	// ★追加：ゴール演出の開始と判定
	void StartGoalPerformance(const KamataEngine::Vector3& goalPos);
	bool IsGoalPerformance() const { return isGoalPerformance_; }

private:
	// モードごとの更新処理
	void UpdateFollow();
	void UpdateForcedScroll();
	void UpdateBossPerformance();
	void UpdateGoalPerformance(); // ★追加：ゴール演出更新

	// プレイヤーを画面内に押し戻す処理
	void ConstrainPlayerInScreen();

private:
	KamataEngine::Camera* camera_ = nullptr;
	Player* target_ = nullptr;
	BossEnemy* boss_ = nullptr;
	KamataEngine::Vector3 targetOffset_ = {0.0f, 0.5f, -15.0f};

	KamataEngine::Vector3 targetPosition_;

	static inline const float kInterpolationRate = 0.3f;
	static inline const float kVelocityBias = 5.0f;

	Rect movableArea_ = {5, 100, 0, 100};
	static inline const Rect margin_ = {-2.0f, 2.0f, -1.0f, 2.0f};

	// 追加パラメータ
	CameraMode mode_ = CameraMode::kFollow;             // 現在のカメラモード
	static inline const float kScrollSpeed = 0.03f;     // 強制スクロールの速度
	static inline const float kHalfScreenWidth = 10.0f; // 画面の中心から端までのワールド座標上の幅（調整可能）

	// ボス演出用変数
	bool isBossPerformance_ = false;            // ボス演出中フラグ
	bool isBossPerformanceFinished_ = false;    // ボス演出完了フラグ
	float bossEventTimer_ = 0.0f;               // 演出経過時間タイマー
	KamataEngine::Vector3 bossEventStartPos_{}; // 演出開始時のカメラ位置

	static inline const float kBossTriggerDistance = 12.0f; // ボス演出が発動する距離
	static inline const float kBossInTime = 1.0f;           // ボスへ移動・ズームインにかかる時間
	static inline const float kBossHoldTime = 1.5f;         // ボスを映し続ける時間
	static inline const float kBossOutTime = 1.0f;          // プレイヤーへ復帰・ズームアウトにかかる時間

	// ★追加：ゴール演出用変数
	bool isGoalPerformance_ = false;            // ゴール演出中フラグ
	bool isGoalPerformanceFinished_ = false;    // ゴール演出完了フラグ
	float goalEventTimer_ = 0.0f;               // ゴール演出タイマー
	KamataEngine::Vector3 goalEventStartPos_{}; // ゴール演出開始時のカメラ位置
	KamataEngine::Vector3 goalTargetPos_{};     // ゴールの目標カメラ位置

	static inline const float kGoalInTime = 1.0f;   // ゴールへ移動・ズームインにかかる時間
	static inline const float kGoalHoldTime = 1.5f; // ゴールを映し続ける時間
	static inline const float kGoalOutTime = 1.0f;  // プレイヤーへ復帰にかかる時間
};