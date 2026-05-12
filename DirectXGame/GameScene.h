#pragma once
#include "KamataEngine.h"
#include <vector>

// 定数
const int kPieceNum = 100;
const int kSparkleNum = 50;
const int kGearNum = 24;

// 構造体定義（すべてKamataEngine::Vector2を使用）
struct Piece {
	KamataEngine::Vector2 position;
	KamataEngine::Vector2 velocity;
	float angle;
	float angularVelocity;
	int textureIndex;
	bool isDisplay;
	float width, height;
};

struct Sparkle {
	KamataEngine::Vector2 position;
	KamataEngine::Vector2 velocity;
	float alpha;
	float lifeSpeed;
	bool isDisplay;
};

struct Gear {
	KamataEngine::Vector2 position;
	float radius;
	float angle;
	float rotateSpeed;
	float size;
	int textureIndex;
	// --- 追加: アニメーション・揺れ用 ---
	float startupTimer;
	bool isStadyRotation;
};

class GameScene {
public:
	~GameScene();

	void Initialize();
	void Update();
	void Draw();

private:
	KamataEngine::Camera camera_{};

	// --- テクスチャハンドル ---
	uint32_t texBg_[3] = {0};
	uint32_t texClock_[3] = {0};
	uint32_t texHandHour_ = 0;
	uint32_t texHandMin_ = 0;
	uint32_t texPiece_[4] = {0};
	uint32_t texGear_[11] = {0};
	uint32_t texSparkle_ = 0;
	// --- 追加: 太陽と月のテクスチャ ---
	uint32_t texSun_ = 0;
	uint32_t texMoon_ = 0;

	// --- スプライトポインタ ---
	KamataEngine::Sprite* sprBg_[3] = {nullptr};
	KamataEngine::Sprite* sprClock_[3] = {nullptr};
	KamataEngine::Sprite* sprHandHour_ = nullptr;
	KamataEngine::Sprite* sprHandMin_ = nullptr;
	KamataEngine::Sprite* sprPiece_[4] = {nullptr};
	KamataEngine::Sprite* sprGear_[11] = {nullptr};
	KamataEngine::Sprite* sprSparkle_ = nullptr;
	// --- 追加: 太陽と月のスプライト ---
	KamataEngine::Sprite* sprSun_ = nullptr;
	KamataEngine::Sprite* sprMoon_ = nullptr;

	// --- ゲームロジック変数 ---
	KamataEngine::Vector2 clockPos_ = {640.0f, 360.0f};
	float minAngle_ = -1.57f, hourAngle_ = -1.57f;
	float minTarget_ = -1.57f, hourTarget_ = -1.57f;
	float minStart_ = -1.57f, hourStart_ = -1.57f;

	bool isRotating_ = false;
	float easeTimer_ = 0.0f;
	int intervalTimer_ = 0;

	// シェイク
	float shakeTimer_ = 0.0f;
	KamataEngine::Vector2 shakeOffset_ = {0, 0};

	// 色変え・演出用変数
	bool isSunActive_ = true;
	float colorLerpTimer_ = 0.0f;
	const float kColorChangeSpeed_ = 0.02f;

	// 配列データ
	Piece pieces_[kPieceNum];
	Sparkle sparkles_[kSparkleNum];
	Gear gears_[kGearNum];

	// ヘルパー関数
	KamataEngine::Vector2 GetZoomPos(float x, float y);
	float Random(float min, float max);
};