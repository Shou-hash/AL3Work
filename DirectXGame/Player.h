#pragma once
#include "3d/DebugCamera.h"
#include "Collider.h"
#include "Kamataengine.h"
#include "PlayerBullet.h"
#include <list>

/// <summary>
/// プレイヤークラス（Colliderを継承）
/// </summary>
class Player : public Collider {
public:
	// デストラクタ
	~Player();

	// 初期化
	void Initialize(KamataEngine::Model* model, uint32_t textureHandle);

	// 更新
	void Update();

	// 描画
	void Draw(KamataEngine::Camera* camera);

	// ★ Colliderの純粋仮想関数・仮想関数をオーバーライド
	KamataEngine::Vector3 GetWorldPosition() const override;
	void OnCollision() override;

	// 自弾リストを取得する getter（参照渡し）
	const std::list<PlayerBullet*>& GetBullets() const { return bullets_; }

private:
	/// <summary>
	/// 攻撃
	/// </summary>
	void Attack();

private:
	KamataEngine::WorldTransform worldTransform_;
	KamataEngine::Model* model_ = nullptr;
	uint32_t textureHandle_ = 0u;

	// キーボード入力
	KamataEngine::Input* input_ = nullptr;

	// 弾のリスト（複数管理）
	std::list<PlayerBullet*> bullets_;
};