#pragma once
#include <cassert>
#include <cstdint>
#include <string>
#include <vector>

struct StageData {
	int32_t stageNo = 0;   // ステージ番号 (0, 1, 2)
	std::string name;      // ステージ名
	int32_t timeLimit = 0; // 制限時間 (秒)
};

class StageManager {
public:
	// ★ 宣言のみにする
	void LoadStageDatas();
	void SetCurrentStageIndexByName(const std::string& name);

	const StageData& GetStageData(int32_t index) const {
		assert(index >= 0 && index < static_cast<int32_t>(stageDatas_.size()));
		return stageDatas_[index];
	}

	const StageData& GetCurrentStageData() const { return GetStageData(currentStageIndex_); }

	void SetCurrentStageIndex(int32_t index) {
		if (index >= 0 && index < static_cast<int32_t>(stageDatas_.size())) {
			currentStageIndex_ = index;
		}
	}

	int32_t GetCurrentStageIndex() const { return currentStageIndex_; }
	int32_t GetStageCount() const { return static_cast<int32_t>(stageDatas_.size()); }

private:
	static inline const int32_t kNumStages = 3; // stageDatas0.csv, stageDatas1.csv, stageDatas2.csv
	std::vector<StageData> stageDatas_;
	int32_t currentStageIndex_ = 0;
};