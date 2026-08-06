#pragma once
#include <KamataEngine.h>

/// <summary>
/// 衝突判定オブジェクト基底クラス
/// </summary>
class Collider {
public:
	// 仮想デストラクタ
	virtual ~Collider() = default;

	/// <summary>
	/// ワールド座標を取得（純粋仮想関数）
	/// </summary>
	virtual KamataEngine::Vector3 GetWorldPosition() const = 0;

	/// <summary>
	/// 衝突時に呼ばれる関数（仮想関数）
	/// </summary>
	virtual void OnCollision() {}

	// 半径を取得
	float GetRadius() const { return radius_; }

	// 半径を設定
	void SetRadius(float radius) { radius_ = radius; }

private:
	// 衝突半径
	float radius_ = 1.0f;
};