#include "HitEffect.h"
#include "Matrix4x4.h"
#include <algorithm>
#include <numbers> // 円周率 std::numbers::pi_v を使用するため追加
#include <random>
#include <cmath>

// 静的メンバ変数の実体定義
KamataEngine::Model* HitEffect::model_ = nullptr;
KamataEngine::Camera* HitEffect::camera_ = nullptr;

void HitEffect::Initialize(const KamataEngine::Vector3& position) {
	// メルセンヌ・ツイスタによる乱数生成
	std::random_device seedGenerator;
	std::mt19937 randomEngine(seedGenerator());

	// 通常エフェクトのトランスフォーム初期化
	worldTransform_.Initialize();
	worldTransform_.translation_ = position;
	worldTransform_.scale_ = {0.1f, 0.1f, 0.1f}; // 最初は小さく開始
	worldTransform_.rotation_ = {0.0f, 0.0f, 0.0f};

	// 2. 楕円リングエフェクトのトランスフォーム初期化
	worldTransformRing_.Initialize();
	worldTransformRing_.translation_ = position;
	worldTransformRing_.scale_ = {0.1f, 0.02f, 0.1f}; // 楕円用に初期から少し薄く潰す

	// 角度（回転）用の乱数生成（0 〜 2π [360度ラジアン]）
	float pi = std::numbers::pi_v<float>;
	std::uniform_real_distribution<float> rotDist(0.0f, 2.0f * pi);

	// Y軸まわりの回転角度を完全にランダム（0〜360度）にする
	float randomRotY = rotDist(randomEngine);

	// X軸・Z軸にも少しだけランダムな傾きを加えて立体感を出す（例: -15度 〜 +15度のランダム）
	std::uniform_real_distribution<float> tiltDist(-15.0f * (pi / 180.0f), 15.0f * (pi / 180.0f));
	float randomRotX = tiltDist(randomEngine);
	float randomRotZ = tiltDist(randomEngine);

	// 楕円トランスフォームの回転にランダムな角度を設定
	worldTransformRing_.rotation_ = {randomRotX, randomRotY, randomRotZ};

	// 【既存】スケール用の乱数生成
	// 楕円リングの広がりスケール倍率を 3.0f 〜 5.5f の間でランダムにする
	std::uniform_real_distribution<float> ringScaleDist(4.0f, 6.5f);
	ringMaxScale_ = ringScaleDist(randomEngine);

	// 通常球エフェクトの広がりスケール倍率を 1.5f 〜 2.5f の間でランダムにする
	std::uniform_real_distribution<float> normalScaleDist(1.5f, 2.5f);
	normalMaxScale_ = normalScaleDist(randomEngine);

	timer_ = 0.0f;
	isFinished_ = false;
}

void HitEffect::Update() {
	if (isFinished_) {
		return;
	}

	// タイマー進行
	timer_ += 1.0f / 60.0f;
	float t = std::clamp(timer_ / kDuration, 0.0f, 1.0f);

	// スプレッド（Spread / 拡がり方）のイージング
	float easeT = 1.0f - std::pow(1.0f - t, 4.0f);

	// 通常エフェクトのスケール計算
	float normalScale = easeT * normalMaxScale_;
	worldTransform_.scale_ = {normalScale, normalScale, normalScale};

	// 楕円エフェクトのスケール計算
	float ringHorizontalScale = easeT * ringMaxScale_;
	float ringVerticalScale = 0.05f; // 潰れ率はキープ
	worldTransformRing_.scale_ = {ringHorizontalScale, ringVerticalScale, ringHorizontalScale};

	// 後半にかけて綺麗に消えるように、アルファ（不透明度）を落とします
	float alpha = 1.0f - t; // リニアな減衰（スライド基準）
	// より滑らかにするためにアルファにも二次曲線などの減衰をかけるとさらにリッチになります
	float alphaRing = 1.0f - (t * t); // 楕円リングは余韻を残しつつ消えるように

	color_.w = alpha; // w成分(A)に適用
	colorRing_.w = alphaRing;

	// 寿命に達したら終了
	if (t >= 1.0f) {
		isFinished_ = true;
	}

	// アフィン行列の更新と転送
	worldTransform_.matWorld_ = MakeAffineMatrix(worldTransform_.scale_, worldTransform_.rotation_, worldTransform_.translation_);
	worldTransform_.TransferMatrix();

	worldTransformRing_.matWorld_ = MakeAffineMatrix(worldTransformRing_.scale_, worldTransformRing_.rotation_, worldTransformRing_.translation_);
	worldTransformRing_.TransferMatrix();
}

void HitEffect::Draw() {
	if (isFinished_) {
		return;
	}

	if (model_ && camera_) {
		// 通常エフェクト描画
		model_->Draw(worldTransform_, *camera_);

		// 楕円エフェクト描画
		model_->Draw(worldTransformRing_, *camera_);
	}
}