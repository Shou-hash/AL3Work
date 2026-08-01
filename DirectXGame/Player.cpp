#include "Player.h"
#include <algorithm>
#include <cassert>
#include <cmath>

using namespace KamataEngine;

// デストラクタ：リスト内のすべての弾を解放する
Player::~Player() {
	for (PlayerBullet* bullet : bullets_) {
		delete bullet;
	}
	bullets_.clear();
}

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

	// 攻撃処理
	Attack();

	// 弾更新（すべての弾を更新）
	for (PlayerBullet* bullet : bullets_) {
		bullet->Update();
	}

	// ImGui表示
	ImGui::Begin("Player");
	ImGui::SliderFloat3("Position", &worldTransform_.translation_.x, -15.0f, 15.0f);
	ImGui::End();
}

// Player.cpp の Attack() 関数を以下のように修正
void Player::Attack() {
	if (input_->TriggerKey(DIK_SPACE)) {
		// 自キャラの座標をコピー（DirectX::XMFLOAT3 から Vector3 に変更）
		Vector3 position = worldTransform_.translation_;

		// 弾を生成し、初期化
		PlayerBullet* newBullet = new PlayerBullet();
		newBullet->Initialize(model_, position);

		// 弾を登録（push_backでリストに追加）
		bullets_.push_back(newBullet);
	}
}

void Player::Draw(KamataEngine::Camera* camera) {
	Model::PreDraw();
	model_->Draw(worldTransform_, *camera, textureHandle_);

	// 弾描画（すべての弾を描画）
	for (PlayerBullet* bullet : bullets_) {
		bullet->Draw(*camera);
	}

	Model::PostDraw();
}