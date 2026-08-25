#include "EndScene.h"
#include "AudioManager.h" // ★ BGM管理クラスのインクルード
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

	// ★ モデルの解放
	for (int32_t i = 0; i < kNumMenus; ++i) {
		delete menuSprites_[i];
	}
}

void EndScene::Initialize(StageManager* stageDataManager) {
	// ★ エンドBGMの再生
	AudioManager::GetInstance()->PlayBGM(BGMType::kEnd);

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

	// メニュー選択モデル（Return.obj, Retry.obj, Title.obj, Exit.obj）のロードと配置初期化
	menuSprites_[0] = KamataEngine::Model::CreateFromOBJ("Return", true);
	menuSprites_[1] = KamataEngine::Model::CreateFromOBJ("Retry", true);
	menuSprites_[2] = KamataEngine::Model::CreateFromOBJ("Title", true);
	menuSprites_[3] = KamataEngine::Model::CreateFromOBJ("Exit", true);

	// 4つの項目を画面中央付近に縦並びで配置
	float startY = 6.5f;
	float spacing = 1.0f;
	float posX = 10.0f;
	float posZ = -7.0f;

	for (int32_t i = 0; i < kNumMenus; ++i) {
		worldTransforms_[i].Initialize();
		worldTransforms_[i].translation_ = {posX, startY - (i * spacing), posZ};
		worldTransforms_[i].rotation_.y = std::numbers::pi_v<float>; // ★ Y軸回転を180度（πラジアン）に設定
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

			float scaleFactor = 0.8f + (animationTimers_[i] * 0.3f);
			worldTransforms_[i].scale_ = {scaleFactor, scaleFactor, scaleFactor};
		} else {
			// 止まっている時（非選択）の演出：速やかに縮小させる
			if (animationTimers_[i] > 0.0f) {
				animationTimers_[i] -= 0.1f;
				if (animationTimers_[i] < 0.0f)
					animationTimers_[i] = 0.0f;
			}

			float scaleFactor = 0.85f + (animationTimers_[i] * 0.25f);
			worldTransforms_[i].scale_ = {scaleFactor, scaleFactor, scaleFactor};
		}

		// ワールド行列の更新
		KamataEngine::Matrix4x4 affineMatrix = MakeAffineMatrix(worldTransforms_[i].scale_, worldTransforms_[i].rotation_, worldTransforms_[i].translation_);
		worldTransforms_[i].matWorld_ = affineMatrix;
		worldTransforms_[i].TransferMatrix();
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

	// ★ 各ボタンの Rotation 調整用ツリー
	if (ImGui::TreeNode("Menu Rotations")) {
		for (int32_t i = 0; i < kNumMenus; ++i) {
			ImGui::PushID(i);
			float* rot = &(worldTransforms_[i].rotation_.x);
			ImGui::DragFloat3(menuNames[i], rot, 0.01f);
			ImGui::PopID();
		}
		ImGui::TreePop();
	}

	ImGui::End();
#endif
}

void EndScene::Draw() {
	// ★ スカイドームの描画
	skydome->Draw();

	// メニュー選択モデルの描画処理
	KamataEngine::Model::PreDraw();
	for (int32_t i = 0; i < kNumMenus; ++i) {
		if (menuSprites_[i]) {
			menuSprites_[i]->Draw(worldTransforms_[i], camera_);
		}
	}
	KamataEngine::Model::PostDraw();

	fade_->Draw();
}