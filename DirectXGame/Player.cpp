#include "Player.h"
#include "Skydome.h"
#include <numbers>

void Player::Initialize(KamataEngine::Model* modelPlayer_, KamataEngine::Camera* camera, const KamataEngine::Vector3& position) 
{
	worldTransform_.Initialize();
	worldTransform_.translation_ = position;

	worldTransform_.rotation_.y = std::numbers::pi_v<float> / 2.0f;
}

void Player::Draw() {
	if (modelPlayer_) {
		KamataEngine::Model::PreDraw();
		modelPlayer_->Draw(worldTransform_,*camera_);
		KamataEngine::Model::PostDraw();
	}
}