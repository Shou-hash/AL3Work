#pragma once
#include <cassert>
#include <cstdint>
#include <string>
#include <vector>

/// <summary>
/// 1ステージ分のデータ構造体
/// </summary>
struct StageData {
	std::string name;  // ステージ名 (フィールドCSVファイル名)
	int32_t timeLimit; // 制限時間 (秒)
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
	/// ステージデータの取得
	/// </summary>
	/// <param name="index">ステージ番号</param>
	/// <returns>ステージデータ</returns>
	const StageData& GetStageData(int32_t index) const {
		assert(index >= 0 && index < static_cast<int32_t>(stageDatas_.size()));
		return stageDatas_[index];
	}

	/// <summary>
	/// 現在ステージのステージデータ取得
	/// </summary>
	/// <returns>ステージデータ</returns>
	const StageData& GetCurrentStageData() const { return GetStageData(currentStageIndex_); }

	// 現在のステージ番号のセッター / ゲッター
	void SetCurrentStageIndex(int32_t index) {
		assert(index >= 0 && index < static_cast<int32_t>(stageDatas_.size()));
		currentStageIndex_ = index;
	}

	int32_t GetCurrentStageIndex() const { return currentStageIndex_; }

private:
	// 全ステージデータ
	std::vector<StageData> stageDatas_;

	// 現在のステージ番号
	int32_t currentStageIndex_ = 0;
};