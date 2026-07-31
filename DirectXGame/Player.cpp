#include "Player.h"
#include <algorithm> // std::clamp を使用するために必要
#include <cassert>

using namespace KamataEngine;

Player::~Player() {}

void Player::Initialize(KamataEngine::Model* model, uint32_t textureHandle) {
	// プレイヤー用のモデルとテクスチャを設定
	assert(model);
	model_ = model;
	textureHandle_ = textureHandle;

	// ワールドトランスフォームの初期化
	worldTransform_.Initialize();

	// シングルトンインスタンスを取得する
	input_ = Input::GetInstance();
}

void Player::Update() {
	// キャラクターの移動ベクトル
	Vector3 move = {0, 0, 0};

	// キャラクターの移動速さ
	const float kCharacterSpeed = 0.2f;

	// 押した方向で移動ベクトルを変更 (左右)
	if (input_->PushKey(DIK_LEFT)) {
		move.x -= kCharacterSpeed;
	} else if (input_->PushKey(DIK_RIGHT)) {
		move.x += kCharacterSpeed;
	}

	// 押した方向で移動ベクトルを変更 (上下)
	if (input_->PushKey(DIK_UP)) {
		move.y += kCharacterSpeed;
	} else if (input_->PushKey(DIK_DOWN)) {
		move.y -= kCharacterSpeed;
	}

	// 座標移動
	worldTransform_.translation_.x += move.x;
	worldTransform_.translation_.y += move.y;
	worldTransform_.translation_.z += move.z;

	// --- 移動限界値の設定 (クランプ処理) ---
	// 移動制限の限界値 (画面端より少し手前に設定)
	const float kMoveLimitX = 34.0f; // 左右の限界値 (例: -12.0f ～ 12.0f)
	const float kMoveLimitY = 18.0f;  // 上下の限界値 (例: -6.0f ～ 6.0f)

	// std::clamp(値, 下限値, 上限値) で範囲内に収める
	worldTransform_.translation_.x = std::clamp(worldTransform_.translation_.x, -kMoveLimitX, kMoveLimitX);
	worldTransform_.translation_.y = std::clamp(worldTransform_.translation_.y, -kMoveLimitY, kMoveLimitY);

	// アフィン変換行列の作成（拡大・回転・平行移動の合成）
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

	// 定数バッファへ転送
	worldTransform_.TransferMatrix();

	// キャラクターの座標をスライダーで表示・操作する処理 (ImGui)
	ImGui::Begin("Player");
	ImGui::SliderFloat3("Position", &worldTransform_.translation_.x, -15.0f, 15.0f);
	ImGui::End();
}

void Player::Draw(KamataEngine::Camera* camera) {
	Model::PreDraw();
	// メンバ変数ではなく、引数で渡されたカメラを使用する
	model_->Draw(worldTransform_, *camera, textureHandle_);

	model_->PostDraw();
}