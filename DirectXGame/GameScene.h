#pragma once
#include "KamataEngine.h"
#include <vector>

// 定数
const int kPieceNum = 100;
const int kSparkleNum = 50;
const int kGearNum = 24;
const int kPieceTexNum = 4;

struct Piece {
	KamataEngine::Vector2 position;
	KamataEngine::Vector2 velocity;
	float angle;
	float angularVelocity;
	int textureIndex;
	bool isDisplay;
	float width, height;
	float life;
	float maxLife;
	KamataEngine::Sprite* sprite;
};

struct Sparkle {
	KamataEngine::Vector2 position;
	KamataEngine::Vector2 velocity;
	float alpha;
	float lifeSpeed;
	bool isDisplay;
	float scale;
	KamataEngine::Sprite* sprite;
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
	KamataEngine::Sprite* sprGear_[11] = {nullptr};
	KamataEngine::Sprite* sprSun_ = nullptr;
	KamataEngine::Sprite* sprMoon_ = nullptr;
	KamataEngine::Sprite* sprSpace_[2] = {nullptr};

	bool isExpanding_ = false;               // 拡大中フラグ
	float scaleTimer1_ = 0.0f;               // 盤面1用のタイマー
	float scaleTimer2_ = 0.0f;               // 盤面2用のタイマー
	float currentScale1_ = 1.0f;             // 盤面1のスケール
	float currentScale2_ = 1.0f;             // 盤面2のスケール
	float currentScale_ = 1.0f;              // 盤面3や針などのベーススケール（1.0固定）
	const float kScaleSpeed_ = 1.0f / 20.0f; // アニメーション速度
	const float kMaxScale_ = 1.5f;           // スケール計算用の定数

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

	// Spaceガイドのアニメーション用タイマー
	int spaceAnimTimer_ = 0;

	// 追加: ズーム用変数
	float globalScale_ = 1.0f;                           // 現在の画面拡大率
	float zoomTimer_ = 0.0f;                             // ズームアニメーション用タイマー
	bool isZooming_ = false;                             // ズーム中フラグ
	const float kMaxZoom_ = 2.0f;                        // 最大何倍までズームするか
	const float kZoomSpeed_ = 0.025f;                    // ズームの速さ
	KamataEngine::Vector2 zoomTargetPos_ = {0.0f, 0.0f}; // ズームの中心（ターゲット）座標

	// 座標をズーム中心に合わせて変換する関数
	KamataEngine::Vector2 GetZoomPos(const KamataEngine::Vector2& pos) {
		KamataEngine::Vector2 result;
		result.x = zoomTargetPos_.x + (pos.x - zoomTargetPos_.x) * globalScale_;
		result.y = zoomTargetPos_.y + (pos.y - zoomTargetPos_.y) * globalScale_;
		return result;
	}

	// 配列データ
	Piece pieces_[kPieceNum];
	Sparkle sparkles_[kSparkleNum];
	Gear gears_[kGearNum];

	// ヘルパー関数
	float Random(float min, float max);
};