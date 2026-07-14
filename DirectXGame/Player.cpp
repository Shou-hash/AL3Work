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

// デストラクタでモデルのメモリおよび残ったエフェクトを解放
Player::~Player() {
	if (modelHitEffect_) {
		delete modelHitEffect_;
		modelHitEffect_ = nullptr;
	}
	// リスト内に残っているエフェクトのメモリを全て解放
	for (auto* effect : hitEffects_) {
		delete effect;
	}
	hitEffects_.clear();
}

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

	// 指定されたモデルファイル名 "hit_effect" をロード
	if (modelHitEffect_ == nullptr) {
		modelHitEffect_ = KamataEngine::Model::CreateFromOBJ("hit_effect", true);

		assert(modelHitEffect_ != nullptr);
	}

	// 既存のエフェクトリストがあれば一度全て解放してクリア
	for (auto* effect : hitEffects_) {
		delete effect;
	}
	hitEffects_.clear();
}

void Player::KeysPush() {}

// 衝突時処理
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

// 通常行動の初期化
void Player::BehaviorRootInit() {}

// 通常行動の更新処理
void Player::BehaviorRootUpdate() {
	Move();

	if (KamataEngine::Input::GetInstance()->TriggerKey(DIK_D)) {
		behaviorRequest_ = Behavior::kAttack;
	}

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

// 攻撃行動の初期化
void Player::BehaviorAttackInit() {
	attackPhase_ = AttackPhase::kCharge;
	attackParameter_ = 0;
}

// 攻撃行動の更新処理
void Player::BehaviorAttackUpdate() {
	attackParameter_++;
	KamataEngine::Vector3 velocity = {};

	switch (attackPhase_) {
	case AttackPhase::kCharge:
	default: {
		float t = static_cast<float>(attackParameter_) / static_cast<float>(kChargeDuration);
		t = std::min(t, 1.0f);

		worldTransform_.scale_.z = EaseOut(1.0f, 0.3f, t);
		worldTransform_.scale_.y = EaseOut(1.0f, 1.6f, t);

		if (attackParameter_ >= kChargeDuration) {
			attackPhase_ = AttackPhase::kDash;
			attackParameter_ = 0;
		}
		break;
	}

	case AttackPhase::kDash: {
		float t = static_cast<float>(attackParameter_) / static_cast<float>(kDashDuration);
		t = std::min(t, 1.0f);

		worldTransform_.scale_.z = EaseOut(0.3f, 1.3f, t);
		worldTransform_.scale_.y = EaseIn(1.6f, 0.7f, t);

		if (lrDirection_ == LRDirection::kRight) {
			velocity.x = +kAttackVelocity;
		} else {
			velocity.x = -kAttackVelocity;
		}

		if (attackParameter_ >= kDashDuration) {
			attackPhase_ = AttackPhase::kRecoil;
			attackParameter_ = 0;
		}
		break;
	}

	case AttackPhase::kRecoil: {
		float t = static_cast<float>(attackParameter_) / static_cast<float>(kRecoilDuration);
		t = std::min(t, 1.0f);

		worldTransform_.scale_.z = EaseOut(1.3f, 1.0f, t);
		worldTransform_.scale_.y = EaseOut(0.7f, 1.0f, t);

		if (attackParameter_ >= kRecoilDuration) {
			behaviorRequest_ = Behavior::kRoot;
		}
		break;
	}
	}

	CollisionMapInfo collisionMapInfo;
	collisionMapInfo.moveAmount = velocity;
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

	worldTransform_.matWorld_ = MakeAffineMatrix(worldTransform_.scale_, worldTransform_.rotation_, worldTransform_.translation_);
	worldTransform_.TransferMatrix();
}

// ヒートエフェクトを動的に生成する関数
void Player::CreateHitEffect(const KamataEngine::Vector3& position) {
	// メモリ確保
	HitEffect* newEffect = new HitEffect();

	// 1. 完全に初期化
	newEffect->worldTransform.Initialize();

	// 2. 座標・回転・スケールを明示的にセット（最初は見えやすいように 1.0f で固定してテストします）
	newEffect->worldTransform.translation_ = position;
	newEffect->worldTransform.rotation_ = {0.0f, 0.0f, 0.0f};
	newEffect->worldTransform.scale_ = {1.0f, 1.0f, 1.0f};

	newEffect->timer = 0;
	newEffect->duration = 30; // 30フレーム表示
	newEffect->isDead = false;

	// 3. アフィン変換行列の合成と即時転送
	newEffect->worldTransform.matWorld_ = MakeAffineMatrix(newEffect->worldTransform.scale_, newEffect->worldTransform.rotation_, newEffect->worldTransform.translation_);
	newEffect->worldTransform.TransferMatrix();

	// リストに追加
	hitEffects_.push_back(newEffect);
}

void Player::Update() {
	if (isDead_) {
		return;
	}

	// デバッグ用：【Tキー】を押したらプレイヤーの目の前に強制的にエフェクトを生成する（判定の不具合を切り分けるため）
	if (KamataEngine::Input::GetInstance()->TriggerKey(DIK_T)) {
		CreateHitEffect(worldTransform_.translation_);
	}

	if (behaviorRequest_) {
		behavior_ = behaviorRequest_.value();
		switch (behavior_) {
		case Behavior::kRoot:
			BehaviorRootInit();
			break;
		case Behavior::kAttack:
			BehaviorAttackInit();
			break;
		}
		behaviorRequest_ = std::nullopt;
	}

	switch (behavior_) {
	case Behavior::kRoot:
		BehaviorRootUpdate();
		break;
	case Behavior::kAttack:
		BehaviorAttackUpdate();
		break;
	}

	// ヒートエフェクトの一斉更新処理
	for (auto* effect : hitEffects_) {
		effect->timer++;
		if (effect->timer >= effect->duration) {
			effect->isDead = true;
		} else {
			// 徐々に大きくするイージング（ここが原因で消えている可能性を考慮し、一旦単純な加算にします）
			float t = static_cast<float>(effect->timer) / static_cast<float>(effect->duration);
			float scaleVal = 1.0f + t * 2.0f; // 1.0倍から3.0倍へ徐々に拡大
			effect->worldTransform.scale_ = {scaleVal, scaleVal, scaleVal};

			// 毎フレーム必ず行列を再計算してGPUに転送する
			effect->worldTransform.matWorld_ = MakeAffineMatrix(effect->worldTransform.scale_, effect->worldTransform.rotation_, effect->worldTransform.translation_);
			effect->worldTransform.TransferMatrix();
		}
	}

	// 寿命を迎えたエフェクトのメモリを解放し、リストから削除
	for (auto it = hitEffects_.begin(); it != hitEffects_.end();) {
		if ((*it)->isDead) {
			delete *it;
			it = hitEffects_.erase(it);
		} else {
			++it;
		}
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

			if (behavior_ == Behavior::kAttack && attackPhase_ == AttackPhase::kDash) {
				KamataEngine::Vector3 hitPos = {(worldTransform_.translation_.x + enemyPos.x) / 2.0f, (worldTransform_.translation_.y + enemyPos.y) / 2.0f, worldTransform_.translation_.z};
				CreateHitEffect(hitPos);

				// 必要に応じてここに敵撃破の処理を記述
			} else {
				OnCollision();
			}
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
		return;
	}

	// プレイヤーモデルの描画
	if (modelPlayer_ && camera_) {
		// ※ GameScene側で PreDraw / PostDraw されているため、ここでは描画のみ行う
		modelPlayer_->Draw(worldTransform_, *camera_);
	}

	// 生成されているすべてのヒートエフェクトを描画
	if (modelHitEffect_ && camera_ && !hitEffects_.empty()) {
		// エフェクトもGameScene側の描画パス（PreDraw〜PostDraw）の中で描画されるようにします
		for (const auto* effect : hitEffects_) {
			modelHitEffect_->Draw(effect->worldTransform, *camera_);
		}
	}
	
}