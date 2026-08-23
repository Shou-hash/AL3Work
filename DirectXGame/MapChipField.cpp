#include "MapChipField.h"
#include <algorithm>
#include <cassert>
#include <cmath>
#include <fstream>
#include <sstream>

namespace {
// マップチップ種別テーブル
std::map<char, MapChipType> mapChipTypeTable = {
    {'B', MapChipType::kBlock },
    {'P', MapChipType::kPlayer},
    {'E', MapChipType::kEnemy },
    {'G', MapChipType::kGoal  }, // ★追加：ゴール（G0で指定可能）
};
} // namespace

void MapChipField::ResetMapChipData() {
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

			// 空白の場合はスキップ
			if (word.empty()) {
				continue;
			}

			// 先頭文字がいずれかのマップチップ種別に該当するか確認
			if (!mapChipTypeTable.contains(word[kChipType])) {
				continue;
			}

			// 先頭文字でマップチップのタイプを判別
			mapChipData_.data[i][j].type = mapChipTypeTable[word[kChipType]];

			// サブIDを含まない場合はスキップ（0番で確定）
			if (word.size() <= kChipSubID) {
				continue;
			}

			// マップチップのサブIDを設定
			mapChipData_.data[i][j].subID = static_cast<uint8_t>(word[kChipSubID] - '0');
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