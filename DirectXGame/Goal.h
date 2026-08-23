#pragma once
#include "KamataEngine.h"

class Goal {
public:
	struct AABB {
		KamataEngine::Vector3 min;
		KamataEngine::Vector3 max;
	};

	Goal();
	~Goal();

	void Initialize(KamataEngine::Model* model, KamataEngine::Camera* camera, const KamataEngine::Vector3& position);
	void Update();
	void Draw();

	AABB GetAABB() const;

	const KamataEngine::WorldTransform& GetWorldTransform() const { return worldTransform_; }

	KamataEngine::WorldTransform& GetWorldTransform() { return worldTransform_; }

	// ★ 位置変更用 Setter
	void SetPosition(const KamataEngine::Vector3& position) { worldTransform_.translation_ = position; }

	// ★ 個別の軸指定用（必要に応じて）
	void SetPositionX(float x) { worldTransform_.translation_.x = x; }
	void SetPositionY(float y) { worldTransform_.translation_.y = y; }
	void SetPositionZ(float z) { worldTransform_.translation_.z = z; }

	// ★ 有効/無効（表示・当たり判定）の設定
	void SetIsActive(bool isActive) { isActive_ = isActive; }
	bool IsActive() const { return isActive_; }

private:
	KamataEngine::Camera* camera_ = nullptr;
	KamataEngine::Model* model_ = nullptr;
	KamataEngine::WorldTransform worldTransform_;

	bool isActive_ = true; // ★追加：表示および当たり判定の有効フラグ

	static inline float kWidth = 1.0f;
	static inline float kHeight = 1.0f;
	static inline float kDepth = 1.0f;
};