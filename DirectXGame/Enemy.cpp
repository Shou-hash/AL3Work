#include "Enemy.h"

void Enemy::Initialize(KamataEngine::Model* model, uint32_t textureHandle) {
	// 引数で受け取ったモデルとテクスチャハンドルを保持
	model_ = model;
	textureHandle_ = textureHandle;

	// ワールドトランスフォームの初期化
	worldTransform_.Initialize();

	// 初期座標の設定 (例: 画面奥の Y=2, Z=40 の位置)
	worldTransform_.translation_ = {0.0f, 2.0f, 40.0f};
}

void Enemy::Update() {
	// Z座標の移動処理（Z座標だけに速度を加算）
	worldTransform_.translation_.z += speed_;

	// 2. 行列（matWorld_）
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

	// 行列の転送
	worldTransform_.TransferMatrix();
}

void Enemy::Draw(const KamataEngine::Camera& camera)
{
	// 3Dモデルの描画
	model_->Draw(worldTransform_, camera, textureHandle_);
}

Enemy::~Enemy() {}