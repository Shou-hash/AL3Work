#include "Enemy.h"
#include "GlobalVariables.h"
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

void Enemy::Initialize(KamataEngine::Model* model, KamataEngine::Camera* camera, const KamataEngine::Vector3& position) {
	modelEnemy_ = model;
	camera_ = camera;

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
}

void Enemy::OnCollision(Player* player) { (void)player; }

void Enemy::OnDead() {
	if (behavior_ == Behavior::kDead) {
		return;
	}

	behavior_ = Behavior::kDead;
	isCollisionDisabled_ = true;
	deadTimer_ = 0.0f;
	velocity_ = {0.0f, 0.0f, 0.0f};
}

void Enemy::BehaviorRootUpdate() {
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

	worldTransform_.translation_.y += 0.05f;
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