#include "Enemy.h"
#include "GlobalVariables.h"
#include "MapChipField.h" // ★追加：インクルード
#include "Matrix4x4.h"
#define _USE_MATH_DEFINES
#include <algorithm>
#include <cmath>
#include <numbers>

void Enemy::RegisterGlobalVariables() {
	GlobalVariables* globalVariables = GlobalVariables::GetInstance();
	const std::string groupName = "Enemy";

	globalVariables->AddItem(groupName, "Walkspeed", kWalkspeed);
	globalVariables->AddItem(groupName, "WalkMotionAnglestart", kWalkMotionAnglestart);
	globalVariables->AddItem(groupName, "WalkMotionAngleEnd", kWalkMotionAngleEnd);
	globalVariables->AddItem(groupName, "WalkMotionTime", kWalkMotionTime);
	globalVariables->AddItem(groupName, "DeadDuration", kDeadDuration);
}

void Enemy::ApplyGlobalVariables() {
	GlobalVariables* globalVariables = GlobalVariables::GetInstance();
	const std::string groupName = "Enemy";

	kWalkspeed = globalVariables->GetFloatValue(groupName, "Walkspeed");
	kWalkMotionAnglestart = globalVariables->GetFloatValue(groupName, "WalkMotionAnglestart");
	kWalkMotionAngleEnd = globalVariables->GetFloatValue(groupName, "WalkMotionAngleEnd");
	kWalkMotionTime = globalVariables->GetFloatValue(groupName, "WalkMotionTime");
	kDeadDuration = globalVariables->GetFloatValue(groupName, "DeadDuration");
}

// ★変更：引数に mapChipField を追加
void Enemy::Initialize(KamataEngine::Model* model, KamataEngine::Camera* camera, const KamataEngine::Vector3& position, MapChipField* mapChipField) {
	modelEnemy_ = model;
	camera_ = camera;
	mapChipField_ = mapChipField; // ★追加

	worldTransform_.Initialize();
	worldTransform_.translation_ = position;
	worldTransform_.scale_ = {1.0f, 1.0f, 1.0f};
	worldTransform_.rotation_ = {0.0f, -90.0f * (std::numbers::pi_v<float> / 180.0f), 0.0f};

	velocity_ = {-kWalkspeed, 0, 0};
	walkTimer_ = 0.0f;

	behavior_ = Behavior::kRoot;
	isDead_ = false;
	isCollisionDisabled_ = false;
	deadTimer_ = 0.0f;
	isItemSpawnRequested_ = false; // ★追加
}

void Enemy::OnCollision(Player* player) { (void)player; }

void Enemy::OnDead() {
	if (behavior_ == Behavior::kDead) {
		return;
	}

	behavior_ = Behavior::kDead;
	isCollisionDisabled_ = true;
	deadTimer_ = 0.0f;
	isItemSpawnRequested_ = true; // ★追加：アイテム生成を要求

	// 放物線上に打ち上げる初期速度を設定（X軸は現在の向きを維持、Y軸に上方向の力を加える）
	float jumpPowerY = 0.25f;
	float speedX = (velocity_.x > 0.0f) ? 0.03f : -0.03f;
	if (velocity_.x == 0.0f) {
		speedX = -0.03f;
	}
	velocity_ = {speedX, jumpPowerY, 0.0f};
}

void Enemy::BehaviorRootUpdate() {
	// ★追加：ステージ端側（足元にブロックがない、または前方に壁がある）での反転判定
	if (mapChipField_) {
		// 進行方向の少し先をチェックするためのオフセット（敵の移動方向に応じて調整）
		float checkOffsetX = (velocity_.x > 0.0f) ? 0.6f : -0.6f;

		KamataEngine::Vector3 frontPos = worldTransform_.translation_;
		frontPos.x += checkOffsetX;

		KamataEngine::Vector3 frontDownPos = frontPos;
		frontDownPos.y -= 1.0f; // 足元の座標

		// 前方に壁がある、または進行方向の足元が空白（床がない）なら反転
		MapChipType frontType = mapChipField_->GetMapChipTypeByPosition(frontPos);
		MapChipType frontDownType = mapChipField_->GetMapChipTypeByPosition(frontDownPos);

		if (frontType == MapChipType::kBlock || frontDownType == MapChipType::kBlank) {
			velocity_.x *= -1.0f; // 移動方向を逆にする

			// 進行方向に応じてモデルの向き（Y軸回転）を反転させる
			if (velocity_.x > 0.0f) {
				worldTransform_.rotation_.y = 90.0f * (std::numbers::pi_v<float> / 180.0f);
			} else {
				worldTransform_.rotation_.y = -90.0f * (std::numbers::pi_v<float> / 180.0f);
			}
		}
	}

	worldTransform_.translation_.x += velocity_.x;
	worldTransform_.translation_.y += velocity_.y;
	worldTransform_.translation_.z += velocity_.z;

	walkTimer_ += 1.0f / 60.0f;

	float param = std::sin(2.0f * std::numbers::pi_v<float> * walkTimer_ / kWalkMotionTime);
	float degree = kWalkMotionAnglestart + (kWalkMotionAngleEnd - kWalkMotionAnglestart) * (param + 1.0f) / 2.0f;

	worldTransform_.rotation_.z = degree * (std::numbers::pi_v<float> / 270.0f);
	worldTransform_.scale_ = {1.0f, 1.0f, 1.0f};
}

void Enemy::BehaviorDeadUpdate() {
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

void Enemy::Update() {
	switch (behavior_) {
	case Behavior::kRoot:
		BehaviorRootUpdate();
		break;
	case Behavior::kDead:
		BehaviorDeadUpdate();
		break;
	}

	worldTransform_.matWorld_ = MakeAffineMatrix(worldTransform_.scale_, worldTransform_.rotation_, worldTransform_.translation_);
	worldTransform_.TransferMatrix();
}

void Enemy::Draw() {
	if (isDead_) {
		return;
	}

	if (modelEnemy_ && camera_) {
		modelEnemy_->Draw(worldTransform_, *camera_);
	}
}

Enemy::AABB Enemy::GetAABB() const {
	// ★変更：判定無効化時または死亡演出中の場合は判定を無効化
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