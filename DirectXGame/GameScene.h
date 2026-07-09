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
	unsigned int color;
};

struct Sparkle {
	KamataEngine::Vector2 position;
	KamataEngine::Vector2 velocity;
	float alpha;
	float lifeSpeed;
	bool isDisplay;
	float scale;
	KamataEngine::Sprite* sprite;
	unsigned int color;
};

struct Gear {
	KamataEngine::Vector2 position;
	float radius;
	float angle;
	float rotateSpeed;
	float size;
	int textureIndex;
	// アニメーション・揺れ用
	float startupTimer;
	bool isStadyRotation;
	float shakeAmount;
	float shakeSpeed;
};

class GameScene {
public:
	~GameScene();

	void Initialize();
	void ImGuiDraw();
	void Update();
	void Draw();

private:
	KamataEngine::Camera camera_{};

	// テクスチャハンドル
	uint32_t texBg_[3] = {0};
	uint32_t texClock_[3] = {0};
	uint32_t texHandHour_ = 0;
	uint32_t texHandMin_ = 0;
	uint32_t texPiece_[4] = {0};
	uint32_t texGear_[11] = {0};
	uint32_t texSparkle_ = 0;
	uint32_t texSun_ = 0;
	uint32_t texMoon_ = 0;

	uint32_t texSunLight_ = 0;
	uint32_t texSunLine_ = 0;
	uint32_t texMonthLight_ = 0;
	uint32_t texMonthLine_ = 0;
	uint32_t texBgLight1_ = 0;
	uint32_t texBgLight2_ = 0;
	uint32_t texSpace_[2] = {0};

	// スプライトポインタ
	KamataEngine::Sprite* sprBg_[3] = {nullptr};
	KamataEngine::Sprite* sprClock_[3] = {nullptr};
	KamataEngine::Sprite* sprHandHour_ = nullptr;
	KamataEngine::Sprite* sprHandMin_ = nullptr;
	KamataEngine::Sprite* sprGear_[11] = {nullptr};
	KamataEngine::Sprite* sprSun_ = nullptr;
	KamataEngine::Sprite* sprMoon_ = nullptr;
	KamataEngine::Sprite* sprSpace_[2] = {nullptr};

	KamataEngine::Sprite* sprSunLight_ = nullptr;
	KamataEngine::Sprite* sprSunLine_ = nullptr;
	KamataEngine::Sprite* sprMonthLight_ = nullptr;
	KamataEngine::Sprite* sprMonthLine_ = nullptr;
	KamataEngine::Sprite* sprBgLight1_ = nullptr;
	KamataEngine::Sprite* sprBgLight2_ = nullptr;

	bool isExpanding_ = false;               // 拡大中フラグ
	float scaleTimer1_ = 0.0f;               // 盤面1用のタイマー
	float scaleTimer2_ = 0.0f;               // 盤面2用のタイマー
	float currentScale1_ = 1.0f;             // 盤面1のスケール
	float currentScale2_ = 1.0f;             // 盤面2のスケール
	float currentScale_ = 1.0f;              // 盤面3や針などのベーススケール（1.0固定）
	const float kScaleSpeed_ = 1.0f / 20.0f; // アニメーション速度
	const float kMaxScale_ = 1.5f;           // スケール計算用の定数

	// ゲームロジック変数
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

	// 太陽・月の回転や拡縮のアニメーション用変数（必要に応じて）
	float sunAngle_ = 0.0f;
	float moonAngle_ = 0.0f;

	// ズーム用変数
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

	// 色変え処理用変数
	// 通常の色
	unsigned int lightBaseColor[5];
	// 目標色
	unsigned int lightTargetBlue[6];

	float colorLerpTimer = 0.0f;

	// 速度
	const float kColorChangeSpeed = 0.01f;

	// 色を線形補間する関数
	unsigned int LerpColor(unsigned int src, unsigned int dst, float t) {
		unsigned char srcR = (src >> 24) & 0xFF;
		unsigned char srcG = (src >> 16) & 0xFF;
		unsigned char srcB = (src >> 8) & 0xFF;
		unsigned char srcA = src & 0xFF;

		unsigned char dstR = (dst >> 24) & 0xFF;
		unsigned char dstG = (dst >> 16) & 0xFF;
		unsigned char dstB = (dst >> 8) & 0xFF;
		unsigned char dstA = dst & 0xFF;

		unsigned char r = (unsigned char)(srcR + (dstR - srcR) * t);
		unsigned char g = (unsigned char)(srcG + (dstG - srcG) * t);
		unsigned char b = (unsigned char)(srcB + (dstB - srcB) * t);
		unsigned char a = (unsigned char)(srcA + (dstA - srcA) * t);

		return (unsigned int)((r << 24) | (g << 16) | (b << 8) | a);
	};

	KamataEngine::Vector4 UintToVector4(unsigned int color) {
		KamataEngine::Vector4 result;
		// 各成分をビットシフトで取り出し、0.0f ～ 1.0f の範囲に変換
		result.x = ((color >> 24) & 0xFF) / 255.0f; // R
		result.y = ((color >> 16) & 0xFF) / 255.0f; // G
		result.z = ((color >> 8) & 0xFF) / 255.0f;  // B
		result.w = (color & 0xFF) / 255.0f;         // A
		return result;
	}

	// 演出管理用フラグ
	bool isEnableZoom_ = true;
	bool isEnableColorChange_ = true;
	bool isEnableScale_ = true;
	bool isEnableParticles_ = true;
	bool isEnableShake_ = true;
	bool isEnableGears_ = true;
	bool isEnableBlending_ = true;

	// 配列データ
	Piece pieces_[kPieceNum];
	Sparkle sparkles_[kSparkleNum];
	Gear gears_[kGearNum];

	// ヘルパー関数
	float Random(float min, float max);
};