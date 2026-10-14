#pragma once
#include "Kamataengine.h"
#include <map>

// マップチップ種別
enum class MapChipType {
	kBlank,       // 空白
	kBlock,       // ブロック
	kBlockFall,   // ★追加：すり抜けブロック (B1)
	kPlayer,      // プレイヤー
	kEnemy,       // 敵
	kGoal,        // ゴール
	kExplanation, // 解説ブロック
};

// ★ ブロックの向き（周囲の空白の位置から決まる見た目の種類）
enum class BlockFace {
	kNormal, // 通常のブロック（空白に面していない）
	kAbove,  // 上が空白（上に向いているブロック ＝ 地面）  ➔ blockAbove
	kBelow,  // 下が空白（下に向いているブロック ＝ 天井）  ➔ blockBelow
	kRight,  // 右が空白（右に向いているブロック ＝ 左壁）  ➔ blockRight
	kLeft,   // 左が空白（左に向いているブロック ＝ 右壁）  ➔ blockLeft
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

	// ★ CSVで実際に使われている横幅（何かが置かれている一番右の列 + 1）。ステージごとに横幅が違うため、カメラ範囲などはこちらを使う
	uint32_t GetUsedBlockHorizontal() const { return usedBlockHorizontal_; }

	// ★ ブロックの向きを判定する（空白に一番近いブロックだけが向きを持ち、それ以外は通常のブロックになる）
	//   優先順位: 上が空白 ➔ 下が空白 ➔ 右が空白 ➔ 左が空白。マップの外側（CSVで使われている横幅より右も含む）はブロックがあるものとして扱う
	BlockFace GetBlockFaceByIndex(uint32_t xIndex, uint32_t yIndex);
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

	// ★ ステージの最大サイズ（縦 30 × 横 200）。CSVはこの範囲内で自由な大きさにできる
	static inline const uint32_t kNumBlockVertical = 30;
	static inline const uint32_t kNumBlockHorizontal = 200;

	MapChipData mapChipData_;

	// CSVで実際に使われている横幅
	uint32_t usedBlockHorizontal_ = 0;
};