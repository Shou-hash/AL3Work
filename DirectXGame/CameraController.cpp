#define NOMINMAX
#include "CameraController.h"
#include "BossEnemy.h"
#include "FinalBoss.h"
#include "Matrix4x4.h"
#include "Player.h"
#include <algorithm>
#include <cmath>
#include <cstdlib>

float Lerp(float current, float target, float rate) { return current + rate * (target - current); }

// ズーム演出用のイージング関数 (EaseInOutCubic)
float EaseInOutCubic(float t) { return t < 0.5f ? 4.0f * t * t * t : 1.0f - std::pow(-2.0f * t + 2.0f, 3.0f) / 2.0f; }

void CameraController::Initialize(KamataEngine::Camera* camera) {
	// 引数で受け取ったカメラのポインタをメンバ変数に保存する
	camera_ = camera;
	mode_ = CameraMode::kFollow; // 初期モードを追従に設定
	boss_ = nullptr;
	isBossPerformance_ = false;
	isBossPerformanceFinished_ = false;
	bossEventTimer_ = 0.0f;

	isGoalPerformance_ = false;
	isGoalPerformanceFinished_ = false;
	goalEventTimer_ = 0.0f;
	frenzyShakeTimer_ = 0.0f;

	isExecution_ = false;
	isExecutionEnding_ = false;
	executionHitTimer_ = 0.0f;
}

void CameraController::Reset() {
	if (!target_ || !camera_) {
		return;
	}

	// プレイヤーの位置を取得
	const KamataEngine::WorldTransform& targetWorldTransform = target_->GetWorldTransform();

	// 開始時は Lerp を使わず、ダイレクトに目標位置（クランプ済み）にカメラを配置する
	float startX = targetWorldTransform.translation_.x + targetOffset_.x;
	float startY = targetWorldTransform.translation_.y + targetOffset_.y;
	float startZ = targetWorldTransform.translation_.z + targetOffset_.z;

	camera_->translation_.x = std::clamp(startX, movableArea_.left, movableArea_.right);
	camera_->translation_.y = std::clamp(startY, movableArea_.bottom, movableArea_.top);
	camera_->translation_.z = startZ;

	isBossPerformance_ = false;
	isBossPerformanceFinished_ = false;
	bossEventTimer_ = 0.0f;

	isGoalPerformance_ = false;
	isGoalPerformanceFinished_ = false;
	goalEventTimer_ = 0.0f;

	frenzyShakeTimer_ = 0.0f;

	isExecution_ = false;
	isExecutionEnding_ = false;
	executionHitTimer_ = 0.0f;

	// ★ ラスボス演出のリセット
	bossCinematic_ = BossCinematic::kNone;
	cinematicTimer_ = 0.0f;
	cinematicOffset_ = {0.0f, 0.0f, 0.0f};
	isBossCinematicWide_ = false;
	wasBossAttacking_ = false;
	attackFollowBlend_ = 0.0f;
	cinematicBaseRotation_ = camera_->rotation_;

	// ★ 敵への打撃演出のリセット
	combatHitTimer_ = 0.0f;
	combatHitAppliedOffset_ = {0.0f, 0.0f, 0.0f};
	combatHitAppliedRotation_ = {0.0f, 0.0f, 0.0f};
}

void CameraController::StartGoalPerformance(const KamataEngine::Vector3& goalPos) {
	isGoalPerformance_ = true;
	isGoalPerformanceFinished_ = false;
	goalEventTimer_ = 0.0f;
	if (camera_) {
		goalEventStartPos_ = camera_->translation_;
	}
	goalTargetPos_ = goalPos;
	goalTargetPos_.z -= 7.5f; // 通常(-15.0f)より近づけてズームイン
}

void CameraController::Update() {
	if (!target_ || !camera_) {
		return;
	}

	// ★ 前フレームに足した敵への打撃演出のオフセットを元に戻す（追従の補間に影響させないため）
	camera_->translation_.x -= combatHitAppliedOffset_.x;
	camera_->translation_.y -= combatHitAppliedOffset_.y;
	camera_->translation_.z -= combatHitAppliedOffset_.z;
	camera_->rotation_.x -= combatHitAppliedRotation_.x;
	camera_->rotation_.z -= combatHitAppliedRotation_.z;
	combatHitAppliedOffset_ = {0.0f, 0.0f, 0.0f};
	combatHitAppliedRotation_ = {0.0f, 0.0f, 0.0f};

	// ★ 処刑演出中は、専用のカメラ更新だけを行う
	if (isExecution_) {
		UpdateExecution();
		return;
	}

	// 中ボスへの接近判定と演出の開始
	if (boss_) {
		if (boss_->IsDead()) {
			boss_ = nullptr; // ボス死亡時にポインタをクリア
		} else if (!isBossPerformance_ && !isBossPerformanceFinished_) {
			const KamataEngine::WorldTransform& playerTransform = target_->GetWorldTransform();
			const KamataEngine::WorldTransform& bossTransform = boss_->GetWorldTransform();

			float dx = bossTransform.translation_.x - playerTransform.translation_.x;
			float dy = bossTransform.translation_.y - playerTransform.translation_.y;
			float dist = std::sqrt(dx * dx + dy * dy);

			if (dist <= kBossTriggerDistance) {
				isBossPerformance_ = true;
				bossEventTimer_ = 0.0f;
				bossEventStartPos_ = camera_->translation_;
			}
		}
	}

	// ラスボス接近判定
	if (finalBoss_) {
		if (finalBoss_->IsDead()) {
			finalBoss_ = nullptr;
		} else if (!isBossPerformance_ && !isBossPerformanceFinished_) {
			const KamataEngine::WorldTransform& playerTransform = target_->GetWorldTransform();
			const KamataEngine::Vector3& bossPos = finalBoss_->GetBasePosition();

			float dx = bossPos.x - playerTransform.translation_.x;
			float dy = bossPos.y - playerTransform.translation_.y;
			if (std::sqrt(dx * dx + dy * dy) <= kFinalBossTriggerDistance) {
				isBossPerformance_ = true;
				bossEventTimer_ = 0.0f;
				bossEventStartPos_ = camera_->translation_;

				// ★ プレイヤーが接近した瞬間にラスボスの出現アニメーションを開始！
				finalBoss_->StartSpawn();
			}
		}
	}

	// ★ ラスボスの攻撃・撃破に合わせたカメラ演出（ゴール演出・出現演出が優先）
	bool isBossCinematicDriven = false;
	if (!isGoalPerformance_ && !isBossPerformance_) {
		isBossCinematicDriven = UpdateFinalBossCinematic();
	}

	// モードに応じてカメラの座標を更新
	if (isGoalPerformance_) {
		UpdateGoalPerformance();
	} else if (isBossPerformance_) {
		if (finalBoss_) {
			UpdateFinalBossPerformance();
		} else {
			UpdateBossPerformance();
		}
	} else if (isBossCinematicDriven) {
		// ラスボスの攻撃・撃破の演出で更新済み
	} else if (mode_ == CameraMode::kFollow) {
		UpdateFollow();
	} else if (mode_ == CameraMode::kForcedScroll) {
		UpdateForcedScroll();
	}

	// 移動範囲（movableArea_）に収まるように制限（演出中は目標地点まで移動できるよう制限を解除）
	if (!isBossPerformance_ && !isGoalPerformance_ && !isBossCinematicWide_) {
		camera_->translation_.x = std::clamp(camera_->translation_.x, movableArea_.left, movableArea_.right);
		camera_->translation_.y = std::clamp(camera_->translation_.y, movableArea_.bottom, movableArea_.top);
	}

	// 画面内への押し出し制限処理（強制スクロール時などに機能）
	ConstrainPlayerInScreen();

	// =========================================================
	// ★ 追加: ラスボスの発狂演出に伴うカメラシェイク（画面振動）処理
	// =========================================================
	if (finalBoss_) {
		FinalBoss::State bossState = finalBoss_->GetState();

		// 発狂の溜め(kFrenzyInit) または 発狂本番(kFrenzy) の時
		if (bossState == FinalBoss::State::kFrenzyInit || bossState == FinalBoss::State::kFrenzy) {
			// タイマーをカウントアップ (60fps相当)
			frenzyShakeTimer_ += 1.0f / 60.0f;

			// 時間経過割合 (0.0f ～ 1.0f)
			float progress = std::clamp(frenzyShakeTimer_ / kFrenzyShakeDuration, 0.0f, 1.0f);

			// 1.0f から 0.0f へ向かって二次曲線で滑らかに減衰 (Ease-Out)
			float decay = (1.0f - progress) * (1.0f - progress);
			float shakeIntensity = kFrenzyMaxShakeIntensity * decay;

			// 振動幅が残っている間のみ画面を揺らす（最後は完全停止）
			if (shakeIntensity > 0.001f) {
				float shakeX = ((static_cast<float>(rand()) / RAND_MAX) * 2.0f - 1.0f) * shakeIntensity;
				float shakeY = ((static_cast<float>(rand()) / RAND_MAX) * 2.0f - 1.0f) * shakeIntensity;

				camera_->translation_.x += shakeX;
				camera_->translation_.y += shakeY;
			}
		} else {
			// 発狂状態が終わったらタイマーをリセット
			frenzyShakeTimer_ = 0.0f;
		}
	}

	// ★ 敵への打撃のカメラ演出を反映
	ApplyCombatHitEffect();
}

// ★ 追加: ラスボス全体を見せるカメラ演出処理
void CameraController::UpdateFinalBossPerformance() {
	if (!finalBoss_ || !target_) {
		isBossPerformance_ = false;
		return;
	}

	bossEventTimer_ += 1.0f / 60.0f;

	const KamataEngine::WorldTransform& playerWorldTransform = target_->GetWorldTransform();
	const KamataEngine::Vector3& bossBasePos = finalBoss_->GetBasePosition();

	// プレイヤー復帰位置
	KamataEngine::Vector3 playerTargetPos;
	playerTargetPos.x = playerWorldTransform.translation_.x + targetOffset_.x;
	playerTargetPos.y = playerWorldTransform.translation_.y + targetOffset_.y;
	playerTargetPos.z = playerWorldTransform.translation_.z + targetOffset_.z;

	// ラスボス全体が収まるカメラ位置（Z軸を通常-15から-22等へ大きく引いて広角表示）
	KamataEngine::Vector3 bossTargetPos;
	bossTargetPos.x = bossBasePos.x; // ラスボスの中心X座標
	bossTargetPos.y = bossBasePos.y; // ラスボスの中心Y座標
	bossTargetPos.z = -22.0f;        // 欠片や手全体が画面に収まるようZ軸を引きに設定

	float totalTime = kBossInTime + kBossHoldTime + kBossOutTime;

	if (bossEventTimer_ <= kBossInTime) {
		// 1. ボス全貌位置へ移動＆引き（ズームアウト）
		float t = std::clamp(bossEventTimer_ / kBossInTime, 0.0f, 1.0f);
		float easeVal = EaseInOutCubic(t);

		camera_->translation_.x = Lerp(bossEventStartPos_.x, bossTargetPos.x, easeVal);
		camera_->translation_.y = Lerp(bossEventStartPos_.y, bossTargetPos.y, easeVal);
		camera_->translation_.z = Lerp(bossEventStartPos_.z, bossTargetPos.z, easeVal);

		// 移動完了タイミングでボスの発狂初期アニメーションを開始
		if (t >= 0.95f) {
			finalBoss_->StartFrenzy();
		}

	} else if (bossEventTimer_ <= kBossInTime + kBossHoldTime) {
		// 2. 全姿と発狂演出を画面内に収めてキープ
		camera_->translation_ = bossTargetPos;

	} else if (bossEventTimer_ <= totalTime) {
		// 3. プレイヤー追従位置へ復帰
		float t = std::clamp((bossEventTimer_ - kBossInTime - kBossHoldTime) / kBossOutTime, 0.0f, 1.0f);
		float easeVal = EaseInOutCubic(t);

		camera_->translation_.x = Lerp(bossTargetPos.x, playerTargetPos.x, easeVal);
		camera_->translation_.y = Lerp(bossTargetPos.y, playerTargetPos.y, easeVal);
		camera_->translation_.z = Lerp(bossTargetPos.z, playerTargetPos.z, easeVal);

	} else {
		isBossPerformance_ = false;
		isBossPerformanceFinished_ = true;
	}
}

void CameraController::UpdateFollow() {
	const KamataEngine::WorldTransform& targetWorldTransform = target_->GetWorldTransform();
	const KamataEngine::Vector3& targetVelocity = target_->GetVelocity();

	// カメラの目標位置（プレイヤーの位置 ＋ オフセット）を計算
	targetPosition_.x = targetWorldTransform.translation_.x + targetOffset_.x + (targetVelocity.x * kVelocityBias);
	targetPosition_.y = targetWorldTransform.translation_.y + targetOffset_.y + (targetVelocity.y * kVelocityBias);
	targetPosition_.z = targetWorldTransform.translation_.z + targetOffset_.z + (targetVelocity.z * kVelocityBias);

	// ★ ラスボスの攻撃中は少し引いて空を見上げるため、オフセットを足す（通常は 0）
	targetPosition_.x += cinematicOffset_.x;
	targetPosition_.y += cinematicOffset_.y;
	targetPosition_.z += cinematicOffset_.z;

	camera_->translation_.x = Lerp(camera_->translation_.x, targetPosition_.x, kInterpolationRate);
	camera_->translation_.y = Lerp(camera_->translation_.y, targetPosition_.y, kInterpolationRate);
	camera_->translation_.z = Lerp(camera_->translation_.z, targetPosition_.z, kInterpolationRate);
}

void CameraController::UpdateForcedScroll() {
	// 強制スクロール：毎フレーム一定速度でX座標を加算
	camera_->translation_.x += kScrollSpeed;

	// Y軸とZ軸はプレイヤーに緩やかに追従、または固定（ここではプレイヤーのYに追従させる例）
	const KamataEngine::WorldTransform& targetWorldTransform = target_->GetWorldTransform();
	float targetY = targetWorldTransform.translation_.y + targetOffset_.y;
	camera_->translation_.y = Lerp(camera_->translation_.y, targetY, kInterpolationRate);
	camera_->translation_.z = targetWorldTransform.translation_.z + targetOffset_.z;
}

void CameraController::UpdateBossPerformance() {
	if (!boss_ || !target_) {
		isBossPerformance_ = false;
		return;
	}

	bossEventTimer_ += 1.0f / 60.0f;

	const KamataEngine::WorldTransform& playerWorldTransform = target_->GetWorldTransform();
	const KamataEngine::WorldTransform& bossWorldTransform = boss_->GetWorldTransform();

	// 復帰時の目標位置（プレイヤー追従位置）
	KamataEngine::Vector3 playerTargetPos;
	playerTargetPos.x = playerWorldTransform.translation_.x + targetOffset_.x;
	playerTargetPos.y = playerWorldTransform.translation_.y + targetOffset_.y;
	playerTargetPos.z = playerWorldTransform.translation_.z + targetOffset_.z;

	// ボス中心の目標位置（Z軸を近づけてズームイン）
	KamataEngine::Vector3 bossTargetPos;
	bossTargetPos.x = bossWorldTransform.translation_.x;
	bossTargetPos.y = bossWorldTransform.translation_.y + 1.0f;
	bossTargetPos.z = bossWorldTransform.translation_.z - 7.5f; // 通常(-15.0f)より近づけてズーム

	float totalTime = kBossInTime + kBossHoldTime + kBossOutTime;

	if (bossEventTimer_ <= kBossInTime) {
		// 1. イージングでボス中心へ移動＆ズームイン
		float t = bossEventTimer_ / kBossInTime;
		t = std::clamp(t, 0.0f, 1.0f);
		float easeVal = EaseInOutCubic(t);

		camera_->translation_.x = Lerp(bossEventStartPos_.x, bossTargetPos.x, easeVal);
		camera_->translation_.y = Lerp(bossEventStartPos_.y, bossTargetPos.y, easeVal);
		camera_->translation_.z = Lerp(bossEventStartPos_.z, bossTargetPos.z, easeVal);

	} else if (bossEventTimer_ <= kBossInTime + kBossHoldTime) {
		// 2. ボスを中心に画面を維持
		camera_->translation_ = bossTargetPos;

	} else if (bossEventTimer_ <= totalTime) {
		// 3. イージングでプレイヤー位置へ復帰＆ズームアウト
		float t = (bossEventTimer_ - kBossInTime - kBossHoldTime) / kBossOutTime;
		t = std::clamp(t, 0.0f, 1.0f);
		float easeVal = EaseInOutCubic(t);

		camera_->translation_.x = Lerp(bossTargetPos.x, playerTargetPos.x, easeVal);
		camera_->translation_.y = Lerp(bossTargetPos.y, playerTargetPos.y, easeVal);
		camera_->translation_.z = Lerp(bossTargetPos.z, playerTargetPos.z, easeVal);

	} else {
		// 演出終了
		isBossPerformance_ = false;
		isBossPerformanceFinished_ = true;
	}
}

void CameraController::UpdateGoalPerformance() {
	if (!target_ || !camera_) {
		isGoalPerformance_ = false;
		return;
	}

	goalEventTimer_ += 1.0f / 60.0f;

	const KamataEngine::WorldTransform& playerWorldTransform = target_->GetWorldTransform();

	// 復帰時の目標位置（プレイヤー追従位置）
	KamataEngine::Vector3 playerTargetPos;
	playerTargetPos.x = playerWorldTransform.translation_.x + targetOffset_.x;
	playerTargetPos.y = playerWorldTransform.translation_.y + targetOffset_.y;
	playerTargetPos.z = playerWorldTransform.translation_.z + targetOffset_.z;

	float totalTime = kGoalInTime + kGoalHoldTime + kGoalOutTime;

	if (goalEventTimer_ <= kGoalInTime) {
		// 1. イージングでゴールへ移動＆ズームイン
		float t = goalEventTimer_ / kGoalInTime;
		t = std::clamp(t, 0.0f, 1.0f);
		float easeVal = EaseInOutCubic(t);

		camera_->translation_.x = Lerp(goalEventStartPos_.x, goalTargetPos_.x, easeVal);
		camera_->translation_.y = Lerp(goalEventStartPos_.y, goalTargetPos_.y, easeVal);
		camera_->translation_.z = Lerp(goalEventStartPos_.z, goalTargetPos_.z, easeVal);

	} else if (goalEventTimer_ <= kGoalInTime + kGoalHoldTime) {
		// 2. ゴールを中心に画面を維持
		camera_->translation_ = goalTargetPos_;

	} else if (goalEventTimer_ <= totalTime) {
		// 3. イージングでプレイヤー位置へ復帰＆ズームアウト
		float t = (goalEventTimer_ - kGoalInTime - kGoalHoldTime) / kGoalOutTime;
		t = std::clamp(t, 0.0f, 1.0f);
		float easeVal = EaseInOutCubic(t);

		camera_->translation_.x = Lerp(goalTargetPos_.x, playerTargetPos.x, easeVal);
		camera_->translation_.y = Lerp(goalTargetPos_.y, playerTargetPos.y, easeVal);
		camera_->translation_.z = Lerp(goalTargetPos_.z, playerTargetPos.z, easeVal);

	} else {
		// 演出終了
		isGoalPerformance_ = false;
		isGoalPerformanceFinished_ = true;
	}
}

void CameraController::ConstrainPlayerInScreen() {
	// プレイヤーが死亡している場合は処理しない
	if (target_->IsDead()) {
		return;
	}
}

float CameraController::GetCameraLeftX() const {
	if (!camera_) {
		return 0.0f;
	}
	// カメラ演出中は押し出し判定をスキップするため、十分小さな値を返す
	if (isBossPerformance_ || isGoalPerformance_ || isExecution_) {
		return -9999.0f;
	}
	return camera_->translation_.x - kHalfScreenWidth;
}

// ★ 処刑演出の開始：中ボスへズームインする
void CameraController::StartExecution(const KamataEngine::Vector3& focusPos) {
	if (!camera_) {
		return;
	}
	isExecution_ = true;
	isExecutionEnding_ = false;
	executionTimer_ = 0.0f;
	executionFocus_ = focusPos;
	executionStartPos_ = camera_->translation_;
	executionOriginalRotation_ = camera_->rotation_;
	executionZoomTarget_ = 9.0f;
	executionHitTimer_ = 0.0f;
	executionTiltSign_ = 1.0f;
}

// ★ 連撃 1 回ごとの演出：少しズームイン + シェイク + カメラの傾き
void CameraController::ExecutionHit(float strength) {
	executionHitTimer_ = kExecutionHitDuration;
	executionHitStrength_ = strength;
	executionTiltSign_ = -executionTiltSign_; // 1 回ごとに傾ける向きを左右交互にする
}

// ★ プレイヤー追従へ復帰する
void CameraController::EndExecution() {
	if (!isExecution_ || !camera_) {
		return;
	}
	isExecutionEnding_ = true;
	executionTimer_ = 0.0f;
	executionEndStartPos_ = camera_->translation_;
}

// ★ 処刑演出のカメラ更新
void CameraController::UpdateExecution() {
	const float dt = 1.0f / 60.0f;

	// ---------------------------------------------------------
	// プレイヤー追従位置へ復帰（傾きもなめらかに元に戻す）
	// ---------------------------------------------------------
	if (isExecutionEnding_) {
		executionTimer_ += dt;
		float t = std::clamp(executionTimer_ / kExecutionReturnTime, 0.0f, 1.0f);
		float easeVal = EaseInOutCubic(t);

		const KamataEngine::WorldTransform& playerTransform = target_->GetWorldTransform();
		KamataEngine::Vector3 playerTargetPos;
		playerTargetPos.x = playerTransform.translation_.x + targetOffset_.x;
		playerTargetPos.y = playerTransform.translation_.y + targetOffset_.y;
		playerTargetPos.z = playerTransform.translation_.z + targetOffset_.z;

		camera_->translation_.x = Lerp(executionEndStartPos_.x, playerTargetPos.x, easeVal);
		camera_->translation_.y = Lerp(executionEndStartPos_.y, playerTargetPos.y, easeVal);
		camera_->translation_.z = Lerp(executionEndStartPos_.z, playerTargetPos.z, easeVal);

		camera_->rotation_.x = Lerp(camera_->rotation_.x, executionOriginalRotation_.x, 0.2f);
		camera_->rotation_.z = Lerp(camera_->rotation_.z, executionOriginalRotation_.z, 0.2f);

		if (t >= 1.0f) {
			camera_->rotation_ = executionOriginalRotation_;
			isExecution_ = false;
			isExecutionEnding_ = false;
		}
		return;
	}

	// ---------------------------------------------------------
	// 連撃 1 回分の演出（時間とともに二次曲線で減衰する）
	// ---------------------------------------------------------
	float decay = 0.0f;
	if (executionHitTimer_ > 0.0f) {
		executionHitTimer_ -= dt;
		if (executionHitTimer_ < 0.0f) {
			executionHitTimer_ = 0.0f;
		}
		float k = executionHitTimer_ / kExecutionHitDuration;
		decay = k * k;
	}
	const float power = executionHitStrength_ * decay;

	// ---------------------------------------------------------
	// ズームイン（開始直後はイージング、その後は注目点へ追従）
	// ---------------------------------------------------------
	float distance = executionZoomTarget_ - kExecutionPunchDistance * power; // 1 発ごとに少し寄る
	KamataEngine::Vector3 desired;
	desired.x = executionFocus_.x;
	desired.y = executionFocus_.y + 0.8f;
	desired.z = executionFocus_.z - distance;

	if (executionTimer_ < kExecutionZoomInTime) {
		executionTimer_ += dt;
		float t = std::clamp(executionTimer_ / kExecutionZoomInTime, 0.0f, 1.0f);
		float easeVal = EaseInOutCubic(t);
		camera_->translation_.x = Lerp(executionStartPos_.x, desired.x, easeVal);
		camera_->translation_.y = Lerp(executionStartPos_.y, desired.y, easeVal);
		camera_->translation_.z = Lerp(executionStartPos_.z, desired.z, easeVal);
	} else {
		camera_->translation_.x = Lerp(camera_->translation_.x, desired.x, 0.2f);
		camera_->translation_.y = Lerp(camera_->translation_.y, desired.y, 0.2f);
		camera_->translation_.z = Lerp(camera_->translation_.z, desired.z, 0.2f);
	}

	// シェイク
	if (power > 0.001f) {
		float shakeX = ((static_cast<float>(rand()) / RAND_MAX) * 2.0f - 1.0f) * kExecutionMaxShake * power;
		float shakeY = ((static_cast<float>(rand()) / RAND_MAX) * 2.0f - 1.0f) * kExecutionMaxShake * power;
		camera_->translation_.x += shakeX;
		camera_->translation_.y += shakeY;
	}

	// カメラの傾き（斜めになり、時間とともに戻る）
	camera_->rotation_.z = executionOriginalRotation_.z + kExecutionTiltRoll * power * executionTiltSign_;
	camera_->rotation_.x = executionOriginalRotation_.x + kExecutionTiltPitch * power * executionTiltSign_;
}

// ★ ラスボスの攻撃・撃破に合わせたカメラ演出（シェイクではなく、ズーム・移動・傾きで見せる）
//   戻り値: この関数でカメラを更新した場合は true
bool CameraController::UpdateFinalBossCinematic() {
	if (!finalBoss_ || !target_ || !camera_) {
		return false;
	}

	const float dt = 1.0f / 60.0f;
	const float pi = 3.14159265f;
	const FinalBoss::State bossState = finalBoss_->GetState();
	const bool isAttacking = (bossState == FinalBoss::State::kAttack);
	const KamataEngine::Vector3& bossBasePos = finalBoss_->GetBasePosition();

	isBossCinematicWide_ = false;

	// プレイヤー追従位置（復帰先）
	const KamataEngine::WorldTransform& playerTransform = target_->GetWorldTransform();
	KamataEngine::Vector3 playerTargetPos;
	playerTargetPos.x = playerTransform.translation_.x + targetOffset_.x;
	playerTargetPos.y = playerTransform.translation_.y + targetOffset_.y;
	playerTargetPos.z = playerTransform.translation_.z + targetOffset_.z;

	// =========================================================
	// ① 撃破演出: 崩れ落ちるラスボスからカメラを遠くにし（ズームアウト）、全体を映す（終わったらゴール追従カメラ演出が始まる）
	// =========================================================
	if (finalBoss_->IsDefeated() && !finalBoss_->IsDefeatAnimationFinished()) {
		if (bossCinematic_ != BossCinematic::kDefeat) {
			bossCinematic_ = BossCinematic::kDefeat;
			cinematicTimer_ = 0.0f;
			cinematicStartPos_ = camera_->translation_;
		}
		cinematicTimer_ += dt;
		isBossCinematicWide_ = true;

		// ★ ズームアウト完了時にラスボスへ通知し、撃破演出（崩れ落ちアニメーション）を開始させる
		if (cinematicTimer_ >= kDefeatCameraInTime) {
			finalBoss_->SetDefeatZoomFinished(true);
		}

		const float progress = finalBoss_->GetDefeatProgress();
		const float blendIn = EaseInOutCubic(std::clamp(cinematicTimer_ / kDefeatCameraInTime, 0.0f, 1.0f));
		const float easeProgress = EaseInOutCubic(progress);

		// 注目位置: ラスボス全体 ➔ 崩れ落ちるにつれて少し下へ。カメラを遠く（ズームアウト）にして全貌を映す
		KamataEngine::Vector3 desired;
		desired.x = bossBasePos.x + std::sin(progress * pi) * 1.5f;
		desired.y = bossBasePos.y + 1.0f - 1.5f * easeProgress;
		desired.z = kDefeatCameraFarZ + (kDefeatCameraNearZ - kDefeatCameraFarZ) * easeProgress;

		camera_->translation_.x = Lerp(cinematicStartPos_.x, desired.x, blendIn);
		camera_->translation_.y = Lerp(cinematicStartPos_.y, desired.y, blendIn);
		camera_->translation_.z = Lerp(cinematicStartPos_.z, desired.z, blendIn);

		// 傾き: 崩れ落ちる向きに合わせて斜めになり、演出の終わりに向けて元に戻る
		const float tilt = std::sin(progress * pi);
		camera_->rotation_.z = cinematicBaseRotation_.z + 0.10f * tilt;
		camera_->rotation_.x = cinematicBaseRotation_.x + 0.04f * tilt;
		return true;
	}

	if (bossCinematic_ == BossCinematic::kDefeat) {
		// 撃破演出が終わったら傾きを元に戻す
		bossCinematic_ = BossCinematic::kNone;
		camera_->rotation_ = cinematicBaseRotation_;
	}

	// =========================================================
	// ★ ダウン（伏せ）演出: ステージ手前に伏せたラスボスとプレイヤーをカメラで映す
	// （※攻撃処理の前に判定を行うように上部へ移動しました）
	// =========================================================
	if (bossState == FinalBoss::State::kDown) {
		isBossCinematicWide_ = true;
		KamataEngine::Vector3 desired;
		desired.x = (playerTransform.translation_.x + bossBasePos.x) * 0.5f;
		desired.y = playerTransform.translation_.y + 1.0f;
		desired.z = -14.0f; // ★ 少しズームインして追撃しやすい構図にする

		camera_->translation_.x = Lerp(camera_->translation_.x, desired.x, 0.1f);
		camera_->translation_.y = Lerp(camera_->translation_.y, desired.y, 0.1f);
		camera_->translation_.z = Lerp(camera_->translation_.z, desired.z, 0.1f);

		camera_->rotation_.x = cinematicBaseRotation_.x - 0.03f;
		camera_->rotation_.z = cinematicBaseRotation_.z;
		return true;
	}

	// =========================================================
	// ② 攻撃開始の検出
	// =========================================================
	if (isAttacking && !wasBossAttacking_) {
		bossCinematic_ = BossCinematic::kAttackIntro;
		cinematicTimer_ = 0.0f;
		cinematicStartPos_ = camera_->translation_;
		cinematicStartRot_ = camera_->rotation_;
		attackFollowBlend_ = 0.0f;
	}
	wasBossAttacking_ = isAttacking;

	// =========================================================
	// ③ 攻撃開始演出: ラスボスへ引き、空へ飛び立つクリスタルを見上げながらゆっくり寄る
	// =========================================================
	if (bossCinematic_ == BossCinematic::kAttackIntro) {
		cinematicTimer_ += dt;
		isBossCinematicWide_ = true;

		const bool finished = !isAttacking || finalBoss_->IsMeteorShowerStarted() || cinematicTimer_ >= kAttackIntroMaxTime;
		if (finished && cinematicTimer_ >= kAttackIntroMinTime) {
			// プレイヤーへ戻る演出へ
			bossCinematic_ = BossCinematic::kAttackOut;
			cinematicTimer_ = 0.0f;
			cinematicStartPos_ = camera_->translation_;
			cinematicStartRot_ = camera_->rotation_;
		} else {
			const float blendIn = EaseInOutCubic(std::clamp(cinematicTimer_ / kAttackIntroInTime, 0.0f, 1.0f));
			const float hold = std::max(0.0f, cinematicTimer_ - kAttackIntroInTime);

			KamataEngine::Vector3 desired;
			desired.x = bossBasePos.x;
			desired.y = bossBasePos.y + 2.0f + 0.5f * hold; // 飛び立つクリスタルを追って少し上へ
			desired.z = kAttackCameraStartZ + 0.8f * hold;  // ゆっくり寄る

			camera_->translation_.x = Lerp(cinematicStartPos_.x, desired.x, blendIn);
			camera_->translation_.y = Lerp(cinematicStartPos_.y, desired.y, blendIn);
			camera_->translation_.z = Lerp(cinematicStartPos_.z, desired.z, blendIn);

			// 空を見上げるようにカメラを上へ傾け、ゆっくり左右にも傾ける
			const float tilt = EaseInOutCubic(std::clamp(cinematicTimer_ / (kAttackIntroInTime + 0.6f), 0.0f, 1.0f));
			camera_->rotation_.x = cinematicBaseRotation_.x - 0.10f * tilt;
			camera_->rotation_.z = cinematicBaseRotation_.z + 0.03f * std::sin(cinematicTimer_ * 1.2f) * tilt;
			return true;
		}
	}

	// =========================================================
	// ④ 攻撃開始演出の後: プレイヤー追従へなめらかに戻る（少し引いた位置へ）
	// =========================================================
	if (bossCinematic_ == BossCinematic::kAttackOut) {
		cinematicTimer_ += dt;
		isBossCinematicWide_ = true;

		const float t = std::clamp(cinematicTimer_ / kAttackOutTime, 0.0f, 1.0f);
		const float easeVal = EaseInOutCubic(t);

		// 攻撃中の追従カメラ（少し引いて上へ）と同じ位置へ戻るので、切り替わりで飛ばない
		KamataEngine::Vector3 returnPos = {playerTargetPos.x, playerTargetPos.y + 1.2f, playerTargetPos.z - 3.5f};
		camera_->translation_.x = Lerp(cinematicStartPos_.x, returnPos.x, easeVal);
		camera_->translation_.y = Lerp(cinematicStartPos_.y, returnPos.y, easeVal);
		camera_->translation_.z = Lerp(cinematicStartPos_.z, returnPos.z, easeVal);

		camera_->rotation_.x = Lerp(cinematicStartRot_.x, cinematicBaseRotation_.x - 0.06f, easeVal);
		camera_->rotation_.z = Lerp(cinematicStartRot_.z, cinematicBaseRotation_.z, easeVal);

		if (t >= 1.0f) {
			bossCinematic_ = BossCinematic::kNone;
			attackFollowBlend_ = 1.0f;
		}
		return true;
	}

	// =========================================================
	// ⑤ 攻撃中: プレイヤーを追従しながら、少し引いて空を見上げる（最後のクリスタルが落ちたら少し寄る）
	// =========================================================
	if (!isAttacking && attackFollowBlend_ <= 0.0f) {
		return false;
	}

	const float blendTarget = isAttacking ? (finalBoss_->IsCrystal22Landed() ? 0.4f : 1.0f) : 0.0f;
	attackFollowBlend_ += (blendTarget - attackFollowBlend_) * 0.04f;

	if (!isAttacking && attackFollowBlend_ < 0.003f) {
		// 攻撃が終わって元の追従に戻りきったら、傾きも元に戻す
		attackFollowBlend_ = 0.0f;
		camera_->rotation_ = cinematicBaseRotation_;
		return false;
	}

	cinematicOffset_ = {0.0f, 1.2f * attackFollowBlend_, -3.5f * attackFollowBlend_};
	UpdateFollow();
	cinematicOffset_ = {0.0f, 0.0f, 0.0f};

	camera_->rotation_.x = cinematicBaseRotation_.x - 0.06f * attackFollowBlend_;
	camera_->rotation_.z = cinematicBaseRotation_.z;
	return true;
}

// ★ 敵への打撃のカメラ演出を開始する
void CameraController::CombatHit(const KamataEngine::Vector3& hitPos, float strength) {
	combatHitTimer_ = kCombatHitDuration;
	combatHitStrength_ = strength;
	combatHitFocus_ = hitPos;
	combatHitTiltSign_ = -combatHitTiltSign_; // 1 回ごとに傾ける向きを左右交互にする
}

// ★ 敵への打撃のカメラ演出を毎フレーム反映する（打撃の瞬間が一番強く、二次曲線で減衰する）
void CameraController::ApplyCombatHitEffect() {
	if (combatHitTimer_ <= 0.0f || !camera_) {
		return;
	}

	combatHitTimer_ = std::max(0.0f, combatHitTimer_ - 1.0f / 60.0f);
	const float k = combatHitTimer_ / kCombatHitDuration;
	const float power = combatHitStrength_ * k * k;

	// 打撃の瞬間にぐっと寄り、打撃を受けた敵の方へ少し振れる
	combatHitAppliedOffset_.z = kCombatHitZoomDistance * power;
	combatHitAppliedOffset_.x = (combatHitFocus_.x - camera_->translation_.x) * kCombatHitPanRate * power;
	combatHitAppliedOffset_.y = (combatHitFocus_.y - camera_->translation_.y) * kCombatHitPanRate * power;

	// カメラを斜めに傾ける（時間とともに戻る）
	combatHitAppliedRotation_.z = kCombatHitTiltRoll * power * combatHitTiltSign_;
	combatHitAppliedRotation_.x = kCombatHitTiltPitch * power * combatHitTiltSign_;

	camera_->translation_.x += combatHitAppliedOffset_.x;
	camera_->translation_.y += combatHitAppliedOffset_.y;
	camera_->translation_.z += combatHitAppliedOffset_.z;
	camera_->rotation_.x += combatHitAppliedRotation_.x;
	camera_->rotation_.z += combatHitAppliedRotation_.z;
}
