#pragma once
#include <cassert>
#include <cstdint>
#include <string>
#include <vector>

/// <summary>
/// 1ステージ分のデータ構造体
/// </summary>
struct StageData {
	int32_t stageNo = 0;   // ステージ番号
	std::string name;      // ステージ名
	int32_t timeLimit = 0; // 制限時間 (秒)
};

/// <summary>
/// ステージ管理
/// </summary>
class StageManager {
public:
	/// <summary>
	/// ステージデータファイルの読み込み
	/// </summary>
	void LoadStageDatas();

	/// <summary>
	/// ステージ名指定で現在ステージ番号設定（スクショ1の資料より）
	/// </summary>
	/// <param name="name">ステージ名</param>
	void SetCurrentStageIndexByName(const std::string& name);

	/// <summary>
	/// ステージデータの取得
	/// </summary>
	const StageData& GetStageData(int32_t index) const {
		assert(index >= 0 && index < static_cast<int32_t>(stageDatas_.size()));
		return stageDatas_[index];
	}

	/// <summary>
	/// 現在ステージのステージデータ取得
	/// </summary>
	const StageData& GetCurrentStageData() const { return GetStageData(currentStageIndex_); }

	// 現在のステージインデックスのセッター / ゲッター
	void SetCurrentStageIndex(int32_t index) {
		assert(index >= 0 && index < static_cast<int32_t>(stageDatas_.size()));
		currentStageIndex_ = index;
	}

	int32_t GetCurrentStageIndex() const { return currentStageIndex_; }

	int32_t GetStageCount() const { return static_cast<int32_t>(stageDatas_.size()); }

private:
	// 全ステージデータ
	std::vector<StageData> stageDatas_;

	// 現在のステージインデックス
	int32_t currentStageIndex_ = 0;
};