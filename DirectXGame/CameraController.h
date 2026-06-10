#pragma once
#include "Kamataengine.h"

class Player;

struct Rect {
	float left = 0.0f;
	float right = 1.0f;
	float bottom = 0.0f;
	float top = 1.0f;
};

// カメラの移動モード
enum class CameraMode {
	kFollow,       // プレイヤー追従モード
	kForcedScroll, // 強制スクロールモード
};

class CameraController {
public:
	// 初期化時にカメラのポインタを受け取るように変更
	void Initialize(KamataEngine::Camera* camera);

	void Update();
	void Reset();

	// 追従対象の設定
	void SetTarget(Player* target) { target_ = target; }

	void SetMovableArea(const Rect& area) { movableArea_ = area; }

	// モード管理用のアクセッサ
	void SetMode(CameraMode mode) { mode_ = mode; }
	CameraMode GetMode() const { return mode_; }

	// 画面の左端座標を取得する（プレイヤーの押し出し・挟まれ判定用）
	float GetCameraLeftX() const;

private:
	// モードごとの更新処理
	void UpdateFollow();
	void UpdateForcedScroll();

	// プレイヤーを画面内に押し戻す処理
	void ConstrainPlayerInScreen();

private:
	KamataEngine::Camera* camera_ = nullptr;
	Player* target_ = nullptr;
	KamataEngine::Vector3 targetOffset_ = {0.0f, 0.5f, -15.0f};

	KamataEngine::Vector3 targetPosition_;

	static inline const float kInterpolationRate = 0.3f;
	static inline const float kVelocityBias = 5.0f;

	Rect movableArea_ = {5, 100, 0, 100};
	static inline const Rect margin_ = {-2.0f, 2.0f, -1.0f, 2.0f};

	// --- 追加パラメータ ---
	CameraMode mode_ = CameraMode::kFollow;             // 現在のカメラモード
	static inline const float kScrollSpeed = 0.03f;     // 強制スクロールの速度
	static inline const float kHalfScreenWidth = 10.0f; // 画面の中心から端までのワールド座標上の幅（調整可能）
};