#include "PlayerHp.h"
#include "Matrix4x4.h"
#include <cmath>

void PlayerHp::Initialize(KamataEngine::Model* model, int32_t maxHp) {
	model_ = model;
	maxHp_ = maxHp;
	currentHp_ = maxHp;

	// ★ 既存のポインタのメモリを解放してからクリアする
	for (auto* transform : worldTransforms_) {
		delete transform;
	}
	worldTransforms_.clear();

	for (int32_t i = 0; i < maxHp_; ++i) {
		// ★ new で動的に生成
		KamataEngine::WorldTransform* transform = new KamataEngine::WorldTransform();
		transform->Initialize();
		transform->scale_ = {0.5f, 0.5f, 0.5f};
		worldTransforms_.push_back(transform);
	}
}

void PlayerHp::Update(const KamataEngine::Vector3& cameraPos) {
	float offsetX = -6.5f;
	float offsetY = 3.0f;
	float offsetZ = 10.0f;

	for (int32_t i = 0; i < maxHp_; ++i) {
		// ★ ポインタなので -> に変更
		worldTransforms_[i]->translation_.x = cameraPos.x + offsetX + (i * 0.7f);
		worldTransforms_[i]->translation_.y = cameraPos.y + offsetY;
		worldTransforms_[i]->translation_.z = cameraPos.z + offsetZ;

		worldTransforms_[i]->matWorld_ = MakeAffineMatrix(worldTransforms_[i]->scale_, worldTransforms_[i]->rotation_, worldTransforms_[i]->translation_);
		worldTransforms_[i]->TransferMatrix();
	}
}

void PlayerHp::Draw(const KamataEngine::Camera& camera) {
	if (!model_)
		return;

	for (int32_t i = 0; i < currentHp_; ++i) {
		// ★ worldTransforms_[i] はポインタなので、デリファレンス (*) をつけて参照を渡す
		model_->Draw(*worldTransforms_[i], camera);
	}
}