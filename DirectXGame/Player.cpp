#include "Player.h"
#include <algorithm>
#include <cassert>
#include <cmath>

using namespace KamataEngine;

Player::~Player() { delete bullet_; }

void Player::Initialize(KamataEngine::Model* model, uint32_t textureHandle) {
	assert(model);
	model_ = model;
	textureHandle_ = textureHandle;

	worldTransform_.Initialize();
	input_ = Input::GetInstance();
}

void Player::Update() {
	// --- キャラクター旋回処理 ---
	const float kRotSpeed = 0.02f;
	if (input_->PushKey(DIK_A)) {
		worldTransform_.rotation_.y += kRotSpeed;
	} else if (input_->PushKey(DIK_D)) {
		worldTransform_.rotation_.y -= kRotSpeed;
	}

	// --- キャラクター移動処理 ---
	Vector3 move = {0, 0, 0};
	const float kCharacterSpeed = 0.2f;

	if (input_->PushKey(DIK_LEFT)) {
		move.x -= kCharacterSpeed;
	} else if (input_->PushKey(DIK_RIGHT)) {
		move.x += kCharacterSpeed;
	}

	if (input_->PushKey(DIK_UP)) {
		move.y += kCharacterSpeed;
	} else if (input_->PushKey(DIK_DOWN)) {
		move.y -= kCharacterSpeed;
	}

	worldTransform_.translation_.x += move.x;
	worldTransform_.translation_.y += move.y;
	worldTransform_.translation_.z += move.z;

	// 移動限界値の設定
	const float kMoveLimitX = 34.0f;
	const float kMoveLimitY = 18.0f;
	worldTransform_.translation_.x = std::clamp(worldTransform_.translation_.x, -kMoveLimitX, kMoveLimitX);
	worldTransform_.translation_.y = std::clamp(worldTransform_.translation_.y, -kMoveLimitY, kMoveLimitY);

	// 行列の再計算
	float cosY = std::cos(worldTransform_.rotation_.y);
	float sinY = std::sin(worldTransform_.rotation_.y);

	worldTransform_.matWorld_ = {
	    worldTransform_.scale_.x * cosY,
	    0.0f,
	    worldTransform_.scale_.x * -sinY,
	    0.0f,
	    0.0f,
	    worldTransform_.scale_.y,
	    0.0f,
	    0.0f,
	    worldTransform_.scale_.z * sinY,
	    0.0f,
	    worldTransform_.scale_.z * cosY,
	    0.0f,
	    worldTransform_.translation_.x,
	    worldTransform_.translation_.y,
	    worldTransform_.translation_.z,
	    1.0f};

	worldTransform_.TransferMatrix();

	// ★ プレイヤーの移動・行列更新が完全に終わった後に攻撃処理を実行する
	Attack();

	// 弾の更新処理
	if (bullet_) {
		bullet_->Update();
	}

	// ImGui表示
	ImGui::Begin("Player");
	ImGui::SliderFloat3("Position", &worldTransform_.translation_.x, -15.0f, 15.0f);
	ImGui::End();
}

void Player::Attack() {
	if (input_->TriggerKey(DIK_SPACE)) {
		PlayerBullet* newBullet = new PlayerBullet();
		// 発射した瞬間のプレイヤー位置を弾に一度だけ渡す
		newBullet->Initialize(model_, worldTransform_.translation_);

		if (bullet_) {
			delete bullet_;
		}

		bullet_ = newBullet;
	}
}

void Player::Draw(KamataEngine::Camera* camera) {
	Model::PreDraw();
	model_->Draw(worldTransform_, *camera, textureHandle_);

	// 【重要】Drawの中でUpdateを呼んでいたのを修正し、Drawを呼び出します
	if (bullet_) {
		bullet_->Draw(*camera);
	}

	Model::PostDraw();
}