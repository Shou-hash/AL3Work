#include "Skydome.h"
#include "Matrix4x4.h"

void Skydome::Initialize(KamataEngine::Model* model, KamataEngine::Camera* camera) {

	model_ = model;
	camera_ = camera;

	worldTransform_.Initialize();

	worldTransform_.scale_ = {500.0f, 500.0f, 500.0f};
}

void Skydome::Update() {
	worldTransform_.matWorld_ = MakeAffineMatrix(worldTransform_.scale_, worldTransform_.rotation_, worldTransform_.translation_);

	worldTransform_.TransferMatrix();
}

void Skydome::Draw() {
	if (model_) {
		KamataEngine::Model::PreDraw();
		model_->Draw(worldTransform_, *camera_);
		KamataEngine::Model::PostDraw();
	}
}