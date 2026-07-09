#define NOMINMAX
#include "Player.h"
#include "CameraController.h"
#include "Enemy.h"
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
	isDead_ = false; // 初期化時は生存

	behavior_ = Behavior::kRoot;
	behaviorRequest_ = std::nullopt;
	attackTimer_ = 0;
}

void Player::KeysPush() {}

// 衝突時処理（スクリーンショット「当たったら死ぬ」の再現）
void Player::OnCollision() {
	isDead_ = true; // デスフラグを立てる
}

void Player::Move() {
	if (isDead_) {
		return;
	}

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
		}
	} else {
		velocity_.y -= kGravityAcceleration;
		velocity_.y = std::max(velocity_.y, -kLimitFallSpeed);
	}
}

// 通常行動の初期化（空のままで構いません）
void Player::BehaviorRootInit() {}

// 通常行動の更新処理
void Player::BehaviorRootUpdate() {
	// 1. キー入力などによる通常の移動処理
	Move();

	// 2. 攻撃ボタン（SPACEキー）が押されたら攻撃状態へリクエスト
	if (KamataEngine::Input::GetInstance()->TriggerKey(DIK_D)) {
		behaviorRequest_ = Behavior::kAttack;
	}

	// 3. 衝突判定と座標の加算
	CollisionMapInfo collisionMapInfo;
	collisionMapInfo.moveAmount = velocity_;
	collisionMapInfo.onGround = onGround_;
	MapCollision(collisionMapInfo);

	worldTransform_.translation_.x += collisionMapInfo.moveAmount.x;
	worldTransform_.translation_.y += collisionMapInfo.moveAmount.y;
	worldTransform_.translation_.z += collisionMapInfo.moveAmount.z;
	ApplyGroundingStatus(collisionMapInfo);

	if (collisionMapInfo.ceilingCollision) {
		velocity_.y = 0.0f;
	}
	if (collisionMapInfo.wallCollision) {
		velocity_.x = 0.0f;
	}

	CheckScreenEdgeCollision();

	// 4. 旋回と行列の更新
	if (turnTimer_ < kTimeTurn) {
		turnTimer_ += 1.0f / 60.0f;
		if (turnTimer_ > kTimeTurn) {
			turnTimer_ = kTimeTurn;
		}
	}
	float destinationRotationYTable[] = {
	    std::numbers::pi_v<float> * 3.0f / 2.0f,
	    std::numbers::pi_v<float> / 2.0f,
	};
	float destinationRotationY = destinationRotationYTable[static_cast<uint32_t>(lrDirection_)];
	float ratio = turnTimer_ / kTimeTurn;
	worldTransform_.rotation_.y = turnFirstRotationY_ + (destinationRotationY - turnFirstRotationY_) * ratio;

	worldTransform_.matWorld_ = MakeAffineMatrix(worldTransform_.scale_, worldTransform_.rotation_, worldTransform_.translation_);
	worldTransform_.TransferMatrix();
}

// ★ 攻撃行動の初期化（突進速度に定数を適用）
void Player::BehaviorAttackInit() {
	// 攻撃タイマーをセット
	attackTimer_ = kAttackDuration;

	// プレイヤーの向いている方向に応じて、定数から突進速度を設定
	if (lrDirection_ == LRDirection::kRight) {
		velocity_.x = kAttackSpeed; // ★ 定数を使用
	} else {
		velocity_.x = -kAttackSpeed; // ★ 定数を使用
	}
	velocity_.y = 0.0f; // 必要に応じてY軸の速度をリセット（空中突進時の落下を防ぐ場合）
}

// ★ 攻撃行動の更新処理（スクショの指示通りに完成）
void Player::BehaviorAttackUpdate() {
	// 1. タイマーのカウントダウンと通常行動への復帰
	if (attackTimer_ > 0) {
		attackTimer_--;
	}
	if (attackTimer_ == 0) {
		behaviorRequest_ = Behavior::kRoot;
	}

	// 2. 攻撃中の移動とマップ衝突判定（Move()は呼ばず、Initで決めた速度を維持）
	CollisionMapInfo collisionMapInfo;
	collisionMapInfo.moveAmount = velocity_;
	collisionMapInfo.onGround = onGround_;
	MapCollision(collisionMapInfo);

	// 座標への加算
	worldTransform_.translation_.x += collisionMapInfo.moveAmount.x;
	worldTransform_.translation_.y += collisionMapInfo.moveAmount.y;
	worldTransform_.translation_.z += collisionMapInfo.moveAmount.z;

	// 必要に応じて着地状態や壁衝突状態を適用
	ApplyGroundingStatus(collisionMapInfo);
	if (collisionMapInfo.ceilingCollision) {
		velocity_.y = 0.0f;
	}
	if (collisionMapInfo.wallCollision) {
		velocity_.x = 0.0f;
	}

	// 画面端の衝突判定
	CheckScreenEdgeCollision();

	// 3. アフィニティ行列の計算と転送（描画用）
	worldTransform_.matWorld_ = MakeAffineMatrix(worldTransform_.scale_, worldTransform_.rotation_, worldTransform_.translation_);
	worldTransform_.TransferMatrix();
}

void Player::Update() {
	if (isDead_) {
		return;
	}

	// ★ 状態遷移のリクエストがあるか確認
	if (behaviorRequest_) {
		// 振る舞いを変更
		behavior_ = behaviorRequest_.value();
		// 各状態の初期化関数を呼び出す
		switch (behavior_) {
		case Behavior::kRoot:
			BehaviorRootInit();
			break;
		case Behavior::kAttack:
			BehaviorAttackInit();
			break;
		}
		// リクエストをリセット
		behaviorRequest_ = std::nullopt;
	}

	// ★ 現在の状態に応じて実行する更新処理を切り替える
	switch (behavior_) {
	case Behavior::kRoot:
		BehaviorRootUpdate();
		break;
	case Behavior::kAttack:
		BehaviorAttackUpdate();
		break;
	}
}

void Player::CheckEnemyCollision(const std::list<Enemy*>& enemies) {
	if (isDead_) {
		return;
	}

	float playerLeft = worldTransform_.translation_.x - kWidth / 2.0f;
	float playerRight = worldTransform_.translation_.x + kWidth / 2.0f;
	float playerBottom = worldTransform_.translation_.y - kHeight / 2.0f;
	float playerTop = worldTransform_.translation_.y + kHeight / 2.0f;

	for (Enemy* enemy : enemies) {
		if (!enemy) {
			continue;
		}
		KamataEngine::Vector3 enemyPos = enemy->GetWorldTransform().translation_;
		float enemyWidth = 0.8f;
		float enemyHeight = 0.8f;
		float enemyLeft = enemyPos.x - enemyWidth / 2.0f;
		float enemyRight = enemyPos.x + enemyWidth / 2.0f;
		float enemyBottom = enemyPos.y - enemyHeight / 2.0f;
		float enemyTop = enemyPos.y + enemyHeight / 2.0f;

		if (playerLeft < enemyRight && playerRight > enemyLeft && playerBottom < enemyTop && playerTop > enemyBottom) {
			OnCollision(); // 衝突したら死亡
			break;
		}
	}
}

void Player::ApplyGroundingStatus(const CollisionMapInfo& info) {
	if (!mapChipField_) {
		return;
	}
	if (onGround_) {
		if (velocity_.y > 0.0f) {
			onGround_ = false;
		} else {
			KamataEngine::Vector3 currentCenter = worldTransform_.translation_;
			KamataEngine::Vector3 offset = {0.0f, -kGroundSearchOffset, 0.0f};
			KamataEngine::Vector3 leftBottomPos = CornerPosition(currentCenter, kLeftBottom);
			leftBottomPos.y += offset.y;
			KamataEngine::Vector3 rightBottomPos = CornerPosition(currentCenter, kRightBottom);
			rightBottomPos.y += offset.y;
			MapChipType chipLeftBottom = mapChipField_->GetMapChipTypeByPosition(leftBottomPos);
			MapChipType chipRightBottom = mapChipField_->GetMapChipTypeByPosition(rightBottomPos);
			bool hit = (chipLeftBottom == MapChipType::kBlock || chipRightBottom == MapChipType::kBlock);
			if (!hit) {
				onGround_ = false;
			}
		}
	} else {
		if (info.onGround) {
			onGround_ = true;
			velocity_.x *= (1.0f - kAttenuationLanding);
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
			OnCollision();
		}
	}
}

KamataEngine::Vector3 Player::CornerPosition(const KamataEngine::Vector3& center, Corner corner) {
	const float insetX = kWidth / 2.0f - 0.01f;
	const float insetY = kHeight / 2.0f - 0.01f;
	const KamataEngine::Vector3 offsetTable[kNumCorner] = {
	    {+insetX, -insetY, 0.0f},
        {-insetX, -insetY, 0.0f},
        {+insetX, +insetY, 0.0f},
        {-insetX, +insetY, 0.0f}
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
		float previousTopY = worldTransform_.translation_.y + kHeight / 2.0f;
		if (previousTopY <= blockBottomY + 0.05f) {
			info.ceilingCollision = true;
			info.moveAmount.y = blockBottomY - previousTopY - 0.005f;
			return;
		}
	}
}

void Player::MapCollisionBottom(CollisionMapInfo& info) {
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
	bool hit = (chipLeftBottom == MapChipType::kBlock || chipRightBottom == MapChipType::kBlock);
	if (hit) {
		Corner targetCorner = (chipLeftBottom == MapChipType::kBlock) ? kLeftBottom : kRightBottom;
		MapChipField::IndexSet index = mapChipField_->GetMapChipIndexByPosition(positionsNew[targetCorner]);
		KamataEngine::Vector3 blockPos = mapChipField_->GetMapChipPositionByIndex(index.x, index.y);
		float blockTopY = blockPos.y + 0.5f;
		float previousBottomY = worldTransform_.translation_.y - kHeight / 2.0f;
		float nextBottomY = nextCenter.y - kHeight / 2.0f;
		if (previousBottomY >= blockTopY - 0.2f && nextBottomY <= blockTopY) {
			info.onGround = true;
			info.moveAmount.y = blockTopY - previousBottomY;
			return;
		}
		if (nextBottomY < blockTopY) {
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
	KamataEngine::Vector3 nextCenter = {worldTransform_.translation_.x + info.moveAmount.x, worldTransform_.translation_.y + info.moveAmount.y, worldTransform_.translation_.z};
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
	KamataEngine::Vector3 nextCenter = {worldTransform_.translation_.x + info.moveAmount.x, worldTransform_.translation_.y + info.moveAmount.y, worldTransform_.translation_.z};
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
	if (isDead_) {
		return; // 死亡時は描画しない（敵と当たったら消える）
	}
	if (modelPlayer_ && camera_) {
		KamataEngine::Model::PreDraw();
		modelPlayer_->Draw(worldTransform_, *camera_);
		KamataEngine::Model::PostDraw();
	}
}