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

	// 固定シード値で乱数生成器を初期化
	std::mt19937 rng(1337);

	// 各種分布の設定
	std::uniform_real_distribution<float> distOffset(-0.8f, 0.8f);
	std::uniform_real_distribution<float> distRotZ(-0.25f, 0.25f);
	std::uniform_real_distribution<float> distFlipY(0.0f, 1.0f);

	// ★ Z座標を奥に下げた分、スケールを大きめに調整
	std::uniform_real_distribution<float> distScaleMid(4.0f, 6.5f);   // 中景用スケール
	std::uniform_real_distribution<float> distScaleFar(12.0f, 20.0f); // 遠景用スケール
	std::uniform_real_distribution<float> distZOffset(-1.0f, 1.0f);   // Z軸の揺らぎ

	// =========================================================
	// レイヤー 1：近〜中景 (Midground)
	// - 修正前: Z = 5.5f 周辺 ➔ 修正後: Z = 20.0f 周辺
	// =========================================================
	float stepX = 2.5f;
	for (float x = -20.0f; x < stageWidth + 20.0f; x += stepX + distOffset(rng) * 0.5f) {
		LayerObject* obj = new LayerObject();
		obj->transform.Initialize();

		float wave1 = std::sin(x * 0.09f) * 2.8f;
		float wave2 = std::sin(x * 0.23f) * 1.1f;
		float wave3 = std::cos(x * 0.41f) * 0.5f;
		float height = wave1 + wave2 + wave3 + distOffset(rng) * 0.6f;

		// ★ Z座標を 20.0f 付近に設定
		obj->basePosition = {x + distOffset(rng), height, 20.0f + distZOffset(rng)};

		float scaleVal = distScaleMid(rng);
		obj->transform.scale_ = {scaleVal, scaleVal * (1.0f + distOffset(rng) * 0.15f), scaleVal};

		float rotY = (distFlipY(rng) > 0.5f) ? std::numbers::pi_v<float> : 0.0f;
		float rotZ = distRotZ(rng);
		obj->transform.rotation_ = {0.0f, rotY, rotZ};

		// ★ 奥に下がったため視差を小さく調整 (0.55 ➔ 0.35)
		obj->parallaxFactor = 0.35f;
		obj->model = modelBlockAbove_;

		backgroundObjects_.push_back(obj);
	}

	// =========================================================
	// レイヤー 2：遠景 (Background / 巨大シルエット連山)
	// - 修正前: Z = 15.0f 周辺 ➔ 修正後: Z = 45.0f 周辺
	// =========================================================
	stepX = 8.0f;
	for (float x = -40.0f; x < stageWidth + 40.0f; x += stepX + distOffset(rng) * 1.2f) {
		LayerObject* obj = new LayerObject();
		obj->transform.Initialize();

		float wave1 = std::cos(x * 0.04f) * 6.0f;
		float wave2 = std::sin(x * 0.11f) * 2.5f;
		float height = wave1 + wave2 + 3.0f + distOffset(rng) * 1.0f;

		// ★ Z座標を 45.0f 付近に設定
		obj->basePosition = {x + distOffset(rng) * 1.5f, height, 45.0f + distZOffset(rng) * 3.0f};

		float scaleVal = distScaleFar(rng);
		obj->transform.scale_ = {scaleVal * (1.0f + distOffset(rng) * 0.1f), scaleVal, scaleVal};

		float rotY = (distFlipY(rng) > 0.5f) ? std::numbers::pi_v<float> : 0.0f;
		float rotZ = distRotZ(rng) * 0.5f;
		obj->transform.rotation_ = {0.0f, rotY, rotZ};

		// ★ より奥でゆっくり動くよう視差を調整 (0.2 ➔ 0.1)
		obj->parallaxFactor = 0.1f;
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

void BackgroundBlocks::SetLightGroups(const KamataEngine::LightGroup* midGroup, const KamataEngine::LightGroup* farGroup) {
	// レイヤー 1（中景）は modelBlockAbove_、レイヤー 2（遠景）は modelBlock_ を使用している
	if (modelBlockAbove_) {
		modelBlockAbove_->SetLightGroup(midGroup);
	}
	if (modelBlock_) {
		modelBlock_->SetLightGroup(farGroup);
	}
}
