#include "Player.h"
#include <cassert>

using namespace KamataEngine;

Player::~Player() {
	
}

void Player::Initialize(KamataEngine::Model* model, uint32_t textureHandle) {
	// プレイヤー用のモデルとテクスチャを設定
	assert(model);
	model_ = model;
	textureHandle_ = textureHandle;

	// ワールドトランスフォームの初期化
	worldTransform_.Initialize();
}

void Player::Update()
{
	worldTransform_.TransferMatrix();
}

void Player::Draw(KamataEngine::Camera* camera)
{
	model_->Draw(worldTransform_, *camera, textureHandle_); 
}