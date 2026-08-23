#include "EndScene.h"
#include "Matrix4x4.h"
#include "StageManager.h"
#include <cmath>
#include <numbers>
#ifdef _DEBUG
#include <imgui.h>
#endif

EndScene::~EndScene() {
	delete fade_;
	delete modelSkydome_;

	// スプライトの解放
	for (int32_t i = 0; i < kNumMenus; ++i) {
		delete menuSprites_[i];
	}
}

void EndScene::Initialize(StageManager* stageDataManager) {
	stageManager_ = stageDataManager;
	finished_ = false;
	phase_ = Phase::FadeIn;
	currentSelect_ = MenuType::Return;

	fade_ = new Fade();
	fade_->Initialize();
	fade_->Start(Fade::Status::FadeIn, 1.0f);

	camera_.Initialize();
	camera_.translation_ = {10.0f, 5.0f, -15.0f};

	// ★ スカイドームの初期化
	modelSkydome_ = KamataEngine::Model::CreateFromOBJ("skydome", true);
	skydome = std::make_unique<Skydome>();
	skydome->Initialize(modelSkydome_, &camera_);

	// スプライト（white1x1.png）のロードと配置初期化
	uint32_t textureHandle = KamataEngine::TextureManager::Load("white1x1.png");

	// 4つの項目を画面中央付近に縦並びで配置
	float startY = 200.0f;
	float spacing = 120.0f;
	float posX = 640.0f; // 画面中央
	float baseWidth = 300.0f;
	float baseHeight = 60.0f;

	for (int32_t i = 0; i < kNumMenus; ++i) {
		menuSprites_[i] = KamataEngine::Sprite::Create(textureHandle, {posX, startY + (i * spacing)});
		if (menuSprites_[i]) {
			menuSprites_[i]->SetSize({baseWidth, baseHeight});
			menuSprites_[i]->SetAnchorPoint({0.5f, 0.5f});
		}
		animationTimers_[i] = 0.0f;
	}
	flashTimer_ = 0.0f;

	// 初期選択項目のタイマーを満了（最大サイズ）状態にする
	animationTimers_[static_cast<int32_t>(currentSelect_)] = 1.0f;
}

void EndScene::Update() {
	fade_->Update();

	// ★ スカイドームの更新
	skydome->Update();

	camera_.UpdateMatrix();

	// 演出用の共通タイマーを進める
	flashTimer_ += 0.08f;

	// 各メニュー項目の演出パラメータ計算
	for (int32_t i = 0; i < kNumMenus; ++i) {
		if (i == static_cast<int32_t>(currentSelect_)) {
			// 選択している時の演出：滑らかに基準大サイズへ補間
			if (animationTimers_[i] < 1.0f) {
				animationTimers_[i] += 0.1f;
				if (animationTimers_[i] > 1.0f)
					animationTimers_[i] = 1.0f;
			}

			float scaleFactor = 0.8f + (animationTimers_[i] * 0.3f); // 0.8f -> 1.1f への補間
			if (menuSprites_[i]) {
				menuSprites_[i]->SetSize({300.0f * scaleFactor, 60.0f * scaleFactor});

				// サイン波でアルファ値を周期的に明滅させて目立たせる
				float alpha = 0.8f + std::sin(flashTimer_) * 0.2f;
				menuSprites_[i]->SetColor({0.7f, 1.0f, 0.7f, alpha}); // 選択中は薄緑色で強調
			}
		} else {
			// 止まっている時（非選択）の演出：速やかに縮小させ、暗めの半透明にする
			if (animationTimers_[i] > 0.0f) {
				animationTimers_[i] -= 0.1f;
				if (animationTimers_[i] < 0.0f)
					animationTimers_[i] = 0.0f;
			}

			float scaleFactor = 0.85f + (animationTimers_[i] * 0.25f);
			if (menuSprites_[i]) {
				menuSprites_[i]->SetSize({300.0f * scaleFactor, 60.0f * scaleFactor});
				menuSprites_[i]->SetColor({0.4f, 0.4f, 0.4f, 0.5f});
			}
		}
	}

	// シーンフェーズ管理
	switch (phase_) {
	case Phase::FadeIn:
		if (fade_->IsFinished())
			phase_ = Phase::Normal;
		break;
	case Phase::Normal:
		// 上下キーでメニュー選択を変更
		if (KamataEngine::Input::GetInstance()->TriggerKey(DIK_UP) || KamataEngine::Input::GetInstance()->TriggerKey(DIK_W)) {
			animationTimers_[static_cast<int32_t>(currentSelect_)] = 0.5f;

			int32_t nextSelect = static_cast<int32_t>(currentSelect_) - 1;
			if (nextSelect < 0) {
				nextSelect = kNumMenus - 1;
			}
			currentSelect_ = static_cast<MenuType>(nextSelect);
			animationTimers_[static_cast<int32_t>(currentSelect_)] = 0.0f;
		}
		if (KamataEngine::Input::GetInstance()->TriggerKey(DIK_DOWN) || KamataEngine::Input::GetInstance()->TriggerKey(DIK_S)) {
			animationTimers_[static_cast<int32_t>(currentSelect_)] = 0.5f;

			int32_t nextSelect = static_cast<int32_t>(currentSelect_) + 1;
			if (nextSelect >= kNumMenus) {
				nextSelect = 0;
			}
			currentSelect_ = static_cast<MenuType>(nextSelect);
			animationTimers_[static_cast<int32_t>(currentSelect_)] = 0.0f;
		}

		// 決定キー（SPACE）で項目を確定＆フェードアウト開始
		if (KamataEngine::Input::GetInstance()->PushKey(DIK_SPACE)) {
			selectedMenu_ = currentSelect_;
			fade_->Start(Fade::Status::FadeOut, 1.0f);
			phase_ = Phase::FadeOut;
		}
		break;
	case Phase::FadeOut:
		if (fade_->IsFinished())
			finished_ = true;
		break;
	}

#ifdef _DEBUG
	ImGui::Begin("End Scene");
	ImGui::Text("Select item with Up/Down arrow keys and press SPACE.");
	const char* menuNames[] = {"Select Scene", "Retry", "Title", "Exit"};
	ImGui::Text("Current Selection: %s", menuNames[static_cast<int32_t>(currentSelect_)]);
	ImGui::End();
#endif
}

void EndScene::Draw() {
	// ★ スカイドームの描画
	skydome->Draw();

	// メニュー選択画像の描画処理
	KamataEngine::Sprite::PreDraw();
	for (int32_t i = 0; i < kNumMenus; ++i) {
		if (menuSprites_[i]) {
			menuSprites_[i]->Draw();
		}
	}
	KamataEngine::Sprite::PostDraw();

	fade_->Draw();
}