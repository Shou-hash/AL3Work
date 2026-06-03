#include "MapChipField.h"
#include <algorithm>
#include <cassert>
#include <cmath>
#include <fstream>
#include <sstream>

namespace {
std::map<std::string, MapChipType> mapChipTable = {
    {"0", MapChipType::kBlank},
    {"1", MapChipType::kBlock},
};
}

void MapChipField::ResetMapChipData() {
	mapChipData_.data.clear();
	mapChipData_.data.resize(kNumBlockVertical);
	for (uint32_t i = 0; i < kNumBlockVertical; i++) {
		mapChipData_.data[i].resize(kNumBlockHorizontal);
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

	for (uint32_t i = 0; i < kNumBlockVertical; i++) {
		std::string line;
		getline(mapChipCsv, line);

		std::istringstream lineStream(line);

		for (uint32_t j = 0; j < kNumBlockHorizontal; j++) {

			std::string word;
			std::getline(lineStream, word, ',');

			if (mapChipTable.contains(word)) {
				mapChipData_.data[i][j] = mapChipTable[word];
			}
		}
	}
}

MapChipType MapChipField::GetMapChipTypeByIndex(uint32_t xIndex, uint32_t yIndex) {
	if (xIndex >= kNumBlockHorizontal) {
		return MapChipType::kBlank;
	}
	if (yIndex >= kNumBlockVertical) {
		return MapChipType::kBlank;
	}

	return mapChipData_.data[yIndex][xIndex];
}

KamataEngine::Vector3 MapChipField::GetMapChipPositionByIndex(uint32_t xIndex, uint32_t yIndex) {
	return KamataEngine::Vector3(kBlockWidth * xIndex, kBlockHeight * (kNumBlockVertical - 1 - yIndex), 0);
}

MapChipField::IndexSet MapChipField::GetMapChipIndexByPosition(const KamataEngine::Vector3& position) {
	IndexSet index{};

	// ブロックの中心が整数座標（0.0f, 1.0f...）なので、
	// 各マスの占有領域は [-0.5, +0.5) になります。+0.5f して floor することで完璧にマッピングされます。
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