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

// ★ 奥行きステージ構造（S）：ステージのタイルと同じ大きさ（1.0）。Z = 12 付近・視差 0.8
constexpr float kBackStageZ = 12.0f;
constexpr float kBackStageParallax = 0.8f;

// 大きさのバリエーション（サブID 0 ~ 3）：横・奥行きの倍率と縦の倍率
constexpr float kScaleMul[4] = {1.0f, 1.3f, 0.8f, 1.0f};
constexpr float kHeightMul[4] = {1.0f, 1.0f, 1.0f, 1.4f};

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

// ★ 背景CSVの読み込み（M = 中景 / F = 遠景 / S = 奥行きステージタイル）
bool BackgroundBlocks::LoadFromCSV(const std::string& csvPath) {
	std::ifstream file(csvPath);
	if (!file.is_open()) {
		return false;
	}

	// 行ごとに読み込み
	std::vector<std::string> lines;
	std::string line;
	while (std::getline(file, line)) {
		lines.push_back(line);
	}
	file.close();

	if (lines.empty()) {
		return false;
	}
	const int numRows = static_cast<int>(lines.size());

	// ★ 奥行きステージ構造（S）用のタイルモデルを読み込み（ステージのタイルと同じモデル）
	const char* tileModelNames[kNumTileModels] = {"block", "blockAbove", "blockBelow", "blockLeft", "blockRight"};
	for (int i = 0; i < kNumTileModels; ++i) {
		if (!tileModels_[i]) {
			tileModels_[i] = KamataEngine::Model::CreateFromOBJ(tileModelNames[i], true);
		}
	}

	// 各行をセルに分解
	std::vector<std::vector<std::string>> grid(numRows);
	for (int row = 0; row < numRows; ++row) {
		std::istringstream lineStream(lines[row]);
		std::string word;
		while (std::getline(lineStream, word, ',')) {
			grid[row].push_back(word);
		}
	}

	// 奥行きステージ構造（S）のタイルかどうか（範囲外はタイルがあるものとして扱う）
	auto isTile = [&](int col, int row) {
		if (row < 0 || row >= numRows || col < 0 || col >= static_cast<int>(grid[row].size())) {
			return true;
		}
		return !grid[row][col].empty() && grid[row][col][0] == 'S';
	};

	// 固定シード値で乱数生成器を初期化（ゆらぎも毎回同じになる）
	std::mt19937 rng(1337);
	std::uniform_real_distribution<float> distOffset(-0.8f, 0.8f);
	std::uniform_real_distribution<float> distRotZ(-0.25f, 0.25f);
	std::uniform_real_distribution<float> distFlipY(0.0f, 1.0f);
	std::uniform_real_distribution<float> distZOffset(-1.0f, 1.0f);

	for (int row = 0; row < numRows; ++row) {
		for (int col = 0; col < static_cast<int>(grid[row].size()); ++col) {
			const std::string& word = grid[row][col];
			if (word.empty()) {
				continue;
			}

			// ★ S：ステージのタイルと同じ大きさの奥行きステージ構造（ブロックの向きは GameScene と同じ判定）
			if (word[0] == 'S') {
				int modelIndex = 0; // 通常のブロック
				if (!isTile(col, row - 1)) {
					modelIndex = 1; // 上が空白（地面）
				} else if (!isTile(col, row + 1)) {
					modelIndex = 2; // 下が空白（天井）
				} else if (!isTile(col + 1, row)) {
					modelIndex = 4; // 右が空白（左壁）
				} else if (!isTile(col - 1, row)) {
					modelIndex = 3; // 左が空白（右壁）
				}

				LayerObject* tile = new LayerObject();
				tile->transform.Initialize();
				tile->basePosition = {kBackgroundOriginX + static_cast<float>(col), kBackgroundOriginY + static_cast<float>(numRows - 1 - row), kBackStageZ};
				tile->transform.scale_ = {1.0f, 1.0f, 1.0f};
				tile->parallaxFactor = kBackStageParallax;
				tile->model = tileModels_[modelIndex];
				tile->cullRange = 40.0f;
				backgroundObjects_.push_back(tile);
				continue;
			}

			if (word[0] != 'M' && word[0] != 'F') {
				continue;
			}
			const bool isMid = (word[0] == 'M');

			int subID = 0;
			if (word.size() > 1 && word[1] >= '0' && word[1] <= '3') {
				subID = word[1] - '0';
			}

			LayerObject* obj = new LayerObject();
			obj->transform.Initialize();

			// 中景：Z = 20 付近・視差 0.35 / 遠景：Z = 45 付近・視差 0.1
			const float zBase = isMid ? 20.0f : 45.0f;
			const float zJitter = isMid ? 1.0f : 3.0f;
			const float baseScale = isMid ? 5.0f : 16.0f;
			const float xJitter = isMid ? 1.0f : 1.5f;

			float x = kBackgroundOriginX + static_cast<float>(col) + distOffset(rng) * xJitter;
			float y = kBackgroundOriginY + static_cast<float>(numRows - 1 - row);
			obj->basePosition = {x, y, zBase + distZOffset(rng) * zJitter};

			float scaleVal = baseScale * kScaleMul[subID];
			obj->transform.scale_ = {scaleVal, scaleVal * kHeightMul[subID], scaleVal};

			float rotY = (distFlipY(rng) > 0.5f) ? std::numbers::pi_v<float> : 0.0f;
			float rotZ = distRotZ(rng) * (isMid ? 1.0f : 0.5f);
			obj->transform.rotation_ = {0.0f, rotY, rotZ};

			obj->parallaxFactor = isMid ? 0.35f : 0.1f;
			obj->model = isMid ? modelBlockAbove_ : modelBlock_;
			obj->cullRange = isMid ? 55.0f : 80.0f;

			backgroundObjects_.push_back(obj);
		}
	}

	return !backgroundObjects_.empty();
}

// ★ 追加: 空の背景を右から左へ移動する無限アニメーション雲の生成処理
void BackgroundBlocks::GenerateClouds() {
	std::uniform_real_distribution<float> distOffset(-0.8f, 0.8f);
	std::uniform_real_distribution<float> distCloudHeight(1.0f, 18.0f);                          // ★ Y軸ポジション（全不規則）
	std::uniform_real_distribution<float> distCloudRotY(0.0f, 2.0f * std::numbers::pi_v<float>); // ★ Y軸ローテーション（全不規則 0〜2π）
	std::uniform_real_distribution<float> distScaleX(3.0f, 8.0f);                                // ★ X軸スケール
	std::uniform_real_distribution<float> distScaleY(2.0f, 5.0f);                                // ★ Y軸スケール
	std::uniform_real_distribution<float> distScaleZ(3.0f, 8.0f);                                // ★ Z軸スケール
	std::uniform_real_distribution<float> distMoveSpeed(0.02f, 0.07f);                           // ★ 右から左への移動速度（全不規則）
	std::uniform_real_distribution<float> distZOffset(-1.0f, 1.0f);

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

void BackgroundBlocks::Initialize(MapChipField* mapChipField, const std::string& csvPath) {
	// 背景描画に使うモデルを読み込み
	modelBlock_ = KamataEngine::Model::CreateFromOBJ("block", true);
	modelBlockAbove_ = KamataEngine::Model::CreateFromOBJ("blockAbove", true);
	modelCloud_ = KamataEngine::Model::CreateFromOBJ("cloud3", true);

	// ★ 先にマップチップからステージ全体の横幅を取得する
	if (mapChipField) {
		stageWidth_ = static_cast<float>(mapChipField->GetUsedBlockHorizontal());
		if (stageWidth_ < 50.0f) {
			stageWidth_ = 100.0f; // ★ 背景が途切れないよう最小横幅を確保
		}
	}

	// 固定シード値で乱数生成器を初期化
	rng_.seed(1337);

	// ★ 背景CSVがあればCSVの配置を使う（読み込み成功時も雲を生成して終了）
	if (!csvPath.empty() && LoadFromCSV(csvPath)) {
		GenerateClouds(); // ★ CSV読み込み時も雲オブジェクトを追加生成する
		return;
	}

	if (!mapChipField) {
		return;
	}

	// 各種分布の設定
	std::uniform_real_distribution<float> distOffset(-0.8f, 0.8f);
	std::uniform_real_distribution<float> distRotZ(-0.25f, 0.25f);
	std::uniform_real_distribution<float> distFlipY(0.0f, 1.0f);

	std::uniform_real_distribution<float> distScaleMid(4.0f, 6.5f);   // 中景用スケール
	std::uniform_real_distribution<float> distScaleFar(12.0f, 20.0f); // 遠景用スケール
	std::uniform_real_distribution<float> distZOffset(-1.0f, 1.0f);   // Z軸の揺らぎ

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
	GenerateClouds();
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