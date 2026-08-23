#include "Goal.h"
#include "Matrix4x4.h"

Goal::Goal() {}

Goal::~Goal() {}

void Goal::Initialize(KamataEngine::Model* model, KamataEngine::Camera* camera, const KamataEngine::Vector3& position) {
	model_ = model;
	camera_ = camera;
	worldTransform_.Initialize();

	// ★ マップチップの位置からオフセット（ずらし）を加える場合
	// 例：Y軸に +1.0f 高くし、X軸に +0.5f ずらす
	worldTransform_.translation_ = {position.x + 0.5f, position.y - 0.5f, position.z + 0.0f};

	// スケールと回転の設定
	worldTransform_.scale_ = {0.5f, 0.5f, 0.5f};
	worldTransform_.rotation_ = {0.0f, -1.5708f, 0.0f};

	isActive_ = true;
}

void Goal::Update() {
	if (!isActive_) {
		return;
	}
	worldTransform_.matWorld_ = MakeAffineMatrix(worldTransform_.scale_, worldTransform_.rotation_, worldTransform_.translation_);
	worldTransform_.TransferMatrix();
}

void Goal::Draw() {
	if (!isActive_) {
		return;
	}
	if (model_ && camera_) {
		model_->Draw(worldTransform_, *camera_);
	}
}

Goal::AABB Goal::GetAABB() const {
	if (!isActive_) {
		return AABB{
		    {0.0f, 0.0f, 0.0f},
            {0.0f, 0.0f, 0.0f}
        };
	}

	AABB aabb;
	KamataEngine::Vector3 pos = worldTransform_.translation_;

	// ★ scale_ を考慮して当たり判定のサイズを計算
	float halfWidth = (kWidth * worldTransform_.scale_.x) * 0.5f;
	float halfHeight = (kHeight * worldTransform_.scale_.y) * 0.5f;
	float halfDepth = (kDepth * worldTransform_.scale_.z) * 0.5f;

	aabb.min = {pos.x - halfWidth, pos.y - halfHeight, pos.z - halfDepth};
	aabb.max = {pos.x + halfWidth, pos.y + halfHeight, pos.z + halfDepth};
	return aabb;
}