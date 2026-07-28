#pragma once
#include "KamataEngine.h"
#include <3d/WorldTransform.h>

class Player;

/// <summary>
/// 敵基底クラス
/// </summary>
class BaseEnemy {
public:
	// AABB構造体
	struct AABB {
		KamataEngine::Vector3 min;
		KamataEngine::Vector3 max;
	};

	// 仮想デストラクタ（メモリリーク防止）
	virtual ~BaseEnemy() = default;

	// 初期化（※資料に合わせて仮想関数として追加）
	virtual void Initialize(KamataEngine::Model* model, KamataEngine::Camera* camera, const KamataEngine::Vector3& position) {
		(void)model;
		(void)camera;
		(void)position;
	}

	// 更新
	virtual void Update() = 0;

	// 描画
	virtual void Draw() = 0;

	// プレイヤーとの衝突応答
	virtual void OnCollision(Player* player) { (void)player; }

	// 死亡演出開始
	virtual void OnDead() {}

	// フラグ取得
	virtual bool IsDead() const = 0;

	// AABB取得
	virtual AABB GetAABB() const = 0;

	void CheckEnemyCollision(const std::list<BaseEnemy*>& enemies);

	// ワールドトランスフォーム取得
	virtual const KamataEngine::WorldTransform& GetWorldTransform() const = 0;
};