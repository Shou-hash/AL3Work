#include "BackgroundBlocks.h"
#include "MapChipField.h"
#include "Matrix4x4.h"
#include <cmath>
#include <cstdlib>
#include <fstream>
#include <numbers>
#include <random>
#include <sstream>

namespace {

// ★ 背景CSVの座標系（1 マス = 1.0）。CSVの列 0 がワールドX = kBackgroundOriginX、一番下の行がワールドY = kBackgroundOriginY
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
	for (int i = 0; i < kNumTileModels; ++i) {
		delete tileModels_[i];
	}

	for (auto* obj : backgroundObjects_) {
		delete obj;
	}
	backgroundObjects_.clear();
}

// ★ 背景CSVの読み込み（M = 中景 / F = 遠景、2 文字目の数字 = 大きさのバリエーション）
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
		tileModels_[i] = KamataEngine::Model::CreateFromOBJ(tileModelNames[i], true);
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

void BackgroundBlocks::Initialize(MapChipField* mapChipField, const std::string& csvPath) {
	// 背景描画に使うモデルを読み込み
	modelBlock_ = KamataEngine::Model::CreateFromOBJ("block", true);
	modelBlockAbove_ = KamataEngine::Model::CreateFromOBJ("blockAbove", true);

	// ★ 背景CSVがあればCSVの配置を使う（無い場合は下の自動生成にフォールバック）
	if (!csvPath.empty() && LoadFromCSV(csvPath)) {
		return;
	}

	if (!mapChipField)
		return;

	// ステージ全体の横幅を取得（CSVで実際に使われている横幅）
	float stageWidth = static_cast<float>(mapChipField->GetUsedBlockHorizontal());

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
}
