#include "BackgroundBlocks.h"
#include "MapChipField.h"
#include "Matrix4x4.h"
#include <cmath>
#include <numbers>
#include <random>

BackgroundBlocks::~BackgroundBlocks() {
	delete modelBlock_;
	delete modelBlockAbove_;

	for (auto* obj : backgroundObjects_) {
		delete obj;
	}
	backgroundObjects_.clear();
}

void BackgroundBlocks::Initialize(MapChipField* mapChipField) {
	// 背景描画に使うモデルを読み込み
	modelBlock_ = KamataEngine::Model::CreateFromOBJ("block", true);
	modelBlockAbove_ = KamataEngine::Model::CreateFromOBJ("blockAbove", true);

	if (!mapChipField)
		return;

	// ステージ全体の横幅を取得
	float stageWidth = mapChipField->GetNumBlockHorizontal() * 1.0f;

	// 固定シード値で乱数生成器を初期化（常に同じ美しい不規則配置を再現するため）
	std::mt19937 rng(1337);

	// 各種分布の設定
	std::uniform_real_distribution<float> distOffset(-0.8f, 0.8f);   // 位置の不規則なズレ
	std::uniform_real_distribution<float> distRotZ(-0.25f, 0.25f);   // Z軸の微小な傾き(ラジアン)
	std::uniform_real_distribution<float> distFlipY(0.0f, 1.0f);     // Y軸反転用(50%の確率)
	std::uniform_real_distribution<float> distScaleMid(2.2f, 3.8f);  // 中景用スケール
	std::uniform_real_distribution<float> distScaleFar(6.5f, 10.5f); // 遠景用スケール
	std::uniform_real_distribution<float> distZOffset(-0.5f, 0.5f);  // Z軸の揺らぎ（厚み）

	// =========================================================
	// レイヤー 1：近〜中景 (Midground)
	// - プレイヤーの少し奥 (Z = 5.0f ~ 6.0f 周辺)
	// - 合成波と不規則な間隔・回転で自然な地肌を表現
	// =========================================================
	float stepX = 2.0f;
	for (float x = -15.0f; x < stageWidth + 15.0f; x += stepX + distOffset(rng) * 0.5f) {
		LayerObject* obj = new LayerObject();
		obj->transform.Initialize();

		// ★ 3つの波を合体させて自然な起伏を作る（合成波）
		float wave1 = std::sin(x * 0.09f) * 2.8f;
		float wave2 = std::sin(x * 0.23f) * 1.1f;
		float wave3 = std::cos(x * 0.41f) * 0.5f;
		float height = wave1 + wave2 + wave3 + distOffset(rng) * 0.6f;

		// 基礎座標（Z軸にもわずかにランダムな幅を持たせる）
		obj->basePosition = {x + distOffset(rng), height, 5.5f + distZOffset(rng)};

		// ランダムなスケール
		float scaleVal = distScaleMid(rng);
		obj->transform.scale_ = {scaleVal, scaleVal * (1.0f + distOffset(rng) * 0.15f), scaleVal};

		// ランダムな回転（Y軸左右反転 + Z軸傾き）
		float rotY = (distFlipY(rng) > 0.5f) ? std::numbers::pi_v<float> : 0.0f;
		float rotZ = distRotZ(rng);
		obj->transform.rotation_ = {0.0f, rotY, rotZ};

		obj->parallaxFactor = 0.55f; // 手前寄りのスクロール速度
		obj->model = modelBlockAbove_;

		backgroundObjects_.push_back(obj);
	}

	// =========================================================
	// レイヤー 2：遠景 (Background / 巨大シルエット連山)
	// - 奥 (Z = 14.0f ~ 16.0f 周辺)
	// - 大きな連山と影のような巨大構造物
	// =========================================================
	stepX = 5.5f;
	for (float x = -30.0f; x < stageWidth + 30.0f; x += stepX + distOffset(rng) * 1.2f) {
		LayerObject* obj = new LayerObject();
		obj->transform.Initialize();

		// 大規模な緩やかで複雑な起伏
		float wave1 = std::cos(x * 0.04f) * 6.0f;
		float wave2 = std::sin(x * 0.11f) * 2.5f;
		float height = wave1 + wave2 + 3.0f + distOffset(rng) * 1.0f;

		obj->basePosition = {x + distOffset(rng) * 1.5f, height, 15.0f + distZOffset(rng) * 2.0f};

		// 巨大スケール
		float scaleVal = distScaleFar(rng);
		obj->transform.scale_ = {scaleVal * (1.0f + distOffset(rng) * 0.1f), scaleVal, scaleVal};

		// ランダム回転
		float rotY = (distFlipY(rng) > 0.5f) ? std::numbers::pi_v<float> : 0.0f;
		float rotZ = distRotZ(rng) * 0.5f;
		obj->transform.rotation_ = {0.0f, rotY, rotZ};

		obj->parallaxFactor = 0.2f; // 奥でゆっくり動く
		obj->model = modelBlock_;

		backgroundObjects_.push_back(obj);
	}
}

void BackgroundBlocks::Update(const KamataEngine::Camera& camera) {
	for (auto* obj : backgroundObjects_) {
		if (!obj)
			continue;

		// 視差スクロール（Parallax）計算
		obj->transform.translation_.x = obj->basePosition.x + camera.translation_.x * (1.0f - obj->parallaxFactor);
		obj->transform.translation_.y = obj->basePosition.y + camera.translation_.y * (1.0f - obj->parallaxFactor);
		obj->transform.translation_.z = obj->basePosition.z;

		// スケール・回転（Initializeで設定済み）・平行移動からアフィン行列を計算
		obj->transform.matWorld_ = MakeAffineMatrix(obj->transform.scale_, obj->transform.rotation_, obj->transform.translation_);
		obj->transform.TransferMatrix();
	}
}

void BackgroundBlocks::Draw(const KamataEngine::Camera& camera) {
	for (auto* obj : backgroundObjects_) {
		if (!obj || !obj->model)
			continue;

		KamataEngine::Model::PreDraw();
		obj->model->Draw(obj->transform, camera);
		KamataEngine::Model::PostDraw();
	}
}