#include "CameraController.h"
#include "Player.h"
#include <algorithm>
#include "Matrix4x4.h"

float Lerp(float current, float target, float rate) { return current + rate * (target - current); }

void CameraController::Initialize(KamataEngine::Camera* camera) {
	// 引数で受け取ったカメラのポインタをメンバ変数に保存する
	camera_ = camera;
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
}

void CameraController::Update() {
	if (!target_ || !camera_) {
		return;
	}

	// ターゲットのワールドトランスフォームを取得
	const KamataEngine::WorldTransform& targetWorldTransform = target_->GetWorldTransform();

	const KamataEngine::Vector3& targetVelocity = target_->GetVelocity();

	// カメラの目標位置（プレイヤーの位置 ＋ オフセット）を計算
	targetPosition_.x = targetWorldTransform.translation_.x + targetOffset_.x + (targetVelocity.x * kVelocityBias);
	targetPosition_.y = targetWorldTransform.translation_.y + targetOffset_.y + (targetVelocity.y * kVelocityBias);
	targetPosition_.z = targetWorldTransform.translation_.z + targetOffset_.z + (targetVelocity.z * kVelocityBias);

	camera_->translation_.x = Lerp(camera_->translation_.x, targetPosition_.x, kInterpolationRate);
	camera_->translation_.y = Lerp(camera_->translation_.y, targetPosition_.y, kInterpolationRate);
	camera_->translation_.z = Lerp(camera_->translation_.z, targetPosition_.z, kInterpolationRate);

	Rect limitArea;
	limitArea.left = movableArea_.left + margin_.left;
	limitArea.right = movableArea_.right + margin_.right;
	limitArea.bottom = movableArea_.bottom + margin_.bottom;
	limitArea.top = movableArea_.top + margin_.top;

	limitArea.top = movableArea_.top + margin_.top + 20.0f;

	camera_->translation_.x = std::clamp(camera_->translation_.x, limitArea.left, limitArea.right);
	camera_->translation_.y = std::clamp(camera_->translation_.y, limitArea.bottom, limitArea.top);

	// 移動範囲（movableArea_）に収まるように制限（クランプ）して追従
	camera_->translation_.x = std::clamp(camera_->translation_.x, movableArea_.left, movableArea_.right);
	camera_->translation_.y = std::clamp(camera_->translation_.y, movableArea_.bottom, movableArea_.top);
	camera_->translation_.z = targetPosition_.z;
}