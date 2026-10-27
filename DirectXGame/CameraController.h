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

	// ★ カメラ演出中（ボス演出・ゴール演出・処刑演出・シネマティック演出等）かどうかの判定
	bool IsCameraPerformance() const { return isBossPerformance_ || isGoalPerformance_ || isExecution_ || bossCinematic_ != BossCinematic::kNone; }

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

	// ★ 敵への打撃のカメラ演出（シェイクではなく、少し寄る・敵の方へ振れる・傾くことで「当たった」感じを出す）
	// hitPos: 打撃を受けた敵の位置、strength: 強さ（1.0 が通常の 1 発分。フィニッシュは大きめにする）
	void CombatHit(const KamataEngine::Vector3& hitPos, float strength = 1.0f);

private:
	// モードごとの更新処理
	void UpdateFollow();
	void UpdateForcedScroll();
	void UpdateFinalBossPerformance(); // ★ 追加: ラスボス用カメラ演出
	void UpdateBossPerformance();
	void UpdateGoalPerformance(); // ★追加：ゴール演出更新
	void UpdateExecution();       // ★ 処刑演出のカメラ更新

	// ★ ラスボスの攻撃・撃破に合わせたカメラ演出（シェイクではなく、ズーム・移動・傾きで見せる）。この関数でカメラを更新したら true
	bool UpdateFinalBossCinematic();

	// プレイヤーを画面内に押し戻す処理
	void ConstrainPlayerInScreen();

	// ★ 敵への打撃のカメラ演出を毎フレーム反映する
	void ApplyCombatHitEffect();

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
	bool isBossPerformance_ = false;              // ボス演出中フラグ
	bool isBossPerformanceFinished_ = false;      // 中ボス演出完了フラグ
	bool isFinalBossPerformanceFinished_ = false; // ★ 追加: ラスボス演出完了フラグ
	float bossEventTimer_ = 0.0f;                 // 演出経過時間タイマー
	KamataEngine::Vector3 bossEventStartPos_{};   // 演出開始時のカメラ位置

	static inline const float kBossTriggerDistance = 12.0f;      // ボス演出が発動する距離
	static inline const float kFinalBossTriggerDistance = 10.0f; // ★ ラスボス接近検知距離（X軸距離）
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

	// ★ ラスボスのカメラ演出列挙体に覚醒(kAwakening)を追加
	enum class BossCinematic {
		kNone,         // 演出なし
		kAttackIntro,  // 攻撃開始: ラスボスへ引いて空を見上げる
		kAttackOut,    // プレイヤー追従へ戻る
		kAwakening,    // ★ 追加: 覚醒演出（ラスボス全体を映し、ゆっくり寄っていく ➔ 咆哮で引く）
		kAwakeningOut, // ★ 追加: 覚醒演出が終わって、プレイヤー追従へなめらかに戻る
		kDefeat,       // 撃破: ゆっくり引いていく
	};

	BossCinematic bossCinematic_ = BossCinematic::kNone;
	float cinematicTimer_ = 0.0f;                   // 各演出の経過時間
	KamataEngine::Vector3 cinematicStartPos_{};     // 各演出の開始時のカメラ位置
	KamataEngine::Vector3 cinematicStartRot_{};     // 各演出の開始時のカメラの角度
	KamataEngine::Vector3 cinematicBaseRotation_{}; // 演出前のカメラの角度（傾きを元に戻すため）
	KamataEngine::Vector3 cinematicOffset_{};       // 追従カメラの目標位置に足すオフセット（攻撃中に少し引くため）
	bool isBossCinematicWide_ = false;              // ボスを映すために移動範囲の制限を外す演出中か
	bool wasBossAttacking_ = false;                 // 前のフレームでラスボスが攻撃中だったか（攻撃開始の検出用）
	float attackFollowBlend_ = 0.0f;                // 攻撃中の「少し引いて空を見上げる」追従の強さ（0.0 ~ 1.0）

	static inline const float kAttackIntroInTime = 0.9f;      // 攻撃開始: ラスボスへ引くのにかかる時間
	static inline const float kAttackIntroMinTime = 2.0f;     // 攻撃開始: 最低限ラスボスを映す時間
	static inline const float kAttackIntroMaxTime = 3.4f;     // 攻撃開始: ラスボスを映す最大時間（流星群が降り始める前に戻る）
	static inline const float kAttackOutTime = 1.0f;          // 攻撃開始: プレイヤーへ戻るのにかかる時間
	static inline const float kAttackCameraStartZ = -24.0f;   // 攻撃開始: ラスボスを映すカメラの Z 座標
	static inline const float kAwakeningInTime = 1.0f;        // ★ 覚醒演出: ラスボスへ移るのにかかる時間
	static inline const float kAwakeningOutTime = 1.3f;       // ★ 覚醒演出: プレイヤー追従へ戻るのにかかる時間
	static inline const float kAwakeningCameraFarZ = -24.0f;  // ★ 覚醒演出: 最初にラスボス全体を映すカメラの Z 座標
	static inline const float kAwakeningCameraNearZ = -19.5f; // ★ 覚醒演出: 胎動でゆっくり寄っていくカメラの Z 座標
	static inline const float kDefeatCameraInTime = 0.8f;     // ★ 修正: 撃破: ズームアウトにかかる時間（高速化）
	static inline const float kDefeatCameraFarZ = -15.0f;     // ★ 修正: 撃破開始時のカメラの Z 座標（通常位置）
	static inline const float kDefeatCameraNearZ = -35.0f;    // ★ 修正: ズームアウトしてカメラを遠くにする Z 座標（ラスボス全体）

	// ★ 敵への打撃のカメラ演出用変数
	float combatHitTimer_ = 0.0f;                      // 演出の残り時間
	float combatHitStrength_ = 1.0f;                   // 演出の強さ
	float combatHitTiltSign_ = 1.0f;                   // 傾ける向き（1 回ごとに左右交互）
	KamataEngine::Vector3 combatHitFocus_{};           // 打撃を受けた敵の位置
	KamataEngine::Vector3 combatHitAppliedOffset_{};   // 前フレームに足した位置のオフセット（次のフレームの先頭で元に戻す）
	KamataEngine::Vector3 combatHitAppliedRotation_{}; // 前フレームに足した角度のオフセット

	static inline const float kCombatHitDuration = 0.35f;    // 1 回分の演出の長さ（秒）
	static inline const float kCombatHitZoomDistance = 1.0f; // 打撃の瞬間に寄る距離
	static inline const float kCombatHitPanRate = 0.10f;     // 打撃を受けた敵の方へカメラが振れる割合
	static inline const float kCombatHitTiltRoll = 0.05f;    // カメラの傾き（斜め：Z 軸まわり）
	static inline const float kCombatHitTiltPitch = 0.025f;  // カメラの傾き（X 軸まわり）

	// ★追加：ラスボス発狂シェイク減衰用変数
	float frenzyShakeTimer_ = 0.0f;
	static inline const float kFrenzyShakeDuration = 2.5f;     // 振動が完全停止するまでの時間（秒）
	static inline const float kFrenzyMaxShakeIntensity = 0.4f; // シェイク開始時の最大振動幅
};
