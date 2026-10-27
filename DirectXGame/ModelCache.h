#pragma once
#include "KamataEngine.h"
#include <string>
#include <unordered_map>

/// <summary>
/// モデルキャッシュ
/// ★ 軽量化: 一度読み込んだ OBJ モデルを使い回す（ステージ切り替え・リトライのたびに OBJ を読み直さない）。
/// ※ 同じ OBJ を別の役割（ライトグループが違うなど）で使う場合は slot を変えて別のインスタンスとして読み込む。
/// ※ キャッシュしたモデルは解放しない（アプリ終了まで保持する）。呼び出し側で delete しないこと。
/// </summary>
namespace ModelCache {

inline KamataEngine::Model* Load(const std::string& name, const std::string& slot = "") {
	static std::unordered_map<std::string, KamataEngine::Model*> cache;

	const std::string key = name + "#" + slot;
	auto it = cache.find(key);
	if (it != cache.end()) {
		return it->second;
	}

	KamataEngine::Model* model = KamataEngine::Model::CreateFromOBJ(name, true);
	cache[key] = model;
	return model;
}

} // namespace ModelCache
