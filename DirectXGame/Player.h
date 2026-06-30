#pragma once
#include "Kamataengine.h"
#include <3d/WorldTransform.h>
#include <array>
#include <list> // std::list を使用するため追加

class MapChipField;
class CameraController;
class Enemy; // 前方宣言

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
	void SetCameraController(CameraController* cameraController) { cameraController_ = cameraController; }

	const KamataEngine::Vector3& GetVelocity() const { return velocity_; }
	const KamataEngine::WorldTransform& GetWorldTransform() const { return worldTransform_; }

	// 死亡状態の取得
	bool IsDead() const { return isDead_; }

	// 敵との衝突判定
	void CheckEnemyCollision(const std::list<Enemy*>& enemies);

	// 各種調整パラメータ
	static inline const float kAcceleration = 0.03f;
	static inline const float kAttenuation = 0.5f;
	static inline const float kLimitRunSpeed = 2.0f;
	static inline const float kTimeTurn = 0.8f;
	static inline const float kGravityAcceleration = 0.08f;
	static inline const float kLimitFallSpeed = 1.0f;
	static inline const float kJumpAcceleration = 1.0f;

	// 追加：着地時の速度減衰率と、接地吸着判定用の微小オフセット
	static inline const float kAttenuationLanding = 0.2f;
	static inline const float kGroundSearchOffset = 0.01f;

private:
	// 移動入力を独立させた関数
	void Move();

	// マップ判定の主要関数と小分け関数
	void MapCollision(CollisionMapInfo& info);
	void MapCollisionTop(CollisionMapInfo& info);
	void MapCollisionBottom(CollisionMapInfo& info);
	void MapCollisionRight(CollisionMapInfo& info);
	void MapCollisionLeft(CollisionMapInfo& info);

	// 追加：接地状態の切り替え処理（内部で空中・地上の処理を分岐）
	void ApplyGroundingStatus(const CollisionMapInfo& info);

	// 画面端の押し出しと挟まれ死亡判定
	void CheckScreenEdgeCollision();

	// データテーブルを用いた角の座標計算
	KamataEngine::Vector3 CornerPosition(const KamataEngine::Vector3& center, Corner corner);

private:
	MapChipField* mapChipField_ = nullptr;
	CameraController* cameraController_ = nullptr;

	// キャラクターの当たり判定サイズ（1ブロック 1.0f に対して一回り小さい 0.8f）
	static inline const float kWidth = 0.8f;
	static inline const float kHeight = 0.8f;

	KamataEngine::Vector3 velocity_ = {};
	bool onGround_ = true;
	bool isDead_ = false; // 死亡フラグ

	float turnFirstRotationY_ = 0.0f;
	float turnTimer_ = 0.0f;

	KamataEngine::Camera* camera_ = nullptr;
	KamataEngine::Model* modelPlayer_ = nullptr;
	KamataEngine::WorldTransform worldTransform_;

	LRDirection lrDirection_ = LRDirection::kRight;
};