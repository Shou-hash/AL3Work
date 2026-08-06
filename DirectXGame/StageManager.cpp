#include "StageManager.h"
#include <algorithm>
#include <cassert>
#include <fstream>
#include <map>
#include <sstream>

void StageManager::LoadStageDatas() {
	stageDatas_.clear();

	// 文字列から数値文字列への変換参照表（テーブル）
	std::map<std::string, std::string> list = {
	    {"B0", "1"}, // ブロック / ステージ1
	    {"P0", "2"}, // プレイヤー / ステージ2
	    {"E0", "3"}, // 普通の敵 / ステージ3
	    {"E1", "4"}, // 盾敵 / ステージ4
	};

	// 0, 1, 2 の3つのステージデータを読み込む
	for (int32_t i = 0; i < kNumStages; ++i) {
		const std::string filePath = "Resources/stageDatas" + std::to_string(i) + ".csv";
		std::ifstream file(filePath);

		if (!file.is_open()) {
			// ファイルがない場合はデフォルト値を設定
			StageData defaultData;
			defaultData.stageNo = i;
			defaultData.name = "Stage " + std::to_string(i);
			defaultData.timeLimit = 60;
			stageDatas_.push_back(defaultData);
			continue;
		}

		std::stringstream lineStream;
		lineStream << file.rdbuf();
		file.close();

		std::string line;
		StageData stageData;
		stageData.stageNo = i;
		stageData.name = "Stage " + std::to_string(i);
		stageData.timeLimit = 60;

		if (std::getline(lineStream, line)) {
			line.erase(std::remove(line.begin(), line.end(), '\r'), line.end());
			std::stringstream wordStream(line);
			std::string word;

			// 1列目: 識別子チェック
			if (std::getline(wordStream, word, ',')) {
				if (list.find(word) != list.end()) {
					word = list[word];
				}
				try {
					stageData.stageNo = std::stoi(word);
				} catch (const std::invalid_argument&) {
				}
			}
			// 2列目: ステージ名
			if (std::getline(wordStream, word, ',')) {
				stageData.name = word;
			}
			// 3列目: 制限時間
			if (std::getline(wordStream, word, ',')) {
				try {
					stageData.timeLimit = std::stoi(word);
				} catch (const std::invalid_argument&) {
				}
			}
		}
		stageDatas_.push_back(stageData);
	}
}

void StageManager::SetCurrentStageIndexByName(const std::string& name) {
	for (size_t i = 0; i < stageDatas_.size(); ++i) {
		if (stageDatas_[i].name == name || std::to_string(stageDatas_[i].stageNo) == name) {
			currentStageIndex_ = static_cast<int32_t>(i);
			return;
		}
	}
	currentStageIndex_ = 0;
}