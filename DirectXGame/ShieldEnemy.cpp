#define _USE_MATH_DEFINES
#include "ShieldEnemy.h"
#include "MapChipField.h" // ★追加：インクルード
#include "Matrix4x4.h"
#include "Player.h"
#include <algorithm>
#include <cmath>
#include <numbers>

// 静的メンバ変数の実体定義
KamataEngine::Model* ShieldEnemy::modelGuardEffect_ = nullptr;

ShieldEnemy::~ShieldEnemy() {
	for (auto* effect : guardEffects_) {
		delete effect;
	}
	guardEffects_.clear();
}

void ShieldEnemy::StaticFinalize() {
	if (modelGuardEffect_) {
		delete modelGuardEffect_;
		modelGuardEffect_ = nullptr;
	}
}

// ★変更：引数に mapChipField を追加
void ShieldEnemy::Initialize(KamataEngine::Model* model, KamataEngine::Camera* camera, const KamataEngine::Vector3& position, MapChipField* mapChipField) {
	modelShieldEnemy_ = model;
	camera_ = camera;
	mapChipField_ = mapChipField; // ★追加

	worldTransform_.Initialize();
	worldTransform_.translation_ = position;
	worldTransform_.scale_ = {1.0f, 1.0f, 1.0f};
	worldTransform_.rotation_ = {0.0f, -90.0f * (std::numbers::pi_v<float> / 180.0f), 0.0f};

	velocity_ = {-kWalkspeed, 0.0f, 0.0f};
	lrDirection_ = ShieldEnemyLRDirection::kLeft;

	walkTimer_ = 0.0f;
	guardTimer_ = 0.0f;
	behavior_ = Behavior::kRoot;
	isDead_ = false;
	isCollisionDisabled_ = false;
	deadTimer_ = 0.0f;
	isItemSpawnRequested_ = false; // ★追加

	// ★ ハンマー連撃用の初期化
	comboHitCount_ = 0;
	lastHammerSwingId_ = 0;
	comboStunTimer_ = 0.0f;
	hitReactionTimer_ = 0.0f;
	hitPushDirX_ = 1.0f;

	// ガード用モデル (ring) のロード
	if (modelGuardEffect_ == nullptr) {
		modelGuardEffect_ = KamataEngine::Model::CreateFromOBJ("ring", true);
	}

	for (auto* effect : guardEffects_) {
		delete effect;
	}
	guardEffects_.clear();
}

void ShieldEnemy::OnCollision(Player* player) {
	if (!player) {
		return;
	}

	// 死亡中または判定無効時はスキップ
	if (isDead_ || isCollisionDisabled_) {
		return;
	}

	// プレイヤーが攻撃中の場合
	if (player->IsAttacking()) {

		// 【ガード判定】向かい合っているか確認（自右&敵左 OR 自左&敵右）
		bool isFacing = (player->GetLRDirection() == LRDirection::kRight && lrDirection_ == ShieldEnemyLRDirection::kLeft) ||
		                (player->GetLRDirection() == LRDirection::kLeft && lrDirection_ == ShieldEnemyLRDirection::kRight);

		if (isFacing) {
			// 1. ガードエフェクト生成
			CreateGuardEffect();

			// 2. プレイヤー側にノックバックを要求 (リクエストフラグを立てる)
			player->RequestKnockback();

			// 3. 敵側のビヘイビアをガード（のけぞり）に切り替え
			behavior_ = Behavior::kGuard;
			guardTimer_ = 0.0f;

			return;
		}

		// 正面以外（背面など）からの攻撃であればデス演出へ移行
		OnDead();
	}
}

void ShieldEnemy::CreateGuardEffect() {
	GuardEffect* newEffect = new GuardEffect();

	newEffect->worldTransform.Initialize();
	newEffect->worldTransform.CreateConstBuffer(); // GPU用のバッファを作成

	// 敵の目の前に少しオフセットしてエフェクトを配置
	float offset = (lrDirection_ == ShieldEnemyLRDirection::kLeft) ? -0.5f : 0.5f;
	newEffect->worldTransform.translation_ = {worldTransform_.translation_.x + offset, worldTransform_.translation_.y, worldTransform_.translation_.z};
	newEffect->worldTransform.rotation_ = worldTransform_.rotation_;
	newEffect->worldTransform.scale_ = {1.0f, 1.0f, 1.0f};

	newEffect->timer = 0;
	newEffect->duration = 15;
	newEffect->isDead = false;

	// 行列を計算して転送
	newEffect->worldTransform.matWorld_ = MakeAffineMatrix(newEffect->worldTransform.scale_, newEffect->worldTransform.rotation_, newEffect->worldTransform.translation_);
	newEffect->worldTransform.TransferMatrix();

	guardEffects_.push_back(newEffect);
}

void ShieldEnemy::OnDead() {
	if (behavior_ == Behavior::kDead) {
		return;
	}

	behavior_ = Behavior::kDead;
	isCollisionDisabled_ = true;
	deadTimer_ = 0.0f;
	isItemSpawnRequested_ = true; // ★追加：アイテム生成要求

	// 放物線上に打ち上げる初期速度を設定（X軸は現在の向きを維持、Y軸に上方向の力を加える）
	float jumpPowerY = 0.25f;
	float speedX = (lrDirection_ == ShieldEnemyLRDirection::kRight) ? 0.03f : -0.03f;
	velocity_ = {speedX, jumpPowerY, 0.0f};
}

void ShieldEnemy::BehaviorRootUpdate() {
	// ★追加：ステージ端側（足元にブロックがない、または前方に壁がある）での反転判定
	if (mapChipField_) {
		float checkOffsetX = (lrDirection_ == ShieldEnemyLRDirection::kLeft) ? -0.6f : 0.6f;

		KamataEngine::Vector3 frontPos = worldTransform_.translation_;
		frontPos.x += checkOffsetX;

		KamataEngine::Vector3 frontDownPos = frontPos;
		frontDownPos.y -= 1.0f; // 足元の座標

		MapChipType frontType = mapChipField_->GetMapChipTypeByPosition(frontPos);
		MapChipType frontDownType = mapChipField_->GetMapChipTypeByPosition(frontDownPos);

		if (frontType == MapChipType::kBlock || frontDownType == MapChipType::kBlank) {
			// 反転処理
			if (lrDirection_ == ShieldEnemyLRDirection::kLeft) {
				lrDirection_ = ShieldEnemyLRDirection::kRight;
				velocity_.x = kWalkspeed;
			} else {
				lrDirection_ = ShieldEnemyLRDirection::kLeft;
				velocity_.x = -kWalkspeed;
			}
		}
	}

	worldTransform_.translation_.x += velocity_.x;
	worldTransform_.translation_.y += velocity_.y;
	worldTransform_.translation_.z += velocity_.z;

	walkTimer_ += 1.0f / 60.0f;

	float param = std::sin(2.0f * std::numbers::pi_v<float> * walkTimer_ / kWalkMotionTime);
	float degree = kWalkMotionAnglestart + (kWalkMotionAngleEnd - kWalkMotionAnglestart) * (param + 1.0f) / 2.0f;

	worldTransform_.rotation_.x = 0.0f;
	// 左右の向き(lrDirection_)に基づいた基準の角度に対して歩行アニメーションのブレを追加
	float baseAngleY = (lrDirection_ == ShieldEnemyLRDirection::kLeft) ? -90.0f : 90.0f;
	worldTransform_.rotation_.y = (baseAngleY + degree) * (std::numbers::pi_v<float> / 180.0f);
	worldTransform_.scale_ = {1.0f, 1.0f, 1.0f};
}

// ガード（のけぞり）アニメーション
void ShieldEnemy::BehaviorGuardUpdate() {
	guardTimer_ += 1.0f / 60.0f;
	float t = std::clamp(guardTimer_ / kGuardDuration, 0.0f, 1.0f);

	float angleParam = std::sin(t * std::numbers::pi_v<float> * 1.5f);
	float leanAngle = angleParam * 35.0f * (std::numbers::pi_v<float> / 180.0f);
	worldTransform_.rotation_.x = leanAngle;

	// アニメーション終了後に通常歩行へ戻る
	if (t >= 1.0f) {
		worldTransform_.rotation_.x = 0.0f;
		behavior_ = Behavior::kRoot;
	}
}

// ★ ハンマーの連撃を 1 回受ける（ハンマーは盾のガードを崩す）
bool ShieldEnemy::ReceiveHammerHit(uint32_t swingId, float pushDirX) {
	// 死亡中、または同じスイングでの多重ヒットは無視する
	if (isDead_ || behavior_ == Behavior::kDead || swingId == lastHammerSwingId_) {
		return false;
	}
	lastHammerSwingId_ = swingId;

	// 連撃の 1 発目なら、その場で止まる状態へ移行する
	if (behavior_ != Behavior::kComboHit) {
		behavior_ = Behavior::kComboHit;
		comboHitCount_ = 0;
	}

	++comboHitCount_;
	comboStunTimer_ = 0.0f;
	hitReactionTimer_ = 0.0f;
	hitPushDirX_ = pushDirX;

	// 規定回数に達したら、大きく吹き飛んで倒れる（フィニッシュ）
	if (comboHitCount_ >= kHammerHitsToDefeat) {
		OnDead();
		velocity_ = {pushDirX * 0.06f, 0.30f, 0.0f};
	}
	return true;
}

// ★ 連撃を受けている間の更新（歩かずにその場で止まり、打撃のたびにのけぞって潰れる）
void ShieldEnemy::BehaviorComboHitUpdate() {
	comboStunTimer_ += 1.0f / 60.0f;
	hitReactionTimer_ += 1.0f / 60.0f;

	float t = std::clamp(hitReactionTimer_ / kHitReactionDuration, 0.0f, 1.0f);
	float impact = (1.0f - t) * (1.0f - t);

	// 向きは止めたまま、のけぞりと潰れで打撃を表現する
	worldTransform_.rotation_.x = 0.0f;
	worldTransform_.rotation_.y = ((lrDirection_ == ShieldEnemyLRDirection::kLeft) ? -90.0f : 90.0f) * (std::numbers::pi_v<float> / 180.0f);
	worldTransform_.rotation_.z = -hitPushDirX_ * 0.35f * impact;
	worldTransform_.scale_ = {1.0f + 0.25f * impact, 1.0f - 0.25f * impact, 1.0f + 0.25f * impact};
	worldTransform_.translation_.x += hitPushDirX_ * 0.04f * impact; // 打撃の瞬間だけ少し押される

	// 連撃が途切れたら、通常の歩行へ戻る
	if (comboStunTimer_ >= kComboRecoverTime) {
		behavior_ = Behavior::kRoot;
		comboHitCount_ = 0;
		worldTransform_.rotation_.z = 0.0f;
		worldTransform_.scale_ = {1.0f, 1.0f, 1.0f};
	}
}

void ShieldEnemy::BehaviorDeadUpdate() {
	deadTimer_ += 1.0f / 60.0f;
	float t = std::clamp(deadTimer_ / kDeadDuration, 0.0f, 1.0f);

	// 重力を適用して放物線運動（落下）させる
	float gravity = 0.012f;
	velocity_.y -= gravity;

	// 座標更新
	worldTransform_.translation_.x += velocity_.x;
	worldTransform_.translation_.y += velocity_.y;
	worldTransform_.translation_.z += velocity_.z;

	// 回転演出は維持
	worldTransform_.rotation_.y += 0.1f;

	float scale = 1.0f - t;
	worldTransform_.scale_ = {scale, scale, scale};

	if (t >= 1.0f) {
		isDead_ = true;
	}
}

void ShieldEnemy::Update() {
	// 各ビヘイビアの更新
	switch (behavior_) {
	case Behavior::kRoot:
		BehaviorRootUpdate();
		break;
	case Behavior::kGuard:
		BehaviorGuardUpdate();
		break;
	case Behavior::kDead:
		BehaviorDeadUpdate();
		break;
	case Behavior::kComboHit:
		BehaviorComboHitUpdate();
		break;
	}

	worldTransform_.matWorld_ = MakeAffineMatrix(worldTransform_.scale_, worldTransform_.rotation_, worldTransform_.translation_);
	worldTransform_.TransferMatrix();

	// エフェクトの更新
	for (auto it = guardEffects_.begin(); it != guardEffects_.end();) {
		GuardEffect* effect = *it;
		effect->timer++;
		if (effect->timer >= effect->duration) {
			delete effect;
			it = guardEffects_.erase(it);
		} else {
			effect->worldTransform.matWorld_ = MakeAffineMatrix(effect->worldTransform.scale_, effect->worldTransform.rotation_, effect->worldTransform.translation_);
			effect->worldTransform.TransferMatrix();
			++it;
		}
	}
}

void ShieldEnemy::Draw() {
	if (isDead_) {
		return;
	}

	if (modelShieldEnemy_ && camera_) {
		modelShieldEnemy_->Draw(worldTransform_, *camera_);
	}

	// ガードエフェクト描画
	for (GuardEffect* effect : guardEffects_) {
		if (modelGuardEffect_ && camera_) {
			modelGuardEffect_->Draw(effect->worldTransform, *camera_);
		}
	}
}

ShieldEnemy::AABB ShieldEnemy::GetAABB() const {
	if (isCollisionDisabled_) {
		return AABB{
		    {0.0f, 0.0f, 0.0f},
            {0.0f, 0.0f, 0.0f}
        };
	}

	AABB aabb;
	const auto& pos = worldTransform_.translation_;
	aabb.min = {pos.x - 0.5f, pos.y - 0.5f, pos.z - 0.5f};
	aabb.max = {pos.x + 0.5f, pos.y + 0.5f, pos.z + 0.5f};
	return aabb;
}
