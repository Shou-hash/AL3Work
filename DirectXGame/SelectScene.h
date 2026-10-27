#pragma once
#include "Fade.h"
#include "Font.h"
#include "Kamataengine.h"
#include "LightManager.h"
#include "Pause.h" // ★ 追加: ポーズクラスのインクルード
#include "Skydome.h"
#include <memory>
#include <vector>

class StageManager;

class SelectScene {

public:
	enum class Phase {
		FadeIn,            // フェードイン中
		Normal,            // 通常（キー入力待ち）
		SelectPerformance, // ★ 追加: 確定時のカメラ演出中
		FadeOut,           // フェードアウト中
	};

	~SelectScene();

	void Initialize(StageManager* stageDataManager);
	void Update();
	void Draw();

	bool IsFinished() const { return finished_; }
	bool IsEndSceneRequested() const { return isEndSceneRequested_; }
	bool IsTitleRequested() const { return isTitleRequested_; } // ★ 追加: タイトル遷移要求フラグを取得
	bool IsExitRequested() const { return isExitRequested_; }   // ★ 追加: ゲーム終了要求フラグを取得

private:
	bool finished_ = false;
	bool isEndSceneRequested_ = false;
	bool isTitleRequested_ = false; // ★ 追加: タイトル画面への遷移要求フラグ
	bool isExitRequested_ = false;  // ★ 追加: ゲーム終了の要求フラグ
	Phase phase_ = Phase::FadeIn;

	Fade* fade_ = nullptr;

	KamataEngine::Camera camera_{};

	// モデルポインタ
	KamataEngine::Model* modelSkydome_ = nullptr;
	std::unique_ptr<Skydome> skydome = nullptr;

	// ★ 操作解説用矢印モデルポインタ
	KamataEngine::Model* modelArrow_ = nullptr;

	// ★ ライティングマネージャ
	std::unique_ptr<LightManager> lightManager_ = nullptr;

	// ★ フォント演出・ポーズUIクラスのインスタンス
	std::unique_ptr<Font> font_ = nullptr;

	// ★ 追加: ポーズUI・看板演出制御専用クラスのインスタンス
	std::unique_ptr<Pause> pause_ = nullptr;

	// ステージマネージャ参照用のポインタ
	StageManager* stageManager_ = nullptr;

	// 選択中のステージインデックス
	int32_t currentSelectIndex_ = 0;

	// ★ ステージ選択モデル用（Stage1.obj, Stage2.obj, Stage3.obj）のモデルポインタ配列
	static inline const int32_t kNumStages = 3;
	KamataEngine::Model* stageSprites_[kNumStages] = {nullptr};
	KamataEngine::WorldTransform worldTransforms_[kNumStages];

	// ★ 左右矢印用 WorldTransform（0: 左矢印, 1: 右矢印）
	KamataEngine::WorldTransform arrowTransforms_[2];
	float arrowAnimationTimers_[2] = {0.0f, 0.0f}; // 矢印押下時のスケールアニメーションタイマー

	// ★ 演出用変数
	float animationTimers_[kNumStages] = {0.0f}; // 各ステージのスケール補間用タイマー
	float flashTimer_ = 0.0f;                    // 選択中ステージの明滅用タイマー

	// ★ 各ステージモデルのランダム回転速度（X, Y, Z軸）
	KamataEngine::Vector3 rotationSpeeds_[kNumStages] = {};

	// ★ 確定時のカメラ演出用変数
	float cameraPerformanceTimer_ = 0.0f;
	KamataEngine::Vector3 cameraStartPos_{};
	KamataEngine::Vector3 cameraTargetPos_{};

	// ★ 追加: ポーズ看板からの選択項目保持用変数
	Pause::MenuType pendingPauseMenu_ = Pause::MenuType::Count;

	// ★ 追加: ステージ2選択時のライティング朝夜交互切替（フリッカー）演出用変数
	int32_t previousSelectIndex_ = -1; // 直前の選択ステージインデックス
	float flickerTimer_ = 0.0f;        // 切替タイマー
	float currentInterval_ = 0.06f;    // 現在の切替間隔（秒。最初は速く段々遅くなる）
	bool isNight_ = false;             // 現在のライティング状態（false: 朝, true: 夜）
	bool isFlickerFinished_ = false;   // 切替演出が終了して夜に固定されたか
};