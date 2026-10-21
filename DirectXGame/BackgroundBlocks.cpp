#include "BackgroundBlocks.h"
#include "MapChipField.h"
#include "Matrix4x4.h"
#include <algorithm>
#include <cmath>
#include <cstdlib>
#include <fstream>
#include <numbers>
#include <random>
#include <sstream>

namespace {

// ★ 背景座標の起点（ワールドX, ワールドY）
constexpr float kBackgroundOriginX = -40.0f;
constexpr float kBackgroundOriginY = -10.0f;

} // namespace

BackgroundBlocks::~BackgroundBlocks() {
	delete modelBlock_;
	delete modelBlockAbove_;
	delete modelCloud_; // ★ 追加: 雲モデルの解放
	for (int i = 0; i < kNumTileModels; ++i) {
		delete tileModels_[i];
	}

	for (auto* obj : backgroundObjects_) {
		delete obj;
	}
	backgroundObjects_.clear();
}

void BackgroundBlocks::Initialize(MapChipField* mapChipField, const std::string& csvPath) {
	(void)csvPath; // ★ CSVを使わない自動生成のみにするため未使用引数化

	// 背景描画に使うモデルを読み込み
	modelBlock_ = KamataEngine::Model::CreateFromOBJ("block", true);
	modelBlockAbove_ = KamataEngine::Model::CreateFromOBJ("blockAbove", true);
	modelCloud_ = KamataEngine::Model::CreateFromOBJ("cloud3", true);

	if (!mapChipField) {
		return;
	}

	// ステージ全体の横幅を取得（マップチップで実際に使われている横幅）
	stageWidth_ = static_cast<float>(mapChipField->GetUsedBlockHorizontal());
	if (stageWidth_ < 50.0f) {
		stageWidth_ = 100.0f; // ★ 背景が途切れないよう最小横幅を確保
	}

	// 固定シード値で乱数生成器を初期化
	rng_.seed(1337);

	// 各種分布の設定
	std::uniform_real_distribution<float> distOffset(-0.8f, 0.8f);
	std::uniform_real_distribution<float> distRotZ(-0.25f, 0.25f);
	std::uniform_real_distribution<float> distFlipY(0.0f, 1.0f);

	std::uniform_real_distribution<float> distScaleMid(4.0f, 6.5f);   // 中景用スケール
	std::uniform_real_distribution<float> distScaleFar(12.0f, 20.0f); // 遠景用スケール
	std::uniform_real_distribution<float> distZOffset(-1.0f, 1.0f);   // Z軸の揺らぎ

	// ★ 雲用の不規則パラメータ分布設定
	std::uniform_real_distribution<float> distCloudHeight(1.0f, 18.0f);                          // ★ Y軸ポジション（全不規則）
	std::uniform_real_distribution<float> distCloudRotY(0.0f, 2.0f * std::numbers::pi_v<float>); // ★ Y軸ローテーション（全不規則 0〜2π）
	std::uniform_real_distribution<float> distScaleX(3.0f, 8.0f);                                // ★ X軸スケール
	std::uniform_real_distribution<float> distScaleY(2.0f, 5.0f);                                // ★ Y軸スケール
	std::uniform_real_distribution<float> distScaleZ(3.0f, 8.0f);                                // ★ Z軸スケール
	std::uniform_real_distribution<float> distMoveSpeed(0.02f, 0.07f);                           // ★ 右から左への移動速度（全不規則）

	// =========================================================
	// レイヤー 1：近〜中景 (Midground)
	// =========================================================
	float stepX = 2.5f;
	for (float x = -30.0f; x < stageWidth_ + 30.0f; x += stepX + distOffset(rng_) * 0.5f) {
		LayerObject* obj = new LayerObject();
		obj->transform.Initialize();

		float wave1 = std::sin(x * 0.09f) * 2.8f;
		float wave2 = std::sin(x * 0.23f) * 1.1f;
		float wave3 = std::cos(x * 0.41f) * 0.5f;
		float height = wave1 + wave2 + wave3 + distOffset(rng_) * 0.6f;

		obj->basePosition = {x + distOffset(rng_), height, 20.0f + distZOffset(rng_)};

		float scaleVal = distScaleMid(rng_);
		obj->transform.scale_ = {scaleVal, scaleVal * (1.0f + distOffset(rng_) * 0.15f), scaleVal};

		float rotY = (distFlipY(rng_) > 0.5f) ? std::numbers::pi_v<float> : 0.0f;
		float rotZ = distRotZ(rng_);
		obj->transform.rotation_ = {0.0f, rotY, rotZ};

		obj->parallaxFactor = 0.35f;
		obj->model = modelBlockAbove_;
		obj->cullRange = 80.0f;

		backgroundObjects_.push_back(obj);
	}

	// =========================================================
	// レイヤー 2：遠景 (Background / 巨大シルエット連山)
	// =========================================================
	stepX = 8.0f;
	for (float x = -50.0f; x < stageWidth_ + 50.0f; x += stepX + distOffset(rng_) * 1.2f) {
		LayerObject* obj = new LayerObject();
		obj->transform.Initialize();

		float wave1 = std::cos(x * 0.04f) * 6.0f;
		float wave2 = std::sin(x * 0.11f) * 2.5f;
		float height = wave1 + wave2 + 3.0f + distOffset(rng_) * 1.0f;

		obj->basePosition = {x + distOffset(rng_) * 1.5f, height, 45.0f + distZOffset(rng_) * 3.0f};

		float scaleVal = distScaleFar(rng_);
		obj->transform.scale_ = {scaleVal * (1.0f + distOffset(rng_) * 0.1f), scaleVal, scaleVal};

		float rotY = (distFlipY(rng_) > 0.5f) ? std::numbers::pi_v<float> : 0.0f;
		float rotZ = distRotZ(rng_) * 0.5f;
		obj->transform.rotation_ = {0.0f, rotY, rotZ};

		obj->parallaxFactor = 0.1f;
		obj->model = modelBlock_;
		obj->cullRange = 150.0f;

		backgroundObjects_.push_back(obj);
	}

	// =========================================================
	// レイヤー 3：空の背景を右から左へ移動する無限アニメーション雲
	// =========================================================
	for (float x = -40.0f; x < stageWidth_ + 40.0f; x += 30.0f + distOffset(rng_) * 8.0f) {
		LayerObject* cloud = new LayerObject();
		cloud->transform.Initialize();

		cloud->isCloud = true; // ★ 雲フラグ設定

		// ★ Y軸ポジションを完全ランダムに設定
		float y = distCloudHeight(rng_);
		cloud->basePosition = {x, y, 32.0f + distZOffset(rng_) * 2.0f};

		// ★ XYZ軸のスケールを個別に不規則ランダム設定
		cloud->originalScale = {distScaleX(rng_), distScaleY(rng_), distScaleZ(rng_)};
		cloud->transform.scale_ = cloud->originalScale;

		// ★ Y軸ローテーションを全角度（0〜2π）ランダム設定
		cloud->transform.rotation_ = {0.0f, distCloudRotY(rng_), 0.0f};

		// ★ 右から左への移動速度をランダム設定
		cloud->moveSpeed = distMoveSpeed(rng_);

		cloud->parallaxFactor = 0.15f; // ゆっくり背景追従
		cloud->model = modelCloud_;
		cloud->cullRange = 200.0f; // ★ カリング範囲

		backgroundObjects_.push_back(cloud);
	}
}

void BackgroundBlocks::Update(const KamataEngine::Camera& camera) {
	// ★ 雲再生成用の乱数分布
	std::uniform_real_distribution<float> distCloudHeight(1.0f, 18.0f);
	std::uniform_real_distribution<float> distCloudRotY(0.0f, 2.0f * std::numbers::pi_v<float>);
	std::uniform_real_distribution<float> distScaleX(3.0f, 8.0f);
	std::uniform_real_distribution<float> distScaleY(2.0f, 5.0f);
	std::uniform_real_distribution<float> distScaleZ(3.0f, 8.0f);
	std::uniform_real_distribution<float> distMoveSpeed(0.02f, 0.07f);
	std::uniform_real_distribution<float> distZOffset(-1.0f, 1.0f);

	const float leftBound = -50.0f;               // ステージ左端の消滅境界
	const float rightBound = stageWidth_ + 50.0f; // ステージ右端の再生成境界
	const float fadeZone = 25.0f;                 // 段々消える（縮小フェードアウト）エリアの幅

	for (auto* obj : backgroundObjects_) {
		if (!obj) {
			continue;
		}

		// ★ 雲の右から左への移動と無限ループロジック
		if (obj->isCloud) {
			obj->basePosition.x -= obj->moveSpeed; // 右から左へ移動

			// マップステージの左側境界を超えたら右端から再生成
			if (obj->basePosition.x < leftBound) {
				obj->basePosition.x = rightBound;
				obj->basePosition.y = distCloudHeight(rng_); // ★ 新しいY軸ポジション
				obj->basePosition.z = 32.0f + distZOffset(rng_) * 2.0f;
				obj->originalScale = {distScaleX(rng_), distScaleY(rng_), distScaleZ(rng_)}; // ★ 新しいXYZスケール
				obj->transform.rotation_.y = distCloudRotY(rng_);                            // ★ 新しいY軸ローテーション
				obj->moveSpeed = distMoveSpeed(rng_);                                        // ★ 新しい移動速度
			}

			// ★ 左側に移動したら段々消えるアニメーション（スケールフェードアウト）
			float fadeFactor = 1.0f;
			if (obj->basePosition.x < leftBound + fadeZone) {
				fadeFactor = (obj->basePosition.x - leftBound) / fadeZone;
				fadeFactor = std::clamp(fadeFactor, 0.0f, 1.0f);
			}
			obj->transform.scale_ = {
			    obj->originalScale.x * fadeFactor,
			    obj->originalScale.y * fadeFactor,
			    obj->originalScale.z * fadeFactor,
			};
		}

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
	// ★ 描画前処理は 1 回だけ行い、まとめて描画する（タイルの数が多いため）
	KamataEngine::Model::PreDraw();
	for (auto* obj : backgroundObjects_) {
		if (!obj || !obj->model)
			continue;

		// カメラから横に遠く離れているものは画面に映らないので描画しない
		if (std::fabs(obj->transform.translation_.x - camera.translation_.x) > obj->cullRange) {
			continue;
		}

		obj->model->Draw(obj->transform, camera);
	}
	KamataEngine::Model::PostDraw();
}

void BackgroundBlocks::SetLightGroups(const KamataEngine::LightGroup* nearGroup, const KamataEngine::LightGroup* midGroup, const KamataEngine::LightGroup* farGroup) {
	// 奥行きステージ構造（S）のタイルは、中景より手前に見えるので専用のライトグループを使う
	for (int i = 0; i < kNumTileModels; ++i) {
		if (tileModels_[i]) {
			tileModels_[i]->SetLightGroup(nearGroup);
		}
	}

	// レイヤー 1（中景）は modelBlockAbove_、レイヤー 2（遠景）は modelBlock_ を使用している
	if (modelBlockAbove_) {
		modelBlockAbove_->SetLightGroup(midGroup);
	}
	if (modelBlock_) {
		modelBlock_->SetLightGroup(farGroup);
	}

	// ★ 追加: 雲モデルにも背景（遠景・空）用ライトグループを適用してライティングを反映
	if (modelCloud_) {
		modelCloud_->SetLightGroup(farGroup);
	}
}