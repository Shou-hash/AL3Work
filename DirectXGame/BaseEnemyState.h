#pragma once
#include <string>

// 前方宣言
class Enemy;

// 敵の状態基底クラス
class BaseEnemyState {
public:
	BaseEnemyState(const std::string& name, Enemy* enemy) : name_(name), enemy_(enemy) {}

	virtual ~BaseEnemyState() = default;

	// 毎フレーム処理（純粋仮想関数）
	virtual void Update() = 0;

	// デバッグログ出力
	virtual void DebugLog();

protected:
	// 状態名
	std::string name_;
	// 操作対象の敵
	Enemy* enemy_ = nullptr;
};