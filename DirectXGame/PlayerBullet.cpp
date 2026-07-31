#include "PlayerBullet.h"
#include <cassert>

using namespace KamataEngine;

void PlayerBullet::Initialize(Model* model, const Vector3& position) {
	assert(model);
	model_ = model;
	textureHandle_ = TextureManager::Load("white1x1.png");

	worldTransform_.Initialize();
	// 発射時のプレイヤー現在座標をセット
	worldTransform_.translation_ = position;
}

void PlayerBullet::Update() {
	// ★ その場に固定するため座標変更（移動）は行いません

	// 行列の構築と転送
	worldTransform_.matWorld_ = {
	    worldTransform_.scale_.x,
	    0.0f,
	    0.0f,
	    0.0f,
	    0.0f,
	    worldTransform_.scale_.y,
	    0.0f,
	    0.0f,
	    0.0f,
	    0.0f,
	    worldTransform_.scale_.z,
	    0.0f,
	    worldTransform_.translation_.x,
	    worldTransform_.translation_.y,
	    worldTransform_.translation_.z,
	    1.0f};

	worldTransform_.TransferMatrix();
}

void PlayerBullet::Draw(const Camera& camera) 
{
	model_->Draw(worldTransform_, camera, textureHandle_); 
}