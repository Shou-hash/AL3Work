#define NOMINMAX
#include "Player.h"
#include "Matrix4x4.h" // MakeAffineMatrix を使うためにインクルード
#include <algorithm>
#include <numbers>

Player::Player() {}
Player::~Player() {}

void Player::Initialize(KamataEngine::Model* model, KamataEngine::Camera* camera, const KamataEngine::Vector3& position) {
	modelPlayer_ = model;
	camera_ = camera;

	worldTransform_.Initialize();
	worldTransform_.translation_ = position;

	worldTransform_.rotation_.y = std::numbers::pi_v<float> / 2.0f;
	lrDirection_ = LRDirection::kRight;

	turnTimer_ = kTimeTurn;
	turnFirstRotationY_ = worldTransform_.rotation_.y;
}

void Player::Update() {
	velocity_.z = 0.0f;

	// 【改善】移動入力を onGround_ の外に出すことで、空中でも左右に動けるようにします
	if (KamataEngine::Input::GetInstance()->PushKey(DIK_RIGHT)) {
		if (lrDirection_ != LRDirection::kRight) {
			lrDirection_ = LRDirection::kRight;
			turnFirstRotationY_ = worldTransform_.rotation_.y;
			turnTimer_ = 0.0f;
		}
		if (velocity_.x < 0.0f) {
			velocity_.x *= (1.0f - kAttenuation);
		}
		velocity_.x += kAcceleration;
	} else if (KamataEngine::Input::GetInstance()->PushKey(DIK_LEFT)) {
		if (lrDirection_ != LRDirection::kLeft) {
			lrDirection_ = LRDirection::kLeft;
			turnFirstRotationY_ = worldTransform_.rotation_.y;
			turnTimer_ = 0.0f;
		}
		if (velocity_.x > 0.0f) {
			velocity_.x *= (1.0f - kAttenuation);
		}
		velocity_.x -= kAcceleration;
	} else {
		// キーが押されていない時は減速
		velocity_.x *= (1.0f - kAttenuation);
		if (std::abs(velocity_.x) < 0.001f) {
			velocity_.x = 0.0f;
		}
	}

	// 左右の速度制限
	velocity_.x = std::clamp(velocity_.x, -kLimitRunSpeed, kLimitRunSpeed);

	// 地上・空中の処理
	if (onGround_) {
		// 【修正】y速度がリセットされなくなったため、これで正しくジャンプできます
		if (KamataEngine::Input::GetInstance()->TriggerKey(DIK_UP)) {
			velocity_.y = kJumpAcceleration;
			onGround_ = false;
		}
	} else {
		// 空中の処理：重力を累積させる
		velocity_.y -= kGravityAcceleration;
		velocity_.y = std::max(velocity_.y, -kLimitFallSpeed);
	}

	// 着地判定（落下中のみ判定）
	bool landing = false;
	if (velocity_.y < 0.0f) {
		if (worldTransform_.translation_.y <= 1.0f) { // 地面の高さを y=1.0f と仮定
			landing = true;
		}
	}

	if (onGround_) {
		// 足場から踏み外して落ちた場合の判定
		if (worldTransform_.translation_.y > 1.0f) {
			onGround_ = false;
		}
	} else {
		if (landing) {
			worldTransform_.translation_.y = 1.0f;
			// 【修正】着地時の不自然な x への速度加算バグを削除
			velocity_.y = 0.0f;
			onGround_ = true;
		}
	}

	// 旋回タイマーの更新と補間
	if (turnTimer_ < kTimeTurn) {
		turnTimer_ += 1.0f / 60.0f;
		if (turnTimer_ > kTimeTurn) {
			turnTimer_ = kTimeTurn;
		}
	}

	// 目標角度の決定
	float destinationRotationYTable[] = {
	    std::numbers::pi_v<float> * 3.0f / 2.0f, // kLeft  (270度)
	    std::numbers::pi_v<float> / 2.0f,        // kRight (90度)
	};
	float destinationRotationY = destinationRotationYTable[static_cast<uint32_t>(lrDirection_)];

	// 旋回イージングの計算
	float ratio = turnTimer_ / kTimeTurn;
	worldTransform_.rotation_.y = turnFirstRotationY_ + (destinationRotationY - turnFirstRotationY_) * ratio;

	// 速度を座標に反映
	worldTransform_.translation_.x += velocity_.x;
	worldTransform_.translation_.y += velocity_.y;
	worldTransform_.translation_.z += velocity_.z;

	// 行列の更新と転送
	worldTransform_.matWorld_ = MakeAffineMatrix(worldTransform_.scale_, worldTransform_.rotation_, worldTransform_.translation_);
	worldTransform_.TransferMatrix();
}

void Player::Draw() {
	if (modelPlayer_ && camera_) {
		KamataEngine::Model::PreDraw();
		modelPlayer_->Draw(worldTransform_, *camera_);
		KamataEngine::Model::PostDraw();
	}
}