#define NOMINMAX
#include "Player.h"
#include "CameraController.h"
#include "MapChipField.h"
#include "Matrix4x4.h"
#include <algorithm>
#include <cmath>
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

	velocity_ = {0.0f, 0.0f, 0.0f};
	onGround_ = true;
	isDead_ = false;
}

void Player::KeysPush() {}

void Player::Move() {
	if (isDead_)
		return; // 死亡時は移動不可

	velocity_.z = 0.0f;

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
		velocity_.x *= (1.0f - kAttenuation);
		if (std::abs(velocity_.x) < 0.001f) {
			velocity_.x = 0.0f;
		}
	}
	velocity_.x = std::clamp(velocity_.x, -kLimitRunSpeed, kLimitRunSpeed);

	if (onGround_) {
		if (KamataEngine::Input::GetInstance()->TriggerKey(DIK_UP)) {
			velocity_.y = kJumpAcceleration;
			// 離地フラグは ApplyGroundingStatus 内で速度を見て制御するため、ここでは velocity 変更のみに留めます
		}
	} else {
		velocity_.y -= kGravityAcceleration;
		velocity_.y = std::max(velocity_.y, -kLimitFallSpeed);
	}
}

void Player::Update() {
	if (isDead_) {
		// 死亡時の演出処理（下に落ちていく処理）
		worldTransform_.translation_.y -= kGravityAcceleration;
		worldTransform_.matWorld_ = MakeAffineMatrix(worldTransform_.scale_, worldTransform_.rotation_, worldTransform_.translation_);
		worldTransform_.TransferMatrix();
		return;
	}

	// 1. 移動入力と重力の計算
	Move();

	// 2. 衝突判定用の移動量を設定
	CollisionMapInfo collisionMapInfo;
	collisionMapInfo.moveAmount = velocity_;
	collisionMapInfo.onGround = onGround_;

	// 3. マップとの衝突判定と補正
	MapCollision(collisionMapInfo);

	// 4. 補正された移動量を座標に反映
	worldTransform_.translation_.x += collisionMapInfo.moveAmount.x;
	worldTransform_.translation_.y += collisionMapInfo.moveAmount.y;
	worldTransform_.translation_.z += collisionMapInfo.moveAmount.z;

	// 5. 資料⑥：接地状態の切り替え処理を適用
	ApplyGroundingStatus(collisionMapInfo);

	// 天井にぶつかったら垂直速度をリセット
	if (collisionMapInfo.ceilingCollision) {
		velocity_.y = 0.0f;
	}
	if (collisionMapInfo.wallCollision) {
		velocity_.x = 0.0f;
	}

	// 画面端での押し出し、および壁との挟まれ死亡判定処理
	CheckScreenEdgeCollision();

	// ターンアニメーション処理
	if (turnTimer_ < kTimeTurn) {
		turnTimer_ += 1.0f / 60.0f;
		if (turnTimer_ > kTimeTurn) {
			turnTimer_ = kTimeTurn;
		}
	}

	float destinationRotationYTable[] = {
	    std::numbers::pi_v<float> * 3.0f / 2.0f, // kLeft
	    std::numbers::pi_v<float> / 2.0f,        // kRight
	};
	float destinationRotationY = destinationRotationYTable[static_cast<uint32_t>(lrDirection_)];

	float ratio = turnTimer_ / kTimeTurn;
	worldTransform_.rotation_.y = turnFirstRotationY_ + (destinationRotationY - turnFirstRotationY_) * ratio;

	worldTransform_.matWorld_ = MakeAffineMatrix(worldTransform_.scale_, worldTransform_.rotation_, worldTransform_.translation_);
	worldTransform_.TransferMatrix();
}

void Player::ApplyGroundingStatus(const CollisionMapInfo& info) {
	if (!mapChipField_) {
		return;
	}

	if (onGround_) {
		// 接地している時の処理
		// Y速度が上向きになった＝ジャンプを開始したということなので、空中状態に切り替える
		if (velocity_.y > 0.0f) {
			onGround_ = false;
		} else {
			// 左右移動や床がなくなるなど、ジャンプせずに落下を始める場合の判定（吸着処理）
			// 現在の足元から、微小な数値（kGroundSearchOffset）だけ下にずらして床があるか判定を行う
			KamataEngine::Vector3 currentCenter = worldTransform_.translation_;
			KamataEngine::Vector3 offset = {0.0f, -kGroundSearchOffset, 0.0f};

			KamataEngine::Vector3 leftBottomPos;
			leftBottomPos.y = CornerPosition(currentCenter, kLeftBottom).y + offset.y;
			KamataEngine::Vector3 rightBottomPos;
			rightBottomPos.y = CornerPosition(currentCenter, kRightBottom).y + offset.y;

			MapChipType chipLeftBottom = mapChipField_->GetMapChipTypeByPosition(leftBottomPos);
			MapChipType chipRightBottom = mapChipField_->GetMapChipTypeByPosition(rightBottomPos);

			bool hit = (chipLeftBottom == MapChipType::kBlock || chipRightBottom == MapChipType::kBlock);

			// 足元にブロックが一切なければ、空中状態に切り替える（落下開始）
			if (!hit) {
				onGround_ = false;
			}
		}
	} else {
		// 空中にいる時の処理
		// 今回の当たり判定で床（ブロック）にヒットしたら、空中状態から接地状態に切り替える
		if (info.onGround) {
			onGround_ = true;

			// 着地時にX速度を減衰させる
			velocity_.x *= (1.0f - kAttenuationLanding);
			// Y速度をゼロにすることで下移動を止める
			velocity_.y = 0.0f;
		}
	}
}

void Player::CheckScreenEdgeCollision() {
	if (!cameraController_ || !mapChipField_) {
		return;
	}

	float cameraLeftX = cameraController_->GetCameraLeftX();
	float playerLeftX = worldTransform_.translation_.x - (kWidth / 2.0f);

	if (playerLeftX < cameraLeftX) {
		worldTransform_.translation_.x = cameraLeftX + (kWidth / 2.0f);

		KamataEngine::Vector3 currentCenter = worldTransform_.translation_;
		KamataEngine::Vector3 rightTopPos = CornerPosition(currentCenter, kRightTop);
		KamataEngine::Vector3 rightBottomPos = CornerPosition(currentCenter, kRightBottom);

		MapChipType chipRightTop = mapChipField_->GetMapChipTypeByPosition(rightTopPos);
		MapChipType chipRightBottom = mapChipField_->GetMapChipTypeByPosition(rightBottomPos);

		if (chipRightTop == MapChipType::kBlock || chipRightBottom == MapChipType::kBlock) {
			isDead_ = true;
			velocity_ = {0.0f, 0.0f, 0.0f};
		}
	}
}

KamataEngine::Vector3 Player::CornerPosition(const KamataEngine::Vector3& center, Corner corner) {
	const float insetX = kWidth / 2.0f - 0.01f;
	const float insetY = kHeight / 2.0f - 0.01f;

	const KamataEngine::Vector3 offsetTable[kNumCorner] = {
	    {+insetX, -insetY, 0.0f}, // kRightBottom
	    {-insetX, -insetY, 0.0f}, // kLeftBottom
	    {+insetX, +insetY, 0.0f}, // kRightTop
	    {-insetX, +insetY, 0.0f}  // kLeftTop
	};

	KamataEngine::Vector3 result;
	result.x = center.x + offsetTable[static_cast<uint32_t>(corner)].x;
	result.y = center.y + offsetTable[static_cast<uint32_t>(corner)].y;
	result.z = center.z + offsetTable[static_cast<uint32_t>(corner)].z;
	return result;
}

void Player::MapCollision(CollisionMapInfo& info) {
	if (!mapChipField_) {
		return;
	}

	MapCollisionRight(info);
	MapCollisionLeft(info);
	MapCollisionTop(info);
	MapCollisionBottom(info);
}

void Player::MapCollisionTop(CollisionMapInfo& info) {
	if (info.moveAmount.y <= 0.0f) {
		return;
	}

	KamataEngine::Vector3 nextCenter = {worldTransform_.translation_.x + info.moveAmount.x, worldTransform_.translation_.y + info.moveAmount.y, worldTransform_.translation_.z};

	std::array<KamataEngine::Vector3, kNumCorner> positionsNew;
	for (uint32_t i = 0; i < kNumCorner; ++i) {
		positionsNew[i] = CornerPosition(nextCenter, static_cast<Corner>(i));
	}

	MapChipType chipLeftTop = mapChipField_->GetMapChipTypeByPosition(positionsNew[kLeftTop]);
	MapChipType chipRightTop = mapChipField_->GetMapChipTypeByPosition(positionsNew[kRightTop]);

	if (chipLeftTop == MapChipType::kBlock || chipRightTop == MapChipType::kBlock) {
		Corner targetCorner = (chipLeftTop == MapChipType::kBlock) ? kLeftTop : kRightTop;
		MapChipField::IndexSet index = mapChipField_->GetMapChipIndexByPosition(positionsNew[targetCorner]);
		KamataEngine::Vector3 blockPos = mapChipField_->GetMapChipPositionByIndex(index.x, index.y);

		float blockBottomY = blockPos.y - 0.5f;
		if (blockBottomY > worldTransform_.translation_.y) {
			info.ceilingCollision = true;
			info.moveAmount.y = blockBottomY - (worldTransform_.translation_.y + kHeight / 2.0f) - 0.005f;
		}
	}
}

void Player::MapCollisionBottom(CollisionMapInfo& info) {
	// 資料①より：上方向への移動（上昇中）であれば、下方向の当たり判定自体をスキップする
	if (info.moveAmount.y >= 0.0f) {
		return;
	}

	KamataEngine::Vector3 nextCenter = {worldTransform_.translation_.x + info.moveAmount.x, worldTransform_.translation_.y + info.moveAmount.y, worldTransform_.translation_.z};

	std::array<KamataEngine::Vector3, kNumCorner> positionsNew;
	for (uint32_t i = 0; i < kNumCorner; ++i) {
		positionsNew[i] = CornerPosition(nextCenter, static_cast<Corner>(i));
	}

	MapChipType chipLeftBottom = mapChipField_->GetMapChipTypeByPosition(positionsNew[kLeftBottom]);
	MapChipType chipRightBottom = mapChipField_->GetMapChipTypeByPosition(positionsNew[kRightBottom]);

	if (chipLeftBottom == MapChipType::kBlock || chipRightBottom == MapChipType::kBlock) {
		Corner targetCorner = (chipLeftBottom == MapChipType::kBlock) ? kLeftBottom : kRightBottom;
		MapChipField::IndexSet index = mapChipField_->GetMapChipIndexByPosition(positionsNew[targetCorner]);
		KamataEngine::Vector3 blockPos = mapChipField_->GetMapChipPositionByIndex(index.x, index.y);
		float blockTopY = blockPos.y + 0.5f;

		float previousBottomY = worldTransform_.translation_.y - kHeight / 2.0f;
		if (previousBottomY >= blockTopY - 0.2f) {
			// 床ヒット時に移動量を補正し、一時構造体に記録する（即時書き換えは行わない）
			info.onGround = true;
			info.moveAmount.y = blockTopY - previousBottomY;
			return;
		}
	}

	info.onGround = false;
}

void Player::MapCollisionRight(CollisionMapInfo& info) {
	if (info.moveAmount.x <= 0.0f) {
		return;
	}

	KamataEngine::Vector3 nextCenter = {worldTransform_.translation_.x + info.moveAmount.x, worldTransform_.translation_.y, worldTransform_.translation_.z};

	std::array<KamataEngine::Vector3, kNumCorner> positionsNew;
	for (uint32_t i = 0; i < kNumCorner; ++i) {
		positionsNew[i] = CornerPosition(nextCenter, static_cast<Corner>(i));
	}

	MapChipType chipRightTop = mapChipField_->GetMapChipTypeByPosition(positionsNew[kRightTop]);
	MapChipType chipRightBottom = mapChipField_->GetMapChipTypeByPosition(positionsNew[kRightBottom]);

	if (chipRightTop == MapChipType::kBlock || chipRightBottom == MapChipType::kBlock) {
		info.wallCollision = true;

		Corner targetCorner = (chipRightTop == MapChipType::kBlock) ? kRightTop : kRightBottom;
		MapChipField::IndexSet index = mapChipField_->GetMapChipIndexByPosition(positionsNew[targetCorner]);
		KamataEngine::Vector3 blockPos = mapChipField_->GetMapChipPositionByIndex(index.x, index.y);

		float blockLeftX = blockPos.x - 0.5f;
		info.moveAmount.x = blockLeftX - (worldTransform_.translation_.x + kWidth / 2.0f) - 0.005f;
	}
}

void Player::MapCollisionLeft(CollisionMapInfo& info) {
	if (info.moveAmount.x >= 0.0f) {
		return;
	}

	KamataEngine::Vector3 nextCenter = {worldTransform_.translation_.x + info.moveAmount.x, worldTransform_.translation_.y, worldTransform_.translation_.z};

	std::array<KamataEngine::Vector3, kNumCorner> positionsNew;
	for (uint32_t i = 0; i < kNumCorner; ++i) {
		positionsNew[i] = CornerPosition(nextCenter, static_cast<Corner>(i));
	}

	MapChipType chipLeftTop = mapChipField_->GetMapChipTypeByPosition(positionsNew[kLeftTop]);
	MapChipType chipLeftBottom = mapChipField_->GetMapChipTypeByPosition(positionsNew[kLeftBottom]);

	if (chipLeftTop == MapChipType::kBlock || chipLeftBottom == MapChipType::kBlock) {
		info.wallCollision = true;

		Corner targetCorner = (chipLeftTop == MapChipType::kBlock) ? kLeftTop : kLeftBottom;
		MapChipField::IndexSet index = mapChipField_->GetMapChipIndexByPosition(positionsNew[targetCorner]);
		KamataEngine::Vector3 blockPos = mapChipField_->GetMapChipPositionByIndex(index.x, index.y);

		float blockRightX = blockPos.x + 0.5f;
		info.moveAmount.x = blockRightX - (worldTransform_.translation_.x - kWidth / 2.0f) + 0.005f;
	}
}

void Player::Draw() {
	if (modelPlayer_ && camera_) {
		KamataEngine::Model::PreDraw();
		modelPlayer_->Draw(worldTransform_, *camera_);
		KamataEngine::Model::PostDraw();
	}
}