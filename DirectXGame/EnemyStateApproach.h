#pragma once
#include "BaseEnemyState.h"

// 接近フェーズ
class EnemyStateApproach : public BaseEnemyState {
public:
	// コンストラクタ
	EnemyStateApproach(Enemy* enemy);

	// 更新処理
	void Update() override;

private:
	// 経過フレームカウンター（動きの計算用）
	uint32_t frameCount_ = 0;
};