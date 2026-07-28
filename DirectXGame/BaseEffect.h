#pragma once
#include <KamataEngine.h>

/// <summary>
/// エフェクト基底クラス
/// </summary>
class BaseEffect {
public:
	virtual ~BaseEffect() = default;

	// 共通インターフェース
	virtual void Update() = 0;
	virtual void Draw() = 0;
	virtual bool IsFinished() const = 0;
};