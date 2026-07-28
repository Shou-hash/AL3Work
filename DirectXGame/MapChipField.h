#pragma once
#include "Kamataengine.h"
#include <map>

enum class MapChipType {
	kBlank,
	kBlock,
};

// 1マス分のデータ
struct MapChipDataUnit {
	MapChipType type = MapChipType::kBlank;
	uint8_t subID = 0;
};

// ステージ全体のマップチップデータ
struct MapChipData {
	std::vector<std::vector<MapChipDataUnit>> data;
};

// マップチップCSVの文字番号
enum MapChipCharIndex {
	kChipType = 0,  // マップチップタイプ
	kChipSubID = 1, // タイプごとのサブID
};

class MapChipField {
public:
	void ResetMapChipData();

	void LoadMapChipDataFromCSV(const std::string& filePath);

	MapChipType GetMapChipTypeByIndex(uint32_t xIndex, uint32_t yIndex);
	uint8_t GetMapChipSubIDByIndex(uint32_t xIndex, uint32_t yIndex);

	uint32_t GetNumBlockVertical() const { return kNumBlockVertical; }
	uint32_t GetNumBlockHorizontal() const { return kNumBlockHorizontal; }
	KamataEngine::Vector3 GetMapChipPositionByIndex(uint32_t xIndex, uint32_t yIndex);

	// 3D座標から対応するマップチップのインデックスを取得する
	struct IndexSet {
		uint32_t x;
		uint32_t y;
	};
	IndexSet GetMapChipIndexByPosition(const KamataEngine::Vector3& position);

	// 3D座標から直接その場所のマップチップの種類を取得する
	MapChipType GetMapChipTypeByPosition(const KamataEngine::Vector3& position);

private:
	static inline const float kBlockWidth = 1.0f;
	static inline const float kBlockHeight = 1.0f;

	static inline const uint32_t kNumBlockVertical = 20;
	static inline const uint32_t kNumBlockHorizontal = 100;

	MapChipData mapChipData_;
};