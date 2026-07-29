#include "GlobalVariables.h"
#include <imgui.h>

GlobalVariables* GlobalVariables::GetInstance() {
	static GlobalVariables instance;
	return &instance;
}

// 毎フレーム更新処理
void GlobalVariables::Update() {
	if (!ImGui::Begin("Global Variables", nullptr, ImGuiWindowFlags_MenuBar)) {
		ImGui::End();
		return;
	}

	if (!ImGui::BeginMenuBar())
		return;

	// 各グループについて
	for (std::map<std::string, Group>::iterator itGroup = datas_.begin(); itGroup != datas_.end(); ++itGroup) {

		// グループ名を取得
		const std::string& groupName = itGroup->first;
		// グループの参照を取得
		Group& group = itGroup->second;

		if (!ImGui::BeginMenu(groupName.c_str()))
			continue;

		// 各項目について
		for (std::map<std::string, Item>::iterator itItem = group.items.begin(); itItem != group.items.end(); ++itItem) {

			// 項目名を取得
			const std::string& itemName = itItem->first;
			// 項目の参照を取得
			Item& item = itItem->second;

			// int32_t 型の値を保持していれば
			if (std::holds_alternative<int32_t>(item.value)) {
				int32_t* ptr = std::get_if<int32_t>(&item.value);
				ImGui::SliderInt(itemName.c_str(), ptr, 0, 100);
			}
			// float 型の値を保持していれば
			else if (std::holds_alternative<float>(item.value)) {
				float* ptr = std::get_if<float>(&item.value);
				ImGui::SliderFloat(itemName.c_str(), ptr, 0.0f, 100.0f);
			}
			// Vector3 型の値を保持していれば
			else if (std::holds_alternative<KamataEngine::Vector3>(item.value)) {
				KamataEngine::Vector3* ptr = std::get_if<KamataEngine::Vector3>(&item.value);
				ImGui::SliderFloat3(itemName.c_str(), reinterpret_cast<float*>(ptr), -10.0f, 10.0f);
			}
		}

		ImGui::EndMenu();
	}

	ImGui::EndMenuBar();
	ImGui::End();
}

// 指定名のオブジェクト（グループ）がなければ追加する
void GlobalVariables::CreateGroup(const std::string& groupName) { datas_[groupName]; }

// 値のセット (int32_t)
void GlobalVariables::SetValue(const std::string& groupName, const std::string& key, int32_t value) {
	Group& group = datas_[groupName];
	Item newItem{};
	newItem.value = value;
	group.items[key] = newItem;
}

// 値のセット (float)
void GlobalVariables::SetValue(const std::string& groupName, const std::string& key, float value) {
	Group& group = datas_[groupName];
	Item newItem{};
	newItem.value = value;
	group.items[key] = newItem;
}

// 値のセット (Vector3)
void GlobalVariables::SetValue(const std::string& groupName, const std::string& key, const KamataEngine::Vector3& value) {
	Group& group = datas_[groupName];
	Item newItem{};
	newItem.value = value;
	group.items[key] = newItem;
}