#include "Enemy.h"
#include "Matrix4x4.h"

void Enemy::Initialize(KamataEngine::Model* model, KamataEngine::Camera* camera, const KamataEngine::Vector3& position) {
	// 各種ポインタの受け取り
	modelEnemy_ = model;
	camera_ = camera;

	// ワールドトランスフォームの初期化と位置設定
	worldTransform_.Initialize();
	worldTransform_.translation_ = position;

	// 必要に応じてスケールや回転の初期値を設定
	worldTransform_.scale_ = {1.0f, 1.0f, 1.0f};
	worldTransform_.rotation_ = {0.0f, 0.0f, 0.0f};
}

void Enemy::Update() {
	// Player と同様に、アフィン変換行列を計算して matWorld_ に代入
	worldTransform_.matWorld_ = MakeAffineMatrix(worldTransform_.scale_, worldTransform_.rotation_, worldTransform_.translation_);

	// 計算した行列を GPU（定数バッファ）に転送
	worldTransform_.TransferMatrix();
}

void Enemy::Draw() {
	// モデルとカメラが正しく設定されていれば描画する
	if (modelEnemy_ && camera_) {
		KamataEngine::Model::PreDraw();
		modelEnemy_->Draw(worldTransform_, *camera_);
		KamataEngine::Model::PostDraw();
	}
}