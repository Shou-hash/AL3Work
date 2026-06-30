#include "DeathParticles.h"
#include "Matrix4x4.h"
#include <cmath>

void DeathParticles::Initialize(KamataEngine::Model* model, KamataEngine::Camera* viewProjection, const KamataEngine::Vector3& position) {
	// メンバ変数の初期化
	model_ = model;
	viewProjection_ = viewProjection;

	objectColor_.Initialize();
	objectColor_.SetColor({1.0f, 1.0f, 1.0f, 1.0f});

	// 各パーティクルの初期化
	for (uint32_t i = 0; i < kNumParticles; ++i) {
		// ワールド変換の初期化
		worldTransforms_[i].Initialize();
		worldTransforms_[i].translation_ = position;

		// 8方向に移動させるための速度計算
		// 基本となる右方向のベクトル（速さ、0、0）※今回は速さを 0.1f と仮定
		float speed = 0.1f;
		KamataEngine::Vector3 baseVelocity = {speed, 0.0f, 0.0f};

		// 2πラジアン（360度）を8分割した角度をN個分、Z軸まわりに回転させる
		float angle = (2.0f * 3.14159265f / kNumParticles) * i;

		// Z軸まわりの回転行列を適用して速度ベクトルを得る
		velocities_[i].x = baseVelocity.x * std::cos(angle) - baseVelocity.y * std::sin(angle);
		velocities_[i].y = baseVelocity.x * std::sin(angle) + baseVelocity.y * std::cos(angle);
		velocities_[i].z = 0.0f;
	}


	// 終了フラグとカウンターの初期化
	isFinished_ = false;
	counter_ = 0;

	isInitialized_ = true;
}

void DeathParticles::Update() {
	// 終了しているなら何もしない
	if (isFinished_) {
		return;
	}

	// カウンターを1進める
	counter_++;
	// 1秒（60フレーム）経ったら終了
	if (counter_ >= kDuration) {
		isFinished_ = true;
		return; // 終了したら処理を抜ける
	}

	if (!isInitialized_) {
		return;
	}

	// カウンターが0のとき1.0f、kDurationに近づくほど0.0fになる計算
	float alpha = 1.0f - (static_cast<float>(counter_) / static_cast<float>(kDuration));
	objectColor_.SetColor({1.0f, 1.0f, 1.0f, alpha});

	for (uint32_t i = 0; i < kNumParticles; ++i) {
		// 座標に速度を加算して移動
		worldTransforms_[i].translation_.x += velocities_[i].x;
		worldTransforms_[i].translation_.y += velocities_[i].y;
		worldTransforms_[i].translation_.z += velocities_[i].z;

		// スケールや回転が初期化された状態を維持、または徐々に小さくするなどの変化を加えても良い
		// 今回は初期状態のスケール {1,1,1} と回転 {0,0,0} を明示的に行列に適用します
		KamataEngine::Matrix4x4 affineMatrix = MakeAffineMatrix(worldTransforms_[i].scale_, worldTransforms_[i].rotation_, worldTransforms_[i].translation_);
		worldTransforms_[i].matWorld_ = affineMatrix;
		worldTransforms_[i].TransferMatrix();
	}
}

void DeathParticles::Draw() {
	// 終了しているなら何もしない
	if (model_ == nullptr || viewProjection_ == nullptr) {
		return;
	}

	// 終了している場合も描画しない
	if (isFinished_) {
		return;
	}

	for (uint32_t i = 0; i < kNumParticles; ++i) {
		// モデルの描画（カメラポインタを正しく渡す）
		KamataEngine::Model::PreDraw();
		model_->Draw(worldTransforms_[i], *viewProjection_, &objectColor_);
		KamataEngine::Model::PostDraw();
	}
}