#pragma once
#include "Kamataengine.h"

class Player;
class BossEnemy;
class FinalBoss;

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

	// ★ 追加
	void SetFinalBoss(FinalBoss* finalBoss) { finalBoss_ = finalBoss; }

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

	// ★ 処刑演出（中ボス撃破後の連撃）用カメラ
	// ズームイン開始（focusPos: 注目する位置 = 中ボス）
	void StartExecution(const KamataEngine::Vector3& focusPos);
	// 注目する位置の更新（打ち上げられた中ボスを追うときなどに毎フレーム呼ぶ）
	void SetExecutionFocus(const KamataEngine::Vector3& focusPos) { executionFocus_ = focusPos; }
	// 注目点からカメラまでの距離の目標（小さいほどズームイン）
	void SetExecutionZoom(float distance) { executionZoomTarget_ = distance; }
	// 連撃 1 回ごとの演出：少しズームイン + シェイク + カメラの傾き（strength: 強さ。1.0 が通常の 1 発分）
	void ExecutionHit(float strength);
	// プレイヤー追従へ復帰（なめらかに戻る）
	void EndExecution();
	bool IsExecution() const { return isExecution_; }

private:
	// モードごとの更新処理
	void UpdateFollow();
	void UpdateForcedScroll();
	void UpdateFinalBossPerformance(); // ★ 追加: ラスボス用カメラ演出
	void UpdateBossPerformance();
	void UpdateGoalPerformance(); // ★追加：ゴール演出更新
	void UpdateExecution();       // ★ 処刑演出のカメラ更新

	// プレイヤーを画面内に押し戻す処理
	void ConstrainPlayerInScreen();

private:
	KamataEngine::Camera* camera_ = nullptr;
	Player* target_ = nullptr;
	BossEnemy* boss_ = nullptr;
	FinalBoss* finalBoss_ = nullptr; // ★ 追加

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

	static inline const float kBossTriggerDistance = 12.0f;      // ボス演出が発動する距離
	static inline const float kFinalBossTriggerDistance = 10.0f; // ★ ラスボス接近検知距離
	static inline const float kBossInTime = 1.0f;                // ボスへ移動・ズームインにかかる時間
	static inline const float kBossHoldTime = 2.5f;              // ボスを映し続ける時間
	static inline const float kBossOutTime = 1.0f;               // プレイヤーへ復帰・ズームアウトにかかる時間

	// ★追加：ゴール演出用変数
	bool isGoalPerformance_ = false;            // ゴール演出中フラグ
	bool isGoalPerformanceFinished_ = false;    // ゴール演出完了フラグ
	float goalEventTimer_ = 0.0f;               // ゴール演出タイマー
	KamataEngine::Vector3 goalEventStartPos_{}; // ゴール演出開始時のカメラ位置
	KamataEngine::Vector3 goalTargetPos_{};     // ゴールの目標カメラ位置

	static inline const float kGoalInTime = 1.0f;   // ゴールへ移動・ズームインにかかる時間
	static inline const float kGoalHoldTime = 1.5f; // ゴールを映し続ける時間
	static inline const float kGoalOutTime = 1.0f;  // プレイヤーへ復帰にかかる時間

	// ★ 処刑演出用変数
	bool isExecution_ = false;                          // 処刑演出中フラグ（復帰中も true）
	bool isExecutionEnding_ = false;                    // プレイヤーへ復帰中
	float executionTimer_ = 0.0f;                       // 開始 / 復帰の経過時間
	KamataEngine::Vector3 executionFocus_{};            // 注目する位置
	KamataEngine::Vector3 executionStartPos_{};         // 開始時のカメラ位置
	KamataEngine::Vector3 executionEndStartPos_{};      // 復帰開始時のカメラ位置
	KamataEngine::Vector3 executionOriginalRotation_{}; // 演出前のカメラの角度（傾きを元に戻すため）
	float executionZoomTarget_ = 9.0f;                  // 注目点からカメラまでの距離
	float executionHitTimer_ = 0.0f;                    // 連撃 1 回分の演出の残り時間
	float executionHitStrength_ = 1.0f;                 // 連撃 1 回分の演出の強さ
	float executionTiltSign_ = 1.0f;                    // カメラを傾ける向き（1 回ごとに左右交互）

	static inline const float kExecutionZoomInTime = 0.9f;    // ズームインにかかる時間
	static inline const float kExecutionReturnTime = 1.0f;    // プレイヤーへ復帰する時間
	static inline const float kExecutionHitDuration = 0.4f;   // 連撃 1 回分の演出の長さ
	static inline const float kExecutionMaxShake = 0.35f;     // シェイクの最大の振れ幅
	static inline const float kExecutionTiltRoll = 0.06f;     // カメラの傾き（斜め：Z 軸まわり）
	static inline const float kExecutionTiltPitch = 0.035f;   // カメラの傾き（X 軸まわり）
	static inline const float kExecutionPunchDistance = 1.2f; // 1 発ごとに少しだけ寄る距離

	// ★追加：ラスボス発狂シェイク減衰用変数
	float frenzyShakeTimer_ = 0.0f;
	static inline const float kFrenzyShakeDuration = 2.5f;     // 振動が完全停止するまでの時間（秒）
	static inline const float kFrenzyMaxShakeIntensity = 0.4f; // シェイク開始時の最大振動幅
};