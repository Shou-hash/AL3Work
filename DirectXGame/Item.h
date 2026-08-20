#pragma once
#include "KamataEngine.h"
#include <3d/WorldTransform.h>

class MapChipField;
class Player;

/// <summary>
/// ドロップアイテム（HP回復アイテム）
/// </summary>
class Item {
public:
	struct AABB {
		KamataEngine::Vector3 min;
		KamataEngine::Vector3 max;
	};

	Item() = default;
	~Item() = default;

	// ★追加：初期化処理
	void Initialize(KamataEngine::Model* model, KamataEngine::Camera* camera, const KamataEngine::Vector3& position, MapChipField* mapChipField);
	void Update();
	void Draw();

	void OnCollision(Player* player);
	bool IsDead() const { return isDead_; }
	AABB GetAABB() const;

private:
	KamataEngine::WorldTransform worldTransform_;
	KamataEngine::Camera* camera_ = nullptr;
	KamataEngine::Model* model_ = nullptr;
	MapChipField* mapChipField_ = nullptr; // ★追加：マップチップフィールド参照

	KamataEngine::Vector3 velocity_ = {0.0f, 0.0f, 0.0f};
	bool isDead_ = false;
	bool isGrounded_ = false;

	static inline const float kGravity = 0.012f;
	static inline const float kJumpPowerY = 0.25f;
};