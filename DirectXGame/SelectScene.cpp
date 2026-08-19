#include "SelectScene.h"
#include "Matrix4x4.h"
#include "StageManager.h"
#include <cmath>
#include <numbers>
#ifdef _DEBUG
#include <imgui.h>
#endif

SelectScene::~SelectScene() {
	delete fade_;
	delete modelSkydome_;

	// ★ スプライトの解放
	for (int32_t i = 0; i < kNumStages; ++i) {
		delete stageSprites_[i];
	}
}

void SelectScene::Initialize(StageManager* stageDataManager) {
	stageManager_ = stageDataManager;
	finished_ = false;
	phase_ = Phase::FadeIn;

	fade_ = new Fade();
	fade_->Initialize();
	fade_->Start(Fade::Status::FadeIn, 1.0f);

	camera_.Initialize();
	camera_.translation_ = {10.0f, 5.0f, -15.0f};

	modelSkydome_ = KamataEngine::Model::CreateFromOBJ("skydome", true);
	skydome = std::make_unique<Skydome>();
	skydome->Initialize(modelSkydome_, &camera_);

	if (stageManager_) {
		currentSelectIndex_ = stageManager_->GetCurrentStageIndex();
	}

	// ★ スプライト（white1x1.png）のロードと配置初期化
	// textureHandle=0、または特定のハンドルが割り当てられている場合はそちらを指定してください
	uint32_t textureHandle = KamataEngine::TextureManager::Load("white1x1.png");

	// 3つのステージ画像を画面中央付近に横並びで配置（画面サイズ 1280x720 想定）
	float startX = 340.0f;
	float spacing = 300.0f;
	float posY = 300.0f;
	float baseWidth = 200.0f;
	float baseHeight = 150.0f;

	for (int32_t i = 0; i < kNumStages; ++i) {
		stageSprites_[i] = KamataEngine::Sprite::Create(textureHandle, {startX + (i * spacing), posY});
		if (stageSprites_[i]) {
			stageSprites_[i]->SetSize({baseWidth, baseHeight});
			// 中心点をスプライト中央に設定してスケール変化の軸にする
			stageSprites_[i]->SetAnchorPoint({0.5f, 0.5f});
		}
		animationTimers_[i] = 0.0f;
	}
	flashTimer_ = 0.0f;

	// 初期選択ステージのタイマーを満了（最大サイズ）状態にしておく
	animationTimers_[currentSelectIndex_] = 1.0f;
}

void SelectScene::Update() {
	fade_->Update();
	skydome->Update();

	camera_.UpdateMatrix();

	// 演出用の共通タイマーを進める
	flashTimer_ += 0.08f;

	// 各ステージスプライトの演出パラメータ計算
	for (int32_t i = 0; i < kNumStages; ++i) {
		if (i == currentSelectIndex_) {
			// ★ 選択している時の演出：切り替わった瞬間はバンプ、通常時は滑らかに基準大サイズへ補間
			// タイマーを少しずつ進めて 1.0f に近づける
			if (animationTimers_[i] < 1.0f) {
				animationTimers_[i] += 0.1f;
				if (animationTimers_[i] > 1.0f)
					animationTimers_[i] = 1.0f;
			}

			// イージング風の処理で、切り替え時は一瞬飛び跳ねるように（1.25倍から1.1倍へ落とし込むような動きをシミュレート）
			float scaleFactor = 0.8f + (animationTimers_[i] * 0.3f); // 0.8f -> 1.1f への補間

			// 止まっている状態（選択中維持）ではサイン波による滑らかなサイズ微変動を加えることも可能ですが、
			// ここでは要求通り、止まっている（非選択）時と明確な差を出すために「目立つ明滅」を色情報として加えます
			if (stageSprites_[i]) {
				stageSprites_[i]->SetSize({200.0f * scaleFactor, 150.0f * scaleFactor});

				// サイン波でアルファ値（透明度）を 0.6f ～ 1.0f の間で周期的に点滅（明滅演出）させて目立たせる
				float alpha = 0.8f + std::sin(flashTimer_) * 0.2f;
				// 選択中は目立つように白ベース(1.0f, 1.0f, 1.0f)、非選択と色味でも差をつける
				stageSprites_[i]->SetColor({1.0f, 1.0f, 0.7f, alpha}); // 少し温かみのある白で強調
			}
		} else {
			// ★ 止まっている時（非選択）の演出：速やかに縮小（0.85倍）させ、暗めの半透明で固定する
			if (animationTimers_[i] > 0.0f) {
				animationTimers_[i] -= 0.1f;
				if (animationTimers_[i] < 0.0f)
					animationTimers_[i] = 0.0f;
			}

			float scaleFactor = 0.85f + (animationTimers_[i] * 0.25f); // 非選択時は 0.85f に収束
			if (stageSprites_[i]) {
				stageSprites_[i]->SetSize({200.0f * scaleFactor, 150.0f * scaleFactor});
				// 非選択ステージは暗く不透明度を下げる
				stageSprites_[i]->SetColor({0.4f, 0.4f, 0.4f, 0.5f});
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
		// 左右キーでステージ選択を変更
		if (KamataEngine::Input::GetInstance()->TriggerKey(DIK_LEFT) || KamataEngine::Input::GetInstance()->TriggerKey(DIK_A)) {
			// 古い選択肢のタイマーをバンプ用の中間値(0.5fなど)に落とし、縮小演出への繋ぎとする
			animationTimers_[currentSelectIndex_] = 0.5f;

			currentSelectIndex_--;
			if (currentSelectIndex_ < 0) {
				currentSelectIndex_ = 2; // ステージ3 (インデックス2) へループ
			}

			// 新しい選択肢のタイマーをリセットしてバンプ演出（0.0fから拡大開始）を誘発させる
			animationTimers_[currentSelectIndex_] = 0.0f;
		}
		if (KamataEngine::Input::GetInstance()->TriggerKey(DIK_RIGHT) || KamataEngine::Input::GetInstance()->TriggerKey(DIK_D)) {
			animationTimers_[currentSelectIndex_] = 0.5f;

			currentSelectIndex_++;
			if (currentSelectIndex_ > 2) {
				currentSelectIndex_ = 0; // ステージ1 (インデックス0) へループ
			}

			animationTimers_[currentSelectIndex_] = 0.0f;
		}

		// 決定キー（SPACE）でステージ確定＆フェードアウト開始
		if (KamataEngine::Input::GetInstance()->PushKey(DIK_SPACE)) {
			if (stageManager_) {
				stageManager_->SetCurrentStageIndex(currentSelectIndex_);
			}
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
	ImGui::Begin("Select Scene");
	ImGui::Text("Select Stage with Left/Right arrow keys and press SPACE to start.");
	if (stageManager_) {
		int stageCount = stageManager_->GetStageCount();
		if (ImGui::SliderInt("Select Stage Index", &currentSelectIndex_, 0, stageCount - 1)) {
			// スライダー操作時もタイマーをリセットして演出を走らせる
			for (int32_t i = 0; i < kNumStages; ++i) {
				if (i != currentSelectIndex_)
					animationTimers_[i] = 0.0f;
			}
			animationTimers_[currentSelectIndex_] = 0.0f;
		}
		ImGui::Text("Current Selection: %s", stageManager_->GetStageData(currentSelectIndex_).name.c_str());
	}
	ImGui::End();
#endif
}

void SelectScene::Draw() {
	skydome->Draw();

	// ★ ステージ選択画像（white1x1.png）の描画処理
	KamataEngine::Sprite::PreDraw();
	for (int32_t i = 0; i < kNumStages; ++i) {
		if (stageSprites_[i]) {
			stageSprites_[i]->Draw();
		}
	}
	KamataEngine::Sprite::PostDraw();

	fade_->Draw();
}