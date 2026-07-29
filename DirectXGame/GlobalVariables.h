#pragma once
#include <KamataEngine.h>
#include <cstdint>
#include <map>
#include <string>
#include <variant>

/// <summary>
/// グローバル変数（調整項目マネージャー）
/// </summary>
class GlobalVariables {
public:
	// 項目構造体 (int32_t, float, Vector3 のいずれかを保持)
	struct Item {
		std::variant<int32_t, float, KamataEngine::Vector3> value;
	};

	// グループ構造体 (項目名をキーとする)
	struct Group {
		std::map<std::string, Item> items;
	};

public:
	/// <summary>
	/// シングルトンインスタンスの取得
	/// </summary>
	static GlobalVariables* GetInstance();

	/// <summary>
	/// 毎フレーム更新処理 (ImGuiの描画)
	/// </summary>
	void Update();

	/// <summary>
	/// グループの作成
	/// </summary>
	/// <param name="groupName">グループ名</param>
	void CreateGroup(const std::string& groupName);

	/// <summary>
	/// 値のセット (int32_t)
	/// </summary>
	void SetValue(const std::string& groupName, const std::string& key, int32_t value);

	/// <summary>
	/// 値のセット (float)
	/// </summary>
	void SetValue(const std::string& groupName, const std::string& key, float value);

	/// <summary>
	/// 値のセット (Vector3)
	/// </summary>
	void SetValue(const std::string& groupName, const std::string& key, const KamataEngine::Vector3& value);

private:
	GlobalVariables() = default;
	~GlobalVariables() = default;
	GlobalVariables(const GlobalVariables&) = delete;
	GlobalVariables& operator=(const GlobalVariables&) = delete;

private:
	// 全データ (グループ名をキーとするコンテナ)
	std::map<std::string, Group> datas_;
};