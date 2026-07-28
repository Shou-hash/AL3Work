#include "StageManager.h"
#include <algorithm> // std::remove のために追加
#include <cassert>
#include <fstream>
#include <map> // std::map のために追加
#include <sstream>

void StageManager::LoadStageDatas() {
	// 念のため以前のデータをクリア
	stageDatas_.clear();

	// 文字列の置き換え用参照表（変換テーブル）
	// 必要に応じて項目を追加してください（例: "B1" -> "2" など）
	const std::map<std::string, std::string> replaceTable = {
	    {",", "0"},
	    {"B0", "1"},// {"B1", "2"}, // 他に特殊な記号記法があればここに追加
	    {"P0", "2"},
	    {"E0", "3"},
	};

	const std::string filePath = "Resources/stageDatas.csv";
	std::ifstream file(filePath);

	// ファイルが開けない場合はアサートを出す
	assert(file.is_open() && "Resources/stageDatas.csv が見つかりません！ファイルの配置場所を確認してください。");

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

		// 1列目: ステージ名
		if (!std::getline(wordStream, word, ',')) {
			continue;
		}
		stageData.name = word;

		// 2列目: 制限時間
		if (std::getline(wordStream, word, ',')) {
			// 参照表にキーが存在する場合は値を置き換える
			auto it = replaceTable.find(word);
			if (it != replaceTable.end()) {
				word = it->second;
			}

			try {
				stageData.timeLimit = std::stoi(word);
				stageDatas_.push_back(stageData);
			} catch (const std::invalid_argument&) {
				// ヘッダー行（"timeLimit" など）のスキップ
				continue;
			}
		}
	}

	// 1件も読み込めなかった場合もアサートを出す
	assert(!stageDatas_.empty() && "stageDatas.csv の中に有効なデータがありません！");
}