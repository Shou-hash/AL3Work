#define NOMINMAX
#include "Player.h"
#include "BaseEnemy.h"
#include "CameraController.h"
#include "GlobalVariables.h"
#include "MapChipField.h"
#include "Matrix4x4.h"
#include <algorithm>
#include <cassert>
#include <cmath>
#include <numbers>

// 行列の掛け算ヘルパー関数 (main.cppのコードを参考)
KamataEngine::Matrix4x4 MultiplyMatrix(const KamataEngine::Matrix4x4& a, const KamataEngine::Matrix4x4& b) {
	KamataEngine::Matrix4x4 r = {};
	for (int i = 0; i < 4; ++i) {
		for (int j = 0; j < 4; ++j) {
			float sum = 0.0f;
			for (int k = 0; k < 4; ++k) {
				sum += a.m[i][k] * b.m[k][j];
			}
			r.m[i][j] = sum;
		}
	}
	return r;
}

// アフィン行列作成ヘルパー関数
KamataEngine::Matrix4x4 CreateAffineMatrix(const KamataEngine::Vector3& scale, const KamataEngine::Vector3& rotate, const KamataEngine::Vector3& translation) {
	// 各軸の回転行列、拡縮、平行移動を組み合わせて作成する簡易的な処理
	return MakeAffineMatrix(scale, rotate, translation);
}

// --- GlobalVariables 調整項目の登録 ---
void Player::RegisterGlobalVariables() {
	GlobalVariables* globalVariables = GlobalVariables::GetInstance();
	const std::string groupName = "Player";

	globalVariables->AddItem(groupName, "Acceleration", kAcceleration);
	globalVariables->AddItem(groupName, "Attenuation", kAttenuation);
	globalVariables->AddItem(groupName, "LimitRunSpeed", kLimitRunSpeed);
	globalVariables->AddItem(groupName, "TimeTurn", kTimeTurn);
	globalVariables->AddItem(groupName, "GravityAcceleration", kGravityAcceleration);
	globalVariables->AddItem(groupName, "LimitFallSpeed", kLimitFallSpeed);
	globalVariables->AddItem(groupName, "JumpAcceleration", kJumpAcceleration);

	// ★ 上下左右を個別に登録に変更
	globalVariables->AddItem(groupName, "PaddingTop", kPaddingTop);
	globalVariables->AddItem(groupName, "PaddingBottom", kPaddingBottom);
	globalVariables->AddItem(groupName, "PaddingLeft", kPaddingLeft);
	globalVariables->AddItem(groupName, "PaddingRight", kPaddingRight);

	globalVariables->AddItem(groupName, "AttackVelocity", kAttackVelocity);
}

// --- GlobalVariables 調整項目の反映 ---
void Player::ApplyGlobalVariables() {
	GlobalVariables* globalVariables = GlobalVariables::GetInstance();
	const std::string groupName = "Player";

	kAcceleration = globalVariables->GetFloatValue(groupName, "Acceleration");
	kAttenuation = globalVariables->GetFloatValue(groupName, "Attenuation");
	kLimitRunSpeed = globalVariables->GetFloatValue(groupName, "LimitRunSpeed");
	kTimeTurn = globalVariables->GetFloatValue(groupName, "TimeTurn");
	kGravityAcceleration = globalVariables->GetFloatValue(groupName, "GravityAcceleration");
	kLimitFallSpeed = globalVariables->GetFloatValue(groupName, "LimitFallSpeed");
	kJumpAcceleration = globalVariables->GetFloatValue(groupName, "JumpAcceleration");

	// ★ 上下左右を個別に反映に変更
	kPaddingTop = globalVariables->GetFloatValue(groupName, "PaddingTop");
	kPaddingBottom = globalVariables->GetFloatValue(groupName, "PaddingBottom");
	kPaddingLeft = globalVariables->GetFloatValue(groupName, "PaddingLeft");
	kPaddingRight = globalVariables->GetFloatValue(groupName, "PaddingRight");

	kAttackVelocity = globalVariables->GetFloatValue(groupName, "AttackVelocity");
}

Player::Player() {}

Player::~Player() {
	if (modelHitEffect_) {
		delete modelHitEffect_;
		modelHitEffect_ = nullptr;
	}
	for (auto* effect : hitEffects_) {
		delete effect;
	}
	hitEffects_.clear();

	// 個別モデルの削除
	delete modelPlayerHead_;
	delete modelPlayerBody_;
	delete modelPlayerLeft_;
	delete modelPlayerRight_;
}

void Player::Initialize(KamataEngine::Model* model, KamataEngine::Camera* camera, const KamataEngine::Vector3& position) {
	modelPlayer_ = model;
	camera_ = camera;
	worldTransform_.Initialize();
	worldTransform_.translation_ = position;
	worldTransform_.rotation_.y = std::numbers::pi_v<float> / 2.0f;
	worldTransform_.scale_ = {1.0f, 1.0f, 1.0f};

	// 4つの各部位のOBJファイルを読み取って適用
	modelPlayerHead_ = KamataEngine::Model::CreateFromOBJ("player_head", true);
	modelPlayerBody_ = KamataEngine::Model::CreateFromOBJ("player_body", true);
	modelPlayerLeft_ = KamataEngine::Model::CreateFromOBJ("player_left", true);
	modelPlayerRight_ = KamataEngine::Model::CreateFromOBJ("player_right", true);

	// 各部位のWorldTransform初期化
	worldTransformHead_.Initialize();
	worldTransformBody_.Initialize();
	worldTransformLeft_.Initialize();
	worldTransformRight_.Initialize();

	// 必要に応じて各部位の初期位置(ローカルのオフセット)を設定
	worldTransformBody_.translation_ = {0.0f, 0.0f, 0.0f};  // ルート（体）
	worldTransformHead_.translation_ = {0.0f, 0.0f, 0.0f};  // 体の上
	worldTransformLeft_.translation_ = {-0.0f, 0.2f, 0.0f}; // 体の左
	worldTransformRight_.translation_ = {0.0f, 0.2f, 0.0f}; // 体の右

	lrDirection_ = LRDirection::kRight;
	turnTimer_ = kTimeTurn;
	turnFirstRotationY_ = worldTransform_.rotation_.y;
	velocity_ = {0.0f, 0.0f, 0.0f};
	onGround_ = true;
	isDead_ = false;

	behavior_ = Behavior::kRoot;
	behaviorRequest_ = std::nullopt;
	isKnockbackRequested_ = false;
	knockbackTimer_ = 0.0f;
	attackPhase_ = AttackPhase::kCharge;
	attackParameter_ = 0;

	walkAnimationTimer_ = 0.0f;

	if (modelHitEffect_ == nullptr) {
		modelHitEffect_ = KamataEngine::Model::CreateFromOBJ("hit_effect", true);
		assert(modelHitEffect_ != nullptr);
	}

	for (auto* effect : hitEffects_) {
		delete effect;
	}
	hitEffects_.clear();
}

void Player::KeysPush() {}

void Player::OnCollision() {
	if (IsAttacking()) {
		return;
	}
	isDead_ = true;
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

void Player::BehaviorRootInit() {
	// 通常状態に戻る際、スケールとZ軸回転(傾斜)をリセット
	worldTransform_.scale_ = {1.0f, 1.0f, 1.0f};
	worldTransform_.rotation_.z = 0.0f;
	worldTransformLeft_.rotation_.z = 0.0f;
	worldTransformRight_.rotation_.z = 0.0f;
	walkAnimationTimer_ = 0.0f;
}

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

	const float pi = std::numbers::pi_v<float>;

	// 目標角度の設定
	// 右向き (kRight) : +PI / 2  (+90度)
	// 左向き (kLeft)  : -PI / 2  (-90度)
	float targetRotationY = (lrDirection_ == LRDirection::kRight) ? (pi / 2.0f) : (-pi / 2.0f);

	// 開始角度から目標角度への最短の差分(diff)を求める
	float diff = targetRotationY - turnFirstRotationY_;

	// 差分を -PI ~ +PI の範囲に正規化する (常に180度以内の最短角度で回す)
	while (diff > pi) {
		diff -= 2.0f * pi;
	}
	while (diff < -pi) {
		diff += 2.0f * pi;
	}

	// イージング(EaseOut)を適用して回転
	float ratio = turnTimer_ / kTimeTurn;
	float easeRatio = EaseOut(0.0f, 1.0f, ratio);

	worldTransform_.rotation_.y = turnFirstRotationY_ + diff * easeRatio;

	// 歩きアニメーション処理を追加
	if (onGround_ && std::abs(velocity_.x) > 0.01f) {
		// 移動速度に応じてアニメーション時間を進める
		walkAnimationTimer_ += (std::abs(velocity_.x) / kLimitRunSpeed) * 0.075f;

		// Z軸の角度を使って両手同時に同じ方向へ揺らす（少し揺れる感じにするため、係数を 0.2f 程度に調整）
		float walkTilt = std::sin(walkAnimationTimer_ * 2.0f * pi) * 0.2f;

		// 左右両方に同じ角度を代入することで、真ん中（体）を中心に両手が一緒に傾きます
		worldTransformLeft_.rotation_.z = walkTilt;
		worldTransformRight_.rotation_.z = walkTilt;
	} else {
		// 移動していないか空中にいる時は徐々に直立に戻す
		worldTransformLeft_.rotation_.z *= 0.8f;
		worldTransformRight_.rotation_.z *= 0.8f;
		if (std::abs(worldTransformLeft_.rotation_.z) < 0.001f) {
			worldTransformLeft_.rotation_.z = 0.0f;
			worldTransformRight_.rotation_.z = 0.0f;
			walkAnimationTimer_ = 0.0f;
		}
	}

	// プレイヤー全体のルート行列を計算
	worldTransform_.matWorld_ = MakeAffineMatrix(worldTransform_.scale_, worldTransform_.rotation_, worldTransform_.translation_);
	worldTransform_.TransferMatrix();

	// 各ノードのローカル行列（LocalMatrix）を計算
	KamataEngine::Matrix4x4 localMatrixBody = MakeAffineMatrix(worldTransformBody_.scale_, worldTransformBody_.rotation_, worldTransformBody_.translation_);
	KamataEngine::Matrix4x4 localMatrixHead = MakeAffineMatrix(worldTransformHead_.scale_, worldTransformHead_.rotation_, worldTransformHead_.translation_);
	KamataEngine::Matrix4x4 localMatrixLeft = MakeAffineMatrix(worldTransformLeft_.scale_, worldTransformLeft_.rotation_, worldTransformLeft_.translation_);
	KamataEngine::Matrix4x4 localMatrixRight = MakeAffineMatrix(worldTransformRight_.scale_, worldTransformRight_.rotation_, worldTransformRight_.translation_);

	// 親子関係に基づきワールド行列（WorldMatrix）を計算
	// W_body  = L_body * W_playerRoot
	// W_head  = L_head * W_body
	// W_left  = L_left * W_body
	// W_right = L_right * W_body
	worldTransformBody_.matWorld_ = MultiplyMatrix(localMatrixBody, worldTransform_.matWorld_);
	worldTransformHead_.matWorld_ = MultiplyMatrix(localMatrixHead, worldTransformBody_.matWorld_);
	worldTransformLeft_.matWorld_ = MultiplyMatrix(localMatrixLeft, worldTransformBody_.matWorld_);
	worldTransformRight_.matWorld_ = MultiplyMatrix(localMatrixRight, worldTransformBody_.matWorld_);

	// 各部位の行列を転送
	worldTransformBody_.TransferMatrix();
	worldTransformHead_.TransferMatrix();
	worldTransformLeft_.TransferMatrix();
	worldTransformRight_.TransferMatrix();
}

void Player::BehaviorAttackInit() {
	attackPhase_ = AttackPhase::kCharge;
	attackParameter_ = 0;
	// 攻撃移行時は歩き用のZ軸回転角度をリセット
	worldTransformLeft_.rotation_.z = 0.0f;
	worldTransformRight_.rotation_.z = 0.0f;
}

void Player::BehaviorAttackUpdate() {
	attackParameter_++;
	KamataEngine::Vector3 velocity = {};

	// 傾き角度の設定
	const float kMaxTiltAngle = 0.4f; // 前傾の最大角度（ラジアン）

	// 【修正】符号を反転
	// 右向き：プラス回転(Z軸)で前傾、左向き：マイナス回転(Z軸)で前傾
	float tiltSign = (lrDirection_ == LRDirection::kRight) ? 1.0f : -1.0f;
	float targetTilt = kMaxTiltAngle * tiltSign;

	// スケールは固定
	worldTransform_.scale_ = {1.0f, 1.0f, 1.0f};

	switch (attackPhase_) {
	case AttackPhase::kCharge:
	default: {
		float t = static_cast<float>(attackParameter_) / static_cast<float>(kChargeDuration);
		t = std::min(t, 1.0f);

		// 0から前傾角度まで滑らかに傾斜
		worldTransform_.rotation_.z = EaseOut(0.0f, targetTilt, t);

		if (attackParameter_ >= kChargeDuration) {
			attackPhase_ = AttackPhase::kDash;
			attackParameter_ = 0;

			CreateHitEffect(worldTransform_.translation_);
		}
		break;
	}

	case AttackPhase::kDash: {
		// 突進中は傾きを保持
		worldTransform_.rotation_.z = targetTilt;

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

		// 前傾姿勢から元の直立(0)へ滑らかに戻る
		worldTransform_.rotation_.z = EaseOut(targetTilt, 0.0f, t);

		if (attackParameter_ >= kRecoilDuration) {
			worldTransform_.rotation_.z = 0.0f;
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

	// 攻撃時も同様に各メッシュの親子関係行列を計算
	KamataEngine::Matrix4x4 localMatrixBody = MakeAffineMatrix(worldTransformBody_.scale_, worldTransformBody_.rotation_, worldTransformBody_.translation_);
	KamataEngine::Matrix4x4 localMatrixHead = MakeAffineMatrix(worldTransformHead_.scale_, worldTransformHead_.rotation_, worldTransformHead_.translation_);
	KamataEngine::Matrix4x4 localMatrixLeft = MakeAffineMatrix(worldTransformLeft_.scale_, worldTransformLeft_.rotation_, worldTransformLeft_.translation_);
	KamataEngine::Matrix4x4 localMatrixRight = MakeAffineMatrix(worldTransformRight_.scale_, worldTransformRight_.rotation_, worldTransformRight_.translation_);

	worldTransformBody_.matWorld_ = MultiplyMatrix(localMatrixBody, worldTransform_.matWorld_);
	worldTransformHead_.matWorld_ = MultiplyMatrix(localMatrixHead, worldTransformBody_.matWorld_);
	worldTransformLeft_.matWorld_ = MultiplyMatrix(localMatrixLeft, worldTransformBody_.matWorld_);
	worldTransformRight_.matWorld_ = MultiplyMatrix(localMatrixRight, worldTransformBody_.matWorld_);

	worldTransformBody_.TransferMatrix();
	worldTransformHead_.TransferMatrix();
	worldTransformLeft_.TransferMatrix();
	worldTransformRight_.TransferMatrix();
}

void Player::CreateHitEffect(const KamataEngine::Vector3& position) {
	HitEffect* newEffect = new HitEffect();

	newEffect->worldTransform.Initialize();
	newEffect->worldTransform.translation_ = position;
	newEffect->direction = lrDirection_;

	newEffect->worldTransform.rotation_ = {0.0f, worldTransform_.rotation_.y, 0.0f};
	newEffect->worldTransform.scale_ = {1.0f, 1.0f, 1.0f};

	newEffect->timer = 0;
	newEffect->duration = 15;
	newEffect->isDead = false;

	newEffect->worldTransform.matWorld_ = MakeAffineMatrix(newEffect->worldTransform.scale_, newEffect->worldTransform.rotation_, newEffect->worldTransform.translation_);
	newEffect->worldTransform.TransferMatrix();

	hitEffects_.push_back(newEffect);
}

std::optional<Player::AABB> Player::GetAttackAABB() const {
	if (behavior_ != Behavior::kAttack || attackPhase_ != AttackPhase::kDash) {
		return std::nullopt;
	}

	AABB aabb;
	const auto& pos = worldTransform_.translation_;

	// ★ kWidth, kHeight の代わりに個別の Padding パラメータを適用
	aabb.min = {pos.x - kPaddingLeft, pos.y - kPaddingBottom, pos.z - 0.5f};
	aabb.max = {pos.x + kPaddingRight, pos.y + kPaddingTop, pos.z + 0.5f};

	return aabb;
}

void Player::Update() {
	if (isDead_) {
		return;
	}

	if (KamataEngine::Input::GetInstance()->TriggerKey(DIK_T)) {
		CreateHitEffect(worldTransform_.translation_);
	}

	if (isKnockbackRequested_) {
		behaviorRequest_ = Behavior::kKnockback;
		isKnockbackRequested_ = false;
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
		case Behavior::kKnockback:
			BehaviorKnockbackInitialize();
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
	case Behavior::kKnockback:
		BehaviorKnockbackUpdate();
		break;
	}

	for (auto* effect : hitEffects_) {
		effect->timer++;
		if (effect->timer >= effect->duration) {
			effect->isDead = true;
		} else {
			effect->worldTransform.translation_ = worldTransform_.translation_;
			effect->worldTransform.rotation_ = {0.0f, worldTransform_.rotation_.y, 0.0f};
			effect->worldTransform.scale_ = {1.0f, 1.0f, 1.0f};
			effect->worldTransform.matWorld_ = MakeAffineMatrix(effect->worldTransform.scale_, effect->worldTransform.rotation_, effect->worldTransform.translation_);
			effect->worldTransform.TransferMatrix();
		}
	}

	for (auto it = hitEffects_.begin(); it != hitEffects_.end();) {
		if ((*it)->isDead) {
			delete *it;
			it = hitEffects_.erase(it);
		} else {
			++it;
		}
	}
}

void Player::BehaviorKnockbackInitialize() {
	knockbackTimer_ = 0.0f;
	// ノックバック移行時も回転角度をクリア
	worldTransformLeft_.rotation_.z = 0.0f;
	worldTransformRight_.rotation_.z = 0.0f;
}

void Player::BehaviorKnockbackUpdate() {
	knockbackTimer_ += 1.0f / 60.0f;

	if (knockbackTimer_ < kKnockbackSpeedDuration) {
		float knockbackSpeed = 0.15f;
		if (lrDirection_ == LRDirection::kRight) {
			worldTransform_.translation_.x -= knockbackSpeed;
		} else {
			worldTransform_.translation_.x += knockbackSpeed;
		}
	}

	worldTransform_.matWorld_ = MakeAffineMatrix(worldTransform_.scale_, worldTransform_.rotation_, worldTransform_.translation_);
	worldTransform_.TransferMatrix();

	// ノックバック時も親子関係行列を更新
	KamataEngine::Matrix4x4 localMatrixBody = MakeAffineMatrix(worldTransformBody_.scale_, worldTransformBody_.rotation_, worldTransformBody_.translation_);
	KamataEngine::Matrix4x4 localMatrixHead = MakeAffineMatrix(worldTransformHead_.scale_, worldTransformHead_.rotation_, worldTransformHead_.translation_);
	KamataEngine::Matrix4x4 localMatrixLeft = MakeAffineMatrix(worldTransformLeft_.scale_, worldTransformLeft_.rotation_, worldTransformLeft_.translation_);
	KamataEngine::Matrix4x4 localMatrixRight = MakeAffineMatrix(worldTransformRight_.scale_, worldTransformRight_.rotation_, worldTransformRight_.translation_);

	worldTransformBody_.matWorld_ = MultiplyMatrix(localMatrixBody, worldTransform_.matWorld_);
	worldTransformHead_.matWorld_ = MultiplyMatrix(localMatrixHead, worldTransformBody_.matWorld_);
	worldTransformLeft_.matWorld_ = MultiplyMatrix(localMatrixLeft, worldTransformBody_.matWorld_);
	worldTransformRight_.matWorld_ = MultiplyMatrix(localMatrixRight, worldTransformBody_.matWorld_);

	worldTransformBody_.TransferMatrix();
	worldTransformHead_.TransferMatrix();
	worldTransformLeft_.TransferMatrix();
	worldTransformRight_.TransferMatrix();

	if (knockbackTimer_ >= kKnockbackTotalDuration) {
		behaviorRequest_ = Behavior::kRoot;
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
	float playerLeftX = worldTransform_.translation_.x - kPaddingLeft; // ★変更

	if (playerLeftX < cameraLeftX) {
		worldTransform_.translation_.x = cameraLeftX + kPaddingLeft;
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

void Player::CheckEnemyCollision(const std::list<BaseEnemy*>& enemies) {
	if (isDead_) {
		return;
	}

	AABB aabbPlayer = GetAABB();

	for (BaseEnemy* enemy : enemies) {
		if (!enemy || enemy->IsDead()) {
			continue;
		}

		BaseEnemy::AABB aabbEnemy = enemy->GetAABB();

		if (aabbPlayer.min.x < aabbEnemy.max.x && aabbPlayer.max.x > aabbEnemy.min.x && aabbPlayer.min.y < aabbEnemy.max.y && aabbPlayer.max.y > aabbEnemy.min.y &&
		    aabbPlayer.min.z < aabbEnemy.max.z && aabbPlayer.max.z > aabbEnemy.min.z) {

			if (behavior_ == Behavior::kAttack && attackPhase_ == AttackPhase::kDash) {
				enemy->OnCollision(this);

				if (isKnockbackRequested_) {
					behavior_ = Behavior::kKnockback;
					BehaviorKnockbackInitialize();
					isKnockbackRequested_ = false;
				}
			} else if (behavior_ != Behavior::kKnockback) {
				OnCollision();
			}
			break;
		}
	}
}

KamataEngine::Vector3 Player::CornerPosition(const KamataEngine::Vector3& center, Corner corner) {
	// ★ 左右・上下のそれぞれの値を適用（0.01fのインセット処理は残しています）
	const float insetLeft = kPaddingLeft - 0.01f;
	const float insetRight = kPaddingRight - 0.01f;
	const float insetBottom = kPaddingBottom - 0.01f;
	const float insetTop = kPaddingTop - 0.01f;

	const KamataEngine::Vector3 offsetTable[kNumCorner] = {
	    {+insetRight, -insetBottom, 0.0f}, // kRightBottom
	    {-insetLeft,  -insetBottom, 0.0f}, // kLeftBottom
	    {+insetRight, +insetTop,    0.0f}, // kRightTop
	    {-insetLeft,  +insetTop,    0.0f}  // kLeftTop
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
		float previousTopY = worldTransform_.translation_.y + kPaddingTop;

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
		float previousBottomY = worldTransform_.translation_.y - kPaddingBottom;
		float nextBottomY = nextCenter.y - kPaddingBottom;

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
		info.moveAmount.x = blockLeftX - (worldTransform_.translation_.x + kPaddingRight) - 0.005f;
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
		info.moveAmount.x = blockRightX - (worldTransform_.translation_.x - kPaddingLeft) + 0.005f;
	}
}

void Player::Draw() {
	if (isDead_) {
		return;
	}

	// 各部位の個別モデルをそれぞれの階層構造行列で描画
	if (camera_) {
		if (modelPlayerBody_ && modelPlayerHead_ && modelPlayerLeft_ && modelPlayerRight_) {
			modelPlayerBody_->Draw(worldTransformBody_, *camera_);
			modelPlayerHead_->Draw(worldTransformHead_, *camera_);
			modelPlayerLeft_->Draw(worldTransformLeft_, *camera_);
			modelPlayerRight_->Draw(worldTransformRight_, *camera_);
		} else if (modelPlayer_) {
			// フォールバック処理
			modelPlayer_->Draw(worldTransform_, *camera_);
		}
	}

	if (modelHitEffect_ && camera_ && !hitEffects_.empty()) {
		for (const auto* effect : hitEffects_) {
			modelHitEffect_->Draw(effect->worldTransform, *camera_);
		}
	}
}

Player::AABB Player::GetAABB() const {
	AABB aabb;
	KamataEngine::Vector3 center = worldTransform_.translation_;

	// Z軸（奥行き）はとりあえず左右の平均値等で代用、または固定値にします
	float halfDepth = (kPaddingLeft + kPaddingRight) / 2.0f;

	aabb.min.x = center.x - kPaddingLeft;   // ★変更
	aabb.min.y = center.y - kPaddingBottom; // ★変更
	aabb.min.z = center.z - halfDepth;

	aabb.max.x = center.x + kPaddingRight; // ★変更
	aabb.max.y = center.y + kPaddingTop;   // ★変更（元の +1.5f 固定処理を外す場合はこのまま）
	aabb.max.z = center.z + halfDepth;

	return aabb;
}