#pragma once
#include <KamataEngine.h>
#include <json.hpp>
#include <map>
#include <string>
#include <variant>

class GlobalVariables {
public:
	using json = nlohmann::json;

	// 項目構造体
	struct Item {
		std::variant<int32_t, float, KamataEngine::Vector3> value;
	};

	// グループ構造体
	struct Group {
		std::map<std::string, Item> items;
	};

	static GlobalVariables* GetInstance();

	void CreateGroup(const std::string& groupName);
	void Update();
	void LoadFiles();
	void LoadFile(const std::string& groupName);
	void SaveFile(const std::string& groupName);

	// 値のセット (オーバーロード)
	void SetValue(const std::string& groupName, const std::string& key, int32_t value);
	void SetValue(const std::string& groupName, const std::string& key, float value);
	void SetValue(const std::string& groupName, const std::string& key, const KamataEngine::Vector3& value);

	// 項目の追加（未登録なら初期値を SetValue で登録）
	void AddItem(const std::string& groupName, const std::string& key, int32_t value);
	void AddItem(const std::string& groupName, const std::string& key, float value);
	void AddItem(const std::string& groupName, const std::string& key, const KamataEngine::Vector3& value);

	// 値の取得 (getter)
	int32_t GetIntValue(const std::string& groupName, const std::string& key) const;
	float GetFloatValue(const std::string& groupName, const std::string& key) const;
	KamataEngine::Vector3 GetVector3Value(const std::string& groupName, const std::string& key) const;

private:
	GlobalVariables() = default;
	~GlobalVariables() = default;
	GlobalVariables(const GlobalVariables&) = delete;
	GlobalVariables& operator=(const GlobalVariables&) = delete;

	std::map<std::string, Group> datas_;
	const std::string kDirectoryPath = "Resources/GlobalVariables/";
};