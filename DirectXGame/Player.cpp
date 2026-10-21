#define NOMINMAX
#include "Player.h"
#include "AudioManager.h"
#include "BaseEnemy.h"
#include "BossEnemy.h"
#include "CameraController.h"
#include "Enemy.h"     // ★追加：アイテム状態チェックのため
#include "GameScene.h" // ★ 追加: BlockData 構造体の定義を参照するため
#include "GlobalVariables.h"
#include "LightManager.h" // ★ 追加：ライトグループ設定用
#include "MapChipField.h"
#include "Matrix4x4.h"
#include "PlayerHp.h"    // ★追加：PlayerHpの関数を使用するため
#include "ShieldEnemy.h" // ★追加：アイテム状態チェックのため
#include <algorithm>
#include <cassert>
#include <cmath>
#include <numbers>

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
}

void Player::Initialize(KamataEngine::Model* model, KamataEngine::Camera* camera, const KamataEngine::Vector3& position) {
	modelPlayer_ = model;
	camera_ = camera;
	worldTransform_.Initialize();
	worldTransform_.translation_ = position;
	worldTransform_.rotation_.y = std::numbers::pi_v<float> / 2.0f;
	worldTransform_.scale_ = {1.0f, 1.0f, 1.0f};

	// アニメーション管理の初期化
	animation_.Initialize();

	lrDirection_ = LRDirection::kRight;
	animation_.OnDirectionChanged(worldTransform_.rotation_.y);
	velocity_ = {0.0f, 0.0f, 0.0f};
	prevVelocityY_ = 0.0f; // ★ 追加
	onGround_ = true;
	wasOnGround_ = true; // ★ 追加
	isDead_ = false;

	behavior_ = Behavior::kRoot;
	behaviorRequest_ = std::nullopt;
	isKnockbackRequested_ = false;
	knockbackTimer_ = 0.0f;

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

	// ★変更：すでにノックバック状態（クールタイム中）なら何もしない
	if (behavior_ == Behavior::kKnockback) {
		return;
	}

	// 効果音の再生
	AudioManager::GetInstance()->PlaySE(SEType::kAttack);

	if (playerHp_) {
		playerHp_->DecreaseHp();
		if (playerHp_->IsDead()) {
			behaviorRequest_ = Behavior::kDeath; // ★ ノックバックではなく死亡演出へ移行
		} else {
			behaviorRequest_ = Behavior::kKnockback;
		}
	} else {
		behaviorRequest_ = Behavior::kDeath;
	}
}

void Player::Move() {
	if (isDead_) {
		return;
	}

	velocity_.z = 0.0f;
	if (KamataEngine::Input::GetInstance()->PushKey(DIK_RIGHT)) {
		if (lrDirection_ != LRDirection::kRight) {
			lrDirection_ = LRDirection::kRight;
			animation_.OnDirectionChanged(worldTransform_.rotation_.y);
		}
		if (velocity_.x < 0.0f) {
			velocity_.x *= (1.0f - kAttenuation);
		}
		velocity_.x += kAcceleration;
	} else if (KamataEngine::Input::GetInstance()->PushKey(DIK_LEFT)) {
		if (lrDirection_ != LRDirection::kLeft) {
			lrDirection_ = LRDirection::kLeft;
			animation_.OnDirectionChanged(worldTransform_.rotation_.y);
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
			// ★ 追加: 雲の上でジャンプした時、足元の雲を蹴り下ろす衝撃を与える
			ApplyCloudImpulse(-0.25f);
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
	animation_.BehaviorRootInit();
}

void Player::BehaviorRootUpdate() {
	Move();

	if (KamataEngine::Input::GetInstance()->TriggerKey(DIK_SPACE)) {
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

	// アニメーションおよび部位のワールド行列計算
	animation_.BehaviorRootUpdate(worldTransform_, lrDirection_, velocity_, onGround_, kLimitRunSpeed, kTimeTurn);
}

void Player::BehaviorAttackInit() { animation_.BehaviorAttackInit(); }

void Player::BehaviorAttackUpdate() {
	KamataEngine::Vector3 attackVelocityMove = {};
	bool isHitEffectNeeded = false;
	bool isAttackFinished = false;

	animation_.BehaviorAttackUpdate(worldTransform_, lrDirection_, kAttackVelocity, attackVelocityMove, isHitEffectNeeded, isAttackFinished);

	if (isHitEffectNeeded) {
		CreateHitEffect(worldTransform_.translation_);
	}

	CollisionMapInfo collisionMapInfo;
	collisionMapInfo.moveAmount = attackVelocityMove;
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

	if (isAttackFinished) {
		worldTransform_.rotation_.z = 0.0f;
		behaviorRequest_ = Behavior::kRoot;
	}
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
	// ダッシュ攻撃時の当たり判定
	if (behavior_ == Behavior::kAttack && animation_.GetAttackPhase() == AttackPhase::kDash) {
		AABB aabb;
		const auto& pos = worldTransform_.translation_;
		aabb.min = {pos.x - kPaddingLeft, pos.y - kPaddingBottom, pos.z - 1.0f};
		aabb.max = {pos.x + kPaddingRight, pos.y + kPaddingTop, pos.z + 1.0f};
		return aabb;
	}

	// ハンマースキル時の当たり判定
	if (behavior_ == Behavior::kHammerSkill) {
		float duration = animation_.GetCurrentHammerSkillDuration();
		float progress = (duration > 0.0f) ? (animation_.GetHammerSkillTimer() / duration) : 0.0f;

		// 修正①: 判定発生時間を 0.1f 〜 0.9f に緩和して振り始め・振り終わりもカバー
		if (progress >= 0.1f && progress <= 0.9f) {
			AABB aabb;
			const auto& pos = worldTransform_.translation_;

			// 修正②: Z軸（奥行き）を -2.0f 〜 +2.0f に拡大
			if (lrDirection_ == LRDirection::kRight) {
				aabb.min = {pos.x - 1.5f, pos.y - 2.5f, pos.z - 2.0f};
				aabb.max = {pos.x + 3.0f, pos.y + 2.5f, pos.z + 2.0f};
			} else {
				aabb.min = {pos.x - 3.0f, pos.y - 2.5f, pos.z - 2.0f};
				aabb.max = {pos.x + 1.5f, pos.y + 2.5f, pos.z + 2.0f};
			}
			return aabb;
		}
	}

	return std::nullopt;
}

void Player::Update() {
	if (isDeathAnimationFinished_) {
		return;
	}

	prevVelocityY_ = velocity_.y; // ★ 追加: 移動前のY速度を保持

	// ★ 処刑演出中は、演出専用の更新だけを行う（移動・通常の攻撃・被ダメージは受け付けない）
	if (isExecuting_) {
		UpdateExecution();
		return;
	}

	if (KamataEngine::Input::GetInstance()->TriggerKey(DIK_T)) {
		CreateHitEffect(worldTransform_.translation_);
	}

	if (isKnockbackRequested_) {
		behaviorRequest_ = Behavior::kKnockback;
		isKnockbackRequested_ = false;
	}

	// 通常時、Eキーでハンマースキルが発動
	if (behavior_ == Behavior::kRoot && KamataEngine::Input::GetInstance()->TriggerKey(DIK_E)) {
		behaviorRequest_ = Behavior::kHammerSkill;
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
		case Behavior::kHammerSkill:
			BehaviorHammerSkillInit();
			break;
		case Behavior::kDeath:
			BehaviorDeathInit();
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
	case Behavior::kHammerSkill:
		BehaviorHammerSkillUpdate();
		break;
	case Behavior::kDeath:
		BehaviorDeathUpdate();
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

	// ★ 追加: 空中から接地した瞬間に着地衝撃を雲へ伝える
	if (!wasOnGround_ && onGround_) {
		float landingImpulse = -std::clamp(std::abs(prevVelocityY_) * 0.35f, 0.15f, 0.5f);
		ApplyCloudImpulse(landingImpulse);
	}
	wasOnGround_ = onGround_; // ★ 追加: 前フレームの接地状態を更新
}

void Player::BehaviorDeathInit() {
	isDead_ = true;
	animation_.BehaviorDeathInit(worldTransform_);
}

void Player::BehaviorDeathUpdate() {
	bool isFinished = false;
	animation_.BehaviorDeathUpdate(isFinished);
	if (isFinished) {
		isDeathAnimationFinished_ = true; // 演出終了
	}
}

void Player::BehaviorKnockbackInitialize() {
	knockbackTimer_ = 0.0f;
	animation_.BehaviorKnockbackInit();
}

void Player::BehaviorKnockbackUpdate() {
	knockbackTimer_ += 1.0f / 60.0f;

	// ノックバックによる移動量を格納する変数
	KamataEngine::Vector3 knockbackMove = {0.0f, 0.0f, 0.0f};

	if (knockbackTimer_ < kKnockbackSpeedDuration) {
		float knockbackSpeed = 0.15f;
		if (lrDirection_ == LRDirection::kRight) {
			knockbackMove.x = -knockbackSpeed;
		} else {
			knockbackMove.x = knockbackSpeed;
		}
	}

	// ★ マップ衝突判定を追加して、壁がある場合は移動量を補正する
	CollisionMapInfo collisionMapInfo;
	collisionMapInfo.moveAmount = knockbackMove;
	collisionMapInfo.onGround = onGround_;
	MapCollision(collisionMapInfo);

	// 補正された移動量を座標に適用
	worldTransform_.translation_.x += collisionMapInfo.moveAmount.x;
	worldTransform_.translation_.y += collisionMapInfo.moveAmount.y;
	worldTransform_.translation_.z += collisionMapInfo.moveAmount.z;

	// 接地状態や画面端の衝突も更新
	ApplyGroundingStatus(collisionMapInfo);
	if (collisionMapInfo.ceilingCollision) {
		velocity_.y = 0.0f;
	}
	if (collisionMapInfo.wallCollision) {
		velocity_.x = 0.0f;
	}

	CheckScreenEdgeCollision();

	// 行列計算と転送処理
	animation_.BehaviorKnockbackUpdate(worldTransform_);

	if (knockbackTimer_ >= kKnockbackTotalDuration) {
		behaviorRequest_ = Behavior::kRoot;
	}
}

// ★ ハンマー攻撃初期化
void Player::BehaviorHammerSkillInit() {
	hammerComboIndex_ = 0;
	comboInputRequested_ = false;
	animation_.BehaviorHammerSkillInit(hammerComboIndex_);
	velocity_ = {0.0f, 0.0f, 0.0f};
}

// ★ ハンマー連撃の更新ロジック
void Player::BehaviorHammerSkillUpdate() {
	bool isHitCheck = false;
	bool isCanCombo = false;
	bool isSkillFinished = false;

	// アニメーション更新（Hit判定・コンボ受付判定・動作終了判定を取得）
	animation_.BehaviorHammerSkillUpdate(worldTransform_, lrDirection_, isHitCheck, isCanCombo, isSkillFinished);

	// 連撃受付時間（isCanCombo）中にEキーが押されたら、先行入力として保持
	if (isCanCombo && KamataEngine::Input::GetInstance()->TriggerKey(DIK_E)) {
		comboInputRequested_ = true;
	}

	if (isHitCheck) {
		// 必要に応じて攻撃SEやヒットエフェクト
	}

	// 現在進行中の連撃動作が「最後まで完了」した時に判定
	if (isSkillFinished) {
		// 先行入力があり、かつ最終段でない場合は次のコンボ動作へ移行
		if (comboInputRequested_ && hammerComboIndex_ < 4) {
			hammerComboIndex_++;
			comboInputRequested_ = false;
			animation_.BehaviorHammerSkillInit(hammerComboIndex_);

			// わずかに前進させる（踏み込み処理）
			float stepForward = (lrDirection_ == LRDirection::kRight) ? 0.3f : -0.3f;
			worldTransform_.translation_.x += stepForward;
		} else {
			// 先行入力がない、または最終段が終わった場合は通常状態へ戻る
			behaviorRequest_ = Behavior::kRoot;
		}
	}
}

// ★ 追加: 足元の雲を探して衝撃（上下揺れ）を加える関数
void Player::ApplyCloudImpulse(float impulse) {
	if (!cloudDatas_) {
		return;
	}

	KamataEngine::Vector3 currentCenter = worldTransform_.translation_;
	float playerLeftX = currentCenter.x - kPaddingLeft;
	float playerRightX = currentCenter.x + kPaddingRight;
	float playerBottomY = currentCenter.y - kPaddingBottom;

	for (auto* cloudData : *cloudDatas_) {
		if (!cloudData || !cloudData->transform) {
			continue;
		}

		float cloudMinX = cloudData->basePosition.x - 2.0f;
		float cloudMaxX = cloudData->basePosition.x + 2.0f;
		float cloudTopY = cloudData->transform->translation_.y + 0.5f;

		if (playerRightX >= cloudMinX && playerLeftX <= cloudMaxX) {
			if (std::abs(playerBottomY - cloudTopY) <= 0.35f) {
				cloudData->velocityY += impulse;
			}
		}
	}
}

// ★ 周辺の雲マップチップ情報を取得する関数（X軸当たり判定は普通ブロックの4倍 = ±2.0f）
std::vector<Player::CloudBlockInfo> Player::GetNearbyClouds(const KamataEngine::Vector3& pos) const {
	std::vector<CloudBlockInfo> clouds;

	// ★ 追加: GameScene から雲データ配列がセットされている場合、動的な座標を使用
	if (cloudDatas_) {
		for (auto* cloudData : *cloudDatas_) {
			if (!cloudData || !cloudData->transform) {
				continue;
			}
			const KamataEngine::Vector3& cloudPos = cloudData->transform->translation_;
			if (std::abs(cloudPos.x - pos.x) <= 3.5f && std::abs(cloudPos.y - pos.y) <= 2.5f) {
				CloudBlockInfo cloud;
				cloud.position = cloudPos;
				cloud.minX = cloudPos.x - 2.0f;
				cloud.maxX = cloudPos.x + 2.0f;
				cloud.minY = cloudPos.y - 0.5f;
				cloud.maxY = cloudPos.y + 0.5f;
				clouds.push_back(cloud);
			}
		}
		return clouds;
	}

	if (!mapChipField_) {
		return clouds;
	}

	MapChipField::IndexSet index = mapChipField_->GetMapChipIndexByPosition(pos);
	int32_t y = static_cast<int32_t>(index.y);
	int32_t startX = std::max(0, static_cast<int32_t>(index.x) - 2);
	int32_t endX = std::min(static_cast<int32_t>(mapChipField_->GetNumBlockHorizontal()) - 1, static_cast<int32_t>(index.x) + 2);

	for (int32_t x = startX; x <= endX; ++x) {
		if (mapChipField_->GetMapChipTypeByIndex(static_cast<uint32_t>(x), static_cast<uint32_t>(y)) == MapChipType::kCloud) {
			KamataEngine::Vector3 blockPos = mapChipField_->GetMapChipPositionByIndex(static_cast<uint32_t>(x), static_cast<uint32_t>(y));
			CloudBlockInfo cloud;
			cloud.position = blockPos;
			// ★ X軸幅は普通ブロック(±0.5f)の4倍 (±2.0f)
			cloud.minX = blockPos.x - 2.0f;
			cloud.maxX = blockPos.x + 2.0f;
			cloud.minY = blockPos.y - 0.5f;
			cloud.maxY = blockPos.y + 0.5f;
			clouds.push_back(cloud);
		}
	}
	return clouds;
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
			bool isDownPressed = KamataEngine::Input::GetInstance()->PushKey(DIK_DOWN);

			bool hitBlock = (chipLeftBottom == MapChipType::kBlock || chipRightBottom == MapChipType::kBlock);

			bool hitBlockFall = (!isDownPressed) && (chipLeftBottom == MapChipType::kBlockFall || chipRightBottom == MapChipType::kBlockFall);

			// ★ 追加: 雲 (kCloud) の接地判定（X軸幅は普通ブロックの4倍 = ±2.0f）
			bool hitCloud = false;
			float playerLeftX = currentCenter.x - kPaddingLeft;
			float playerRightX = currentCenter.x + kPaddingRight;
			float playerBottomY = currentCenter.y - kPaddingBottom + offset.y;

			std::vector<CloudBlockInfo> cloudsLeft = GetNearbyClouds(leftBottomPos);
			std::vector<CloudBlockInfo> cloudsRight = GetNearbyClouds(rightBottomPos);
			std::vector<CloudBlockInfo> clouds = cloudsLeft;
			clouds.insert(clouds.end(), cloudsRight.begin(), cloudsRight.end());

			for (const auto& cloud : clouds) {
				if (playerRightX >= cloud.minX && playerLeftX <= cloud.maxX) {
					if (std::abs(playerBottomY - cloud.maxY) <= 0.1f) {
						hitCloud = true;
						break;
					}
				}
			}

			// ★ 修正: hitCloudのチェックを追加して雲の上で接地状態が維持されるように修正
			if (!hitBlock && !hitBlockFall && !hitCloud) {
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

	// ★ダッシュ攻撃またはハンマースキル中なら、広範囲の攻撃用AABBを取得する
	std::optional<AABB> attackAABB = GetAttackAABB();

	for (BaseEnemy* enemy : enemies) {
		if (!enemy || enemy->IsDead()) {
			continue;
		}

		BaseEnemy::AABB aabbEnemy = enemy->GetAABB();

		// ★攻撃中であれば攻撃用AABBで判定し、それ以外ならプレイヤー本体のAABBで判定する
		AABB aabbToCheck = attackAABB.has_value() ? attackAABB.value() : GetAABB();

		if (aabbToCheck.min.x < aabbEnemy.max.x && aabbToCheck.max.x > aabbEnemy.min.x && aabbToCheck.min.y < aabbEnemy.max.y && aabbToCheck.max.y > aabbEnemy.min.y &&
		    aabbToCheck.min.z < aabbEnemy.max.z && aabbToCheck.max.z > aabbEnemy.min.z) {

			if (IsAttacking()) {
				enemy->OnCollision(this);

				if (isKnockbackRequested_) {
					behavior_ = Behavior::kKnockback;
					BehaviorKnockbackInitialize();
					isKnockbackRequested_ = false;
				}
			} else if (behavior_ != Behavior::kKnockback) { // ★変更：ノックバック中（クールタイム中）でなければ処理
				// ★ 被弾時効果音の再生
				BossEnemy* boss = dynamic_cast<BossEnemy*>(enemy);
				if (boss) {
					if (boss->GetState() == BossState::kAttackDash) {
						AudioManager::GetInstance()->PlaySE(SEType::kBossDash);
					} else if (boss->GetState() == BossState::kAttackSlam || boss->GetState() == BossState::kAttackSlamCharge) {
						AudioManager::GetInstance()->PlaySE(SEType::kBossAttack);
					} else {
						AudioManager::GetInstance()->PlaySE(SEType::kAttack);
					}
				} else {
					AudioManager::GetInstance()->PlaySE(SEType::kAttack);
				}

				// ★変更：HPを一個減らし、ノックバック状態へ移行させることで連続ダメージを防ぐ
				if (playerHp_) {
					playerHp_->DecreaseHp();
					if (playerHp_->IsDead()) {
						behaviorRequest_ = Behavior::kDeath; // ★ 死亡演出へ
					} else {
						behaviorRequest_ = Behavior::kKnockback;
					}
				} else {
					behaviorRequest_ = Behavior::kDeath;
				}
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

	// ★ 追加: 雲 (kCloud) との上方向当たり判定（X軸幅は普通ブロックの4倍 = ±2.0f）
	float playerLeftX = nextCenter.x - kPaddingLeft;
	float playerRightX = nextCenter.x + kPaddingRight;
	float previousTopY = worldTransform_.translation_.y + kPaddingTop;

	std::vector<CloudBlockInfo> cloudsLeft = GetNearbyClouds(positionsNew[kLeftTop]);
	std::vector<CloudBlockInfo> cloudsRight = GetNearbyClouds(positionsNew[kRightTop]);
	std::vector<CloudBlockInfo> clouds = cloudsLeft;
	clouds.insert(clouds.end(), cloudsRight.begin(), cloudsRight.end());

	for (const auto& cloud : clouds) {
		if (playerRightX >= cloud.minX && playerLeftX <= cloud.maxX) {
			float cloudBottomY = cloud.minY;
			if (previousTopY <= cloudBottomY + 0.05f) {
				info.ceilingCollision = true;
				info.moveAmount.y = cloudBottomY - previousTopY - 0.005f;
				return;
			}
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

	// 下キー(DIK_DOWN)が押されている場合は kBlockFall を判定対象から外す
	bool isDownPressed = KamataEngine::Input::GetInstance()->PushKey(DIK_DOWN);

	bool hitBlock = (chipLeftBottom == MapChipType::kBlock || chipRightBottom == MapChipType::kBlock);

	bool hitBlockFall = (!isDownPressed) && (chipLeftBottom == MapChipType::kBlockFall || chipRightBottom == MapChipType::kBlockFall);

	if (hitBlock || hitBlockFall) {
		Corner targetCorner = (chipLeftBottom == MapChipType::kBlock || chipLeftBottom == MapChipType::kBlockFall) ? kLeftBottom : kRightBottom;
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

	// ★ 追加: 雲 (kCloud) との下方向当たり判定（X軸幅は普通ブロックの4倍 = ±2.0f）
	float playerLeftX = nextCenter.x - kPaddingLeft;
	float playerRightX = nextCenter.x + kPaddingRight;
	float previousBottomY = worldTransform_.translation_.y - kPaddingBottom;
	float nextBottomY = nextCenter.y - kPaddingBottom;

	std::vector<CloudBlockInfo> cloudsLeft = GetNearbyClouds(positionsNew[kLeftBottom]);
	std::vector<CloudBlockInfo> cloudsRight = GetNearbyClouds(positionsNew[kRightBottom]);
	std::vector<CloudBlockInfo> clouds = cloudsLeft;
	clouds.insert(clouds.end(), cloudsRight.begin(), cloudsRight.end());

	for (const auto& cloud : clouds) {
		if (playerRightX >= cloud.minX && playerLeftX <= cloud.maxX) {
			float cloudTopY = cloud.maxY;
			if (previousBottomY >= cloudTopY - 0.2f && nextBottomY <= cloudTopY) {
				info.onGround = true;
				info.moveAmount.y = cloudTopY - previousBottomY;
				return;
			}

			if (nextBottomY < cloudTopY && previousBottomY >= cloudTopY) {
				info.onGround = true;
				info.moveAmount.y = cloudTopY - previousBottomY;
				return;
			}
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

	// ★ 追加: 雲 (kCloud) との右方向当たり判定（雲の左端 cloud.minX = pos.x - 2.0f）
	float playerTopY = nextCenter.y + kPaddingTop;
	float playerBottomY = nextCenter.y - kPaddingBottom;
	float previousRightX = worldTransform_.translation_.x + kPaddingRight;

	std::vector<CloudBlockInfo> cloudsTop = GetNearbyClouds(positionsNew[kRightTop]);
	std::vector<CloudBlockInfo> cloudsBottom = GetNearbyClouds(positionsNew[kRightBottom]);
	std::vector<CloudBlockInfo> clouds = cloudsTop;
	clouds.insert(clouds.end(), cloudsBottom.begin(), cloudsBottom.end());

	for (const auto& cloud : clouds) {
		if (playerTopY >= cloud.minY && playerBottomY <= cloud.maxY) {
			if (previousRightX <= cloud.minX + 0.05f) {
				info.wallCollision = true;
				info.moveAmount.x = cloud.minX - previousRightX - 0.005f;
				return;
			}
		}
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
		return;
	}

	// ★ 追加: 雲 (kCloud) との左方向当たり判定（雲の右端 cloud.maxX = pos.x + 2.0f）
	float playerTopY = nextCenter.y + kPaddingTop;
	float playerBottomY = nextCenter.y - kPaddingBottom;
	float previousLeftX = worldTransform_.translation_.x - kPaddingLeft;

	std::vector<CloudBlockInfo> cloudsTop = GetNearbyClouds(positionsNew[kLeftTop]);
	std::vector<CloudBlockInfo> cloudsBottom = GetNearbyClouds(positionsNew[kLeftBottom]);
	std::vector<CloudBlockInfo> clouds = cloudsTop;
	clouds.insert(clouds.end(), cloudsBottom.begin(), cloudsBottom.end());

	for (const auto& cloud : clouds) {
		if (playerTopY >= cloud.minY && playerBottomY <= cloud.maxY) {
			if (previousLeftX >= cloud.maxX - 0.05f) {
				info.wallCollision = true;
				info.moveAmount.x = cloud.maxX - previousLeftX + 0.005f;
				return;
			}
		}
	}
}

// ★ プレイヤーのモデルにマップタイルなどと同じライトグループ（kWorld）を設定する
void Player::ApplyLightManager(const LightManager& lightManager) {
	// プレイヤー自身が持つモデル（本体・ハンマー・ヒットエフェクト）
	lightManager.ApplyToModel(modelPlayer_, LightManager::GroupType::kWorld);
	lightManager.ApplyToModel(modelHammer_, LightManager::GroupType::kWorld);
	lightManager.ApplyToModel(modelHitEffect_, LightManager::GroupType::kWorld);

	// 頭・体・左手・右手の各部位のモデルは PlayerAnimation が持っているため、
	// PlayerAnimation に ApplyLightManager 関数があればそちらでも設定する（無い場合は何もしない）
	[&lightManager](auto& animation) {
		if constexpr (requires { animation.ApplyLightManager(lightManager); }) {
			animation.ApplyLightManager(lightManager);
		}
	}(animation_);
}

void Player::Draw() {
	if (isDeathAnimationFinished_) {
		return; // 演出が終わったら非表示
	}

	// 各部位の個別モデルをそれぞれの階層構造行列で描画
	animation_.Draw(camera_, modelHammer_);
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
	aabb.max.y = center.y + kPaddingTop;   // ★変更
	aabb.max.z = center.z + halfDepth;

	return aabb;
}

// ★ 処刑演出の開始：指定した位置・向きに固定する
void Player::BeginExecution(const KamataEngine::Vector3& position, LRDirection direction) {
	isExecuting_ = true;
	executionSwinging_ = false;
	executionHitPending_ = false;
	executionHitDone_ = false;

	behavior_ = Behavior::kRoot;
	behaviorRequest_ = std::nullopt;
	isKnockbackRequested_ = false;
	velocity_ = {0.0f, 0.0f, 0.0f};

	worldTransform_.translation_ = position;

	// 向きが変わる場合は、通常の移動時と同じように振り向きアニメーションを開始する
	if (lrDirection_ != direction) {
		lrDirection_ = direction;
		animation_.OnDirectionChanged(worldTransform_.rotation_.y);
	}
	animation_.BehaviorRootInit();
}

// ★ 処刑演出の終了：通常の操作に戻る
void Player::EndExecution() {
	isExecuting_ = false;
	executionSwinging_ = false;
	executionHitPending_ = false;
	velocity_ = {0.0f, 0.0f, 0.0f};

	// 次の更新で通常状態の初期化（ハンマーを消す等）が行われる
	behavior_ = Behavior::kRoot;
	behaviorRequest_ = Behavior::kRoot;
}

// ★ ハンマーの連撃を 1 回開始する
bool Player::StartExecutionSwing(uint32_t comboIndex) {
	if (!isExecuting_ || executionSwinging_) {
		return false;
	}
	executionSwinging_ = true;
	executionHitPending_ = false;
	executionHitDone_ = false;
	animation_.BehaviorHammerSkillInit(comboIndex);
	return true;
}

// ★ 処刑演出中の更新（位置は固定のまま、ハンマーのスイングと待機ポーズだけを更新する）
void Player::UpdateExecution() {
	velocity_ = {0.0f, 0.0f, 0.0f};

	if (executionSwinging_) {
		bool isHitCheck = false;
		bool isCanCombo = false;
		bool isSkillFinished = false;
		animation_.BehaviorHammerSkillUpdate(worldTransform_, lrDirection_, isHitCheck, isCanCombo, isSkillFinished);

		// 当たりのタイミングを 1 スイングにつき 1 回だけ通知する（アニメーションの当たり判定、または動作の半分を過ぎた時点）
		float duration = animation_.GetCurrentHammerSkillDuration();
		float progress = (duration > 0.0f) ? (animation_.GetHammerSkillTimer() / duration) : 0.0f;
		if (!executionHitDone_ && (isHitCheck || progress >= 0.5f)) {
			executionHitPending_ = true;
			executionHitDone_ = true;
		}

		if (isSkillFinished) {
			executionSwinging_ = false;
			animation_.BehaviorRootInit();
		}
	} else {
		// 待機ポーズ（その場で立っている）
		animation_.BehaviorRootUpdate(worldTransform_, lrDirection_, velocity_, true, kLimitRunSpeed, kTimeTurn);
	}
}