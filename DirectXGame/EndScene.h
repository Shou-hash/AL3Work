#pragma once
#include "Fade.h"
#include "Kamataengine.h"
#include "Skydome.h"
#include <memory>
#include <vector>

class StageManager;

class EndScene {

public:
	enum class Phase {
		FadeIn,  // フェードイン中
		Normal,  // 通常（キー入力待ち）
		FadeOut, // フェードアウト中
	};

	enum class MenuType {
		Return, // ゲームに戻る
		Retry,  // リトライ
		Title,  // タイトルに戻る
		Exit,   // ゲーム終了
		Count   // 項目の総数
	};

	~EndScene();

	void Initialize(StageManager* stageDataManager);
	void Update();
	void Draw();

	bool IsFinished() const { return finished_; }
	MenuType GetSelectedMenu() const { return selectedMenu_; }

private:
	bool finished_ = false;
	Phase phase_ = Phase::FadeIn;

	Fade* fade_ = nullptr;

	KamataEngine::Camera camera_{};

	// ★ モデルポインタ
	KamataEngine::Model* modelSkydome_ = nullptr;
	std::unique_ptr<Skydome> skydome = nullptr;

	// ステージマネージャ参照用のポインタ
	StageManager* stageManager_ = nullptr;

	// 現在選択されているメニュー項目
	MenuType currentSelect_ = MenuType::Return;
	// 最終的に確定されたメニュー項目
	MenuType selectedMenu_ = MenuType::Return;

	// メニュー選択画像用（white1x1.png）のスプライトポインタ配列
	static inline const int32_t kNumMenus = static_cast<int32_t>(MenuType::Count);
	KamataEngine::Sprite* menuSprites_[kNumMenus] = {nullptr};

	// 演出用変数
	float animationTimers_[kNumMenus] = {0.0f}; // 各項目のスケール補間用タイマー
	float flashTimer_ = 0.0f;                   // 選択中項目の明滅用タイマー
};