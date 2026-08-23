#include "CameraController.h"
#include "BossEnemy.h"
#include "Matrix4x4.h"
#include "Player.h"
#include <algorithm>
#include <cmath>

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

	// ボスへの接近判定と演出の開始
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

	// モードに応じてカメラの座標を更新
	if (isGoalPerformance_) {
		UpdateGoalPerformance();
	} else if (isBossPerformance_) {
		UpdateBossPerformance();
	} else if (mode_ == CameraMode::kFollow) {
		UpdateFollow();
	} else if (mode_ == CameraMode::kForcedScroll) {
		UpdateForcedScroll();
	}

	// 移動範囲（movableArea_）に収まるように制限（演出中は目標地点まで移動できるよう制限を解除）
	if (!isBossPerformance_ && !isGoalPerformance_) {
		camera_->translation_.x = std::clamp(camera_->translation_.x, movableArea_.left, movableArea_.right);
		camera_->translation_.y = std::clamp(camera_->translation_.y, movableArea_.bottom, movableArea_.top);
	}

	// 画面内への押し出し制限処理（強制スクロール時などに機能）
	ConstrainPlayerInScreen();
}

void CameraController::UpdateFollow() {
	const KamataEngine::WorldTransform& targetWorldTransform = target_->GetWorldTransform();
	const KamataEngine::Vector3& targetVelocity = target_->GetVelocity();

	// カメラの目標位置（プレイヤーの位置 ＋ オフセット）を計算
	targetPosition_.x = targetWorldTransform.translation_.x + targetOffset_.x + (targetVelocity.x * kVelocityBias);
	targetPosition_.y = targetWorldTransform.translation_.y + targetOffset_.y + (targetVelocity.y * kVelocityBias);
	targetPosition_.z = targetWorldTransform.translation_.z + targetOffset_.z + (targetVelocity.z * kVelocityBias);

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
	if (isBossPerformance_ || isGoalPerformance_) {
		return -9999.0f;
	}
	return camera_->translation_.x - kHalfScreenWidth;
}