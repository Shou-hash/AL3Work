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
	velocity_ = {0.0f, 0.0f, 0.0f};
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

void ShieldEnemy::BehaviorDeadUpdate() {
	deadTimer_ += 1.0f / 60.0f;
	float t = std::clamp(deadTimer_ / kDeadDuration, 0.0f, 1.0f);

	worldTransform_.translation_.y += 0.05f;
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
	}

	// 敵本体の行列転送
	worldTransform_.matWorld_ = MakeAffineMatrix(worldTransform_.scale_, worldTransform_.rotation_, worldTransform_.translation_);
	worldTransform_.TransferMatrix();

	// ガードエフェクトの更新
	for (auto* effect : guardEffects_) {
		effect->timer++;
		if (effect->timer >= effect->duration) {
			effect->isDead = true;
		} else {
			float scaleProgress = 1.0f + (static_cast<float>(effect->timer) / effect->duration) * 1.5f;
			effect->worldTransform.scale_ = {scaleProgress, scaleProgress, scaleProgress};
			effect->worldTransform.matWorld_ = MakeAffineMatrix(effect->worldTransform.scale_, effect->worldTransform.rotation_, effect->worldTransform.translation_);
			effect->worldTransform.TransferMatrix();
		}
	}

	// 削除フラグの立ったエフェクトのメモリ解放
	for (auto it = guardEffects_.begin(); it != guardEffects_.end();) {
		if ((*it)->isDead) {
			delete *it;
			it = guardEffects_.erase(it);
		} else {
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

	if (modelGuardEffect_ && camera_) {
		for (const auto* effect : guardEffects_) {
			if (effect) {
				modelGuardEffect_->Draw(effect->worldTransform, *camera_);
			}
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