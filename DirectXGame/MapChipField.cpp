#include "MapChipField.h"
#include <algorithm>
#include <cassert>
#include <cmath>
#include <fstream>
#include <sstream>

namespace {
// マップチップ種別テーブル
std::map<char, MapChipType> mapChipTypeTable = {
    {'B', MapChipType::kBlock      },
    {'P', MapChipType::kPlayer     },
    {'E', MapChipType::kEnemy      },
    {'G', MapChipType::kGoal       },
    {'K', MapChipType::kExplanation}, // ★追加：解説ブロック
    {'C', MapChipType::kCloud      }, // ★ 追加：'C' を雲マップチップとして読み込み
};
} // namespace

void MapChipField::ResetMapChipData() {
	usedBlockHorizontal_ = 0;
	mapChipData_.data.clear();
	mapChipData_.data.resize(kNumBlockVertical);
	for (auto& mapChipDataLine : mapChipData_.data) {
		mapChipDataLine.resize(kNumBlockHorizontal);
	}
}

void MapChipField::LoadMapChipDataFromCSV(const std::string& filePath) {
	ResetMapChipData();

	std::ifstream file;
	file.open(filePath);
	assert(file.is_open());

	std::stringstream mapChipCsv;
	mapChipCsv << file.rdbuf();
	file.close();

	for (uint32_t i = 0; i < kNumBlockVertical; ++i) {
		std::string line;
		if (!getline(mapChipCsv, line)) {
			break;
		}

		std::istringstream lineStream(line);

		for (uint32_t j = 0; j < kNumBlockHorizontal; ++j) {

			std::string word;
			std::getline(lineStream, word, ',');

			if (word.empty()) {
				continue;
			}

			if (!mapChipTypeTable.contains(word[kChipType])) {
				continue;
			}

			// マップチップのタイプを設定
			mapChipData_.data[i][j].type = mapChipTypeTable[word[kChipType]];

			// 実際に使われている横幅を更新
			if (j + 1 > usedBlockHorizontal_) {
				usedBlockHorizontal_ = j + 1;
			}

			if (word.size() <= kChipSubID) {
				continue;
			}

			// サブIDの読み込み
			uint8_t subID = static_cast<uint8_t>(word[kChipSubID] - '0');
			mapChipData_.data[i][j].subID = subID;

			// B1 (Block 且つ subIDが1) の場合は kBlockFall として設定
			if (mapChipData_.data[i][j].type == MapChipType::kBlock && subID == 1) {
				mapChipData_.data[i][j].type = MapChipType::kBlockFall;
			}
		}
	}
}

MapChipType MapChipField::GetMapChipTypeByIndex(uint32_t xIndex, uint32_t yIndex) {
	if (xIndex >= kNumBlockHorizontal || yIndex >= kNumBlockVertical) {
		return MapChipType::kBlank;
	}

	return mapChipData_.data[yIndex][xIndex].type;
}

uint8_t MapChipField::GetMapChipSubIDByIndex(uint32_t xIndex, uint32_t yIndex) {
	if (xIndex >= kNumBlockHorizontal || yIndex >= kNumBlockVertical) {
		return 0;
	}

	return mapChipData_.data[yIndex][xIndex].subID;
}

KamataEngine::Vector3 MapChipField::GetMapChipPositionByIndex(uint32_t xIndex, uint32_t yIndex) {
	return KamataEngine::Vector3(kBlockWidth * xIndex, kBlockHeight * (kNumBlockVertical - 1 - yIndex), 0);
}

MapChipField::IndexSet MapChipField::GetMapChipIndexByPosition(const KamataEngine::Vector3& position) {
	IndexSet index{};

	float xIndexF = (position.x / kBlockWidth) + 0.5f;
	float yIndexF = static_cast<float>(kNumBlockVertical - 1) - (position.y / kBlockHeight) + 0.5f;

	int32_t xInt = static_cast<int32_t>(std::floor(xIndexF));
	int32_t yInt = static_cast<int32_t>(std::floor(yIndexF));

	index.x = std::clamp(xInt, 0, static_cast<int32_t>(kNumBlockHorizontal - 1));
	index.y = std::clamp(yInt, 0, static_cast<int32_t>(kNumBlockVertical - 1));

	return index;
}

MapChipType MapChipField::GetMapChipTypeByPosition(const KamataEngine::Vector3& position) {
	IndexSet index = GetMapChipIndexByPosition(position);
	return GetMapChipTypeByIndex(index.x, index.y);
}

BlockFace MapChipField::GetBlockFaceByIndex(uint32_t xIndex, uint32_t yIndex) {
	// その場所が「空白（ブロック以外）」かどうか。
	// マップの外側（最大サイズの外、およびCSVで使われている横幅より右）はブロックがあるものとして扱い、外周に余計な向きを付けない
	auto isBlank = [this](int32_t x, int32_t y) {
		if (x < 0 || y < 0 || x >= static_cast<int32_t>(usedBlockHorizontal_) || y >= static_cast<int32_t>(kNumBlockVertical)) {
			return false;
		}
		return GetMapChipTypeByIndex(static_cast<uint32_t>(x), static_cast<uint32_t>(y)) != MapChipType::kBlock;
	};

	const int32_t x = static_cast<int32_t>(xIndex);
	const int32_t y = static_cast<int32_t>(yIndex);

	// yIndex は上が 0（-1 が上、+1 が下）、xIndex は左が 0（-1 が左、+1 が右）
	if (isBlank(x, y - 1)) {
		return BlockFace::kAbove; // 上が空白 ➔ 地面のブロック
	}
	if (isBlank(x, y + 1)) {
		return BlockFace::kBelow; // 下が空白 ➔ 天井のブロック
	}
	if (isBlank(x + 1, y)) {
		return BlockFace::kRight; // 右が空白 ➔ 左壁のブロック（右に向いている）
	}
	if (isBlank(x - 1, y)) {
		return BlockFace::kLeft; // 左が空白 ➔ 右壁のブロック（左に向いている）
	}
	return BlockFace::kNormal;
}
