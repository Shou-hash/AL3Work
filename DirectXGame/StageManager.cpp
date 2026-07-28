#include "StageManager.h"
#include <algorithm> // std::remove のため
#include <cassert>
#include <fstream>
#include <map> // std::map のため
#include <sstream>

void StageManager::LoadStageDatas() {
	// 念のため以前のデータをクリア
	stageDatas_.clear();

	// 文字列から数値文字列への変換参照表（テーブル）
	std::map<std::string, std::string> list = {
	    {"B0", "1"}, // ブロック / ステージ1
	    {"P0", "2"}, // プレイヤー / ステージ2
	    {"E0", "3"}, // 普通の敵 / ステージ3
	    {"E1", "4"}, // 盾敵 / ステージ4
	};

	const std::string filePath = "Resources/stageDatas" + std::to_string(currentStageIndex_) + ".csv";
	std::ifstream file(filePath);

	// ファイルが開けない場合はアサートを出す
	assert(file.is_open() && "指定の stageData*.csv が見つかりません！ファイルの配置場所を確認してください。");

	std::stringstream lineStream;
	lineStream << file.rdbuf();
	file.close();

	std::string line;
	while (std::getline(lineStream, line)) {
		line.erase(std::remove(line.begin(), line.end(), '\r'), line.end());

		if (line.empty()) {
			continue;
		}

		std::stringstream wordStream(line);
		StageData stageData;
		std::string word;

		// 1列目: ステージ番号（または識別子文字 "B0", "1" など）
		if (!std::getline(wordStream, word, ',')) {
			continue;
		}

		// 参照表(list)にキーが存在する場合は番号文字列に置換
		if (list.find(word) != list.end()) {
			word = list[word];
		}

		try {
			// ステージ番号を整数に変換
			stageData.stageNo = std::stoi(word);
		} catch (const std::invalid_argument&) {
			// ヘッダー行（"stageNo" など）のスキップ
			continue;
		}

		//// 2列目: ステージ名
		if (std::getline(wordStream, word, ',')) {
			stageData.name = word;
		}

		// 3列目: 制限時間
		if (std::getline(wordStream, word, ',')) {
			try {
				stageData.timeLimit = std::stoi(word);
			} catch (const std::invalid_argument&) {
				stageData.timeLimit = 60; // 変換失敗時のデフォルト値
			}
		}

		stageDatas_.push_back(stageData);
	}

	// 1件も読み込めなかった場合もアサートを出す
	assert(!stageDatas_.empty() && "stageData*.csv の中に有効なデータがありません！");
}

void StageManager::SetCurrentStageIndexByName(const std::string& name) {
	// 全ステージデータを検索
	for (size_t i = 0; i < stageDatas_.size(); ++i) {
		// ステージ名または識別子("stageDatas0" や "1" など)が一致したら設定
		if (stageDatas_[i].name == name || std::to_string(stageDatas_[i].stageNo) == name) {
			currentStageIndex_ = static_cast<int32_t>(i);
			return;
		}
	}

	// 見つからなかった場合は0番目にフォールバック（警告を出すなど）
	currentStageIndex_ = 0;
}