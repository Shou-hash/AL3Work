#pragma once
#include "KamataEngine.h"
#include <vector>

// 定数
const int kPieceNum = 100;
const int kSparkleNum = 50;
const int kGearNum = 24;

struct Piece {
	KamataEngine::Vector2 position;
	KamataEngine::Vector2 velocity;
	float angle;
	float angularVelocity;
	int textureIndex;
	bool isDisplay;
	float width, height;
	// --- 追加：生存時間 ---
	float life;
	float maxLife;
};

struct Sparkle {
	KamataEngine::Vector2 position;
	KamataEngine::Vector2 velocity;
	float alpha;
	float lifeSpeed;
	bool isDisplay;
	float scale;
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
	float shakeAmount;
	float shakeSpeed;
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
	uint32_t texSun_ = 0;
	uint32_t texMoon_ = 0;
	// --- 追加: Space操作ガイド用 ---
	uint32_t texSpace_[2] = {0};

	// --- スプライトポインタ ---
	KamataEngine::Sprite* sprBg_[3] = {nullptr};
	KamataEngine::Sprite* sprClock_[3] = {nullptr};
	KamataEngine::Sprite* sprHandHour_ = nullptr;
	KamataEngine::Sprite* sprHandMin_ = nullptr;
	KamataEngine::Sprite* sprPiece_[4] = {nullptr};
	KamataEngine::Sprite* sprGear_[11] = {nullptr};
	KamataEngine::Sprite* sprSparkle_ = nullptr;
	KamataEngine::Sprite* sprSun_ = nullptr;
	KamataEngine::Sprite* sprMoon_ = nullptr;
	KamataEngine::Sprite* sprSpace_[2] = {nullptr};

	// --- ゲームロジック変数 ---
	KamataEngine::Vector2 clockPos_ = {640.0f, 360.0f};
	float minAngle_ = -1.57f, hourAngle_ = -1.57f;
	float minTarget_ = -1.57f, hourTarget_ = -1.57f;
	float minStart_ = -1.57f, hourStart_ = -1.57f;

	float hourCos_ = 0.0f;

	bool isRotating_ = false;
	float easeTimer_ = 0.0f;
	int intervalTimer_ = 0;

	// シェイク
	float shakeTimer_ = 0.0f;
	KamataEngine::Vector2 shakeOffset_ = {0, 0};

	// 拡大縮小用変数 (パルスアニメーション)
	float scaleTimer_ = 0.0f;
	float currentScale_ = 1.0f;
	bool isExpanding_ = false;

	// Spaceガイドのアニメーション用タイマー
	int spaceAnimTimer_ = 0;

	// 配列データ
	Piece pieces_[kPieceNum];
	Sparkle sparkles_[kSparkleNum];
	Gear gears_[kGearNum];

	// ヘルパー関数
	float Random(float min, float max);
};