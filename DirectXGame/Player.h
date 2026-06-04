#pragma once
#include "Kamataengine.h"
#include <3d/WorldTransform.h>
#include <array>

class MapChipField;

enum class LRDirection {
	kLeft,
	kRight,
};

class Player {
public:
	// 4つの角の定義
	enum Corner {
		kRightBottom, // 右下 (0)
		kLeftBottom,  // 左下 (1)
		kRightTop,    // 右上 (2)
		kLeftTop,     // 左上 (3)
		kNumCorner    // 要素数 (4)
	};

	// 衝突判定結果の構造体
	struct CollisionMapInfo {
		bool ceilingCollision = false;    // 天井衝突
		bool onGround = false;            // 着地
		bool wallCollision = false;       // 壁接触
		KamataEngine::Vector3 moveAmount; // 補正後の移動量
	};

	Player();
	~Player();

	void Initialize(KamataEngine::Model* model, KamataEngine::Camera* camera, const KamataEngine::Vector3& position);
	void KeysPush();
	void Update();
	void Draw();

	void SetMapChipField(MapChipField* mapChipField) { mapChipField_ = mapChipField; }

	const KamataEngine::Vector3& GetVelocity() const { return velocity_; }
	const KamataEngine::WorldTransform& GetWorldTransform() const { return worldTransform_; }

	// 各種調整パラメータ
	static inline const float kAcceleration = 0.03f;
	static inline const float kAttenuation = 0.5f;
	static inline const float kLimitRunSpeed = 2.0f;
	static inline const float kTimeTurn = 0.8f;
	static inline const float kGravityAcceleration = 0.08f; // スムーズな落下にするため元の0.3から調整
	static inline const float kLimitFallSpeed = 2.0f;       // 元の0.2から調整
	static inline const float kJumpAcceleration = 1.0f;     // 快適なジャンプ力に調整

private:
	// 移動入力を独立させた関数
	void Move();

	// マップ判定の主要関数と小分け関数
	void MapCollision(CollisionMapInfo& info);
	void MapCollisionTop(CollisionMapInfo& info);
	void MapCollisionBottom(CollisionMapInfo& info);
	void MapCollisionRight(CollisionMapInfo& info);
	void MapCollisionLeft(CollisionMapInfo& info);

	// データテーブルを用いた角の座標計算
	KamataEngine::Vector3 CornerPosition(const KamataEngine::Vector3& center, Corner corner);

private:
	MapChipField* mapChipField_ = nullptr;

	// キャラクターの当たり判定サイズ（1ブロック 1.0f に対して一回り小さい 0.8f）
	static inline const float kWidth = 0.8f;
	static inline const float kHeight = 0.8f;

	KamataEngine::Vector3 velocity_ = {};
	bool onGround_ = true;

	float turnFirstRotationY_ = 0.0f;
	float turnTimer_ = 0.0f;

	KamataEngine::Camera* camera_ = nullptr;
	KamataEngine::Model* modelPlayer_ = nullptr;
	KamataEngine::WorldTransform worldTransform_;

	LRDirection lrDirection_ = LRDirection::kRight;
};