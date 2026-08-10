#pragma once
#include "KamataEngine.h"

/// <summary>
/// 衝突判定の基底クラス
/// </summary>
class Collider {
public:
	virtual ~Collider() = default;

	// ワールド座標を取得するための純粋仮想関数
	virtual KamataEngine::Vector3 GetWorldPosition() const = 0;

	// 衝突時に呼び出される純粋仮想関数
	virtual void OnCollision() = 0;

	// 半径のゲッター・セッター
	float GetRadius() const { return radius_; }
	void SetRadius(float radius) { radius_ = radius; }

	// 衝突属性（フィルタリング用）のゲッター・セッター
	uint32_t GetCollisionAttribute() const { return collisionAttribute_; }
	void SetCollisionAttribute(uint32_t attribute) { collisionAttribute_ = attribute; }

	// 衝突マスク（フィルタリング用）のゲッター・セッター
	uint32_t GetCollisionMask() const { return collisionMask_; }
	void SetCollisionMask(uint32_t mask) { collisionMask_ = mask; }

private:
	// 当たり判定の半径
	float radius_ = 0.0f;

	// 自分の衝突属性
	uint32_t collisionAttribute_ = 0xffffffff;

	// 衝突を受け入れる相手のマスク
	uint32_t collisionMask_ = 0xffffffff;
};