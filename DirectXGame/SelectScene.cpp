#include "SelectScene.h"
#include "AudioManager.h" // ★ BGM管理クラスのインクルード
#include "Matrix4x4.h"
#include "ModelCache.h"
#include "StageManager.h"
#include <algorithm>
#include <cmath>
#include <numbers>
#ifdef _DEBUG
#include <imgui.h>
#endif

SelectScene::~SelectScene() {
	delete fade_;

	// ★ 軽量化: モデルは ModelCache で使い回しているため、ここでは解放しない
}

void SelectScene::Initialize(StageManager* stageDataManager) {
	// ★ 軽量化: 前回の Initialize で確保したものを先に解放する（モデルは ModelCache で使い回すので解放しない）
	skydome.reset();
	delete fade_;
	fade_ = nullptr;

	pendingPauseMenu_ = Pause::MenuType::Count; // ポーズ選択項目の初期化

	// ★ セレクトBGMの再生
	AudioManager::GetInstance()->PlayBGM(BGMType::kSelect);

	stageManager_ = stageDataManager;
	finished_ = false;
	isEndSceneRequested_ = false;
	isTitleRequested_ = false; // ★ 遷移要求フラグの初期化
	isExitRequested_ = false;
	phase_ = Phase::FadeIn;

	fade_ = new Fade();
	fade_->Initialize();
	fade_->Start(Fade::Status::FadeIn, 1.0f);

	camera_.Initialize();
	camera_.translation_ = {10.0f, 5.0f, -15.0f};

	modelSkydome_ = ModelCache::Load("skydome");
	skydome = std::make_unique<Skydome>();
	skydome->Initialize(modelSkydome_, &camera_);

	if (stageManager_) {
		currentSelectIndex_ = stageManager_->GetCurrentStageIndex();
	}

	// ★ ステージ選択モデル（Stage1.obj, Stage2.obj, Stage3.obj）のロードと配置初期化
	stageSprites_[0] = ModelCache::Load("Stage1");
	stageSprites_[1] = ModelCache::Load("Stage2");
	stageSprites_[2] = ModelCache::Load("Stage3");

	// ★ 操作解説用矢印モデル（Arrow.obj）のロード
	modelArrow_ = ModelCache::Load("Arrow");

	// 3つのステージモデルをカメラ前方（ワールド空間）に横並びで配置
	float startX = 7.0f;
	float spacing = 3.0f;
	float posY = 5.0f;
	float posZ = -7.0f;

	for (int32_t i = 0; i < kNumStages; ++i) {
		worldTransforms_[i].Initialize();
		worldTransforms_[i].translation_ = {startX + (i * spacing), posY, posZ};
		animationTimers_[i] = 0.0f;

		// ★ 回転速度を全軸（またはY軸メイン）でランダムに設定 (例: 0.02f ～ 0.07f)
		float rx = 0.02f + (static_cast<float>(rand()) / RAND_MAX) * 0.05f;
		float ry = 0.03f + (static_cast<float>(rand()) / RAND_MAX) * 0.06f;
		float rz = 0.01f + (static_cast<float>(rand()) / RAND_MAX) * 0.04f;

		rotationSpeeds_[i] = {rx, ry, rz};
	}
	flashTimer_ = 0.0f;

	// ★ 左右の操作解説矢印の配置初期化（0: 左端矢印, 1: 右端矢印）
	for (int32_t i = 0; i < 2; ++i) {
		arrowTransforms_[i].Initialize();
		arrowAnimationTimers_[i] = 0.0f;
		arrowTransforms_[i].scale_ = {1.0f, 1.0f, 1.0f};
	}

	float arrowPosY = posY - 2.0f;
	arrowTransforms_[0].translation_ = {6.5f, arrowPosY, posZ};  // 左矢印
	arrowTransforms_[1].translation_ = {13.5f, arrowPosY, posZ}; // 右矢印

	// 矢印モデルの向き（0: 左向き Z=0, 1: 右向き Z=180度）
	arrowTransforms_[0].rotation_ = {0.0f, 0.0f, 0.0f};
	arrowTransforms_[1].rotation_ = {0.0f, 0.0f, std::numbers::pi_v<float>};

	for (int32_t i = 0; i < 2; ++i) {
		KamataEngine::Matrix4x4 arrowMatrix = MakeAffineMatrix(arrowTransforms_[i].scale_, arrowTransforms_[i].rotation_, arrowTransforms_[i].translation_);
		arrowTransforms_[i].matWorld_ = arrowMatrix;
		arrowTransforms_[i].TransferMatrix();
	}

	// 初期選択ステージのタイマーを満了状態にしておく
	animationTimers_[currentSelectIndex_] = 1.0f;

	// ★ ライティング初期化
	lightManager_ = std::make_unique<LightManager>();
	lightManager_->Initialize();
	lightManager_->ApplyToModel(modelSkydome_, LightManager::GroupType::kWorld);
	for (int32_t i = 0; i < kNumStages; ++i) {
		lightManager_->ApplyToModel(stageSprites_[i], LightManager::GroupType::kWorld);
	}
	if (modelArrow_) {
		lightManager_->ApplyToModel(modelArrow_, LightManager::GroupType::kWorld);
	}

	// ★ ライティング切替演出変数の初期化
	previousSelectIndex_ = currentSelectIndex_;
	if (currentSelectIndex_ == 2) {
		flickerTimer_ = 0.0f;
		currentInterval_ = 0.06f; // 最初の切替間隔（約0.06秒＝高速切替）
		isNight_ = true;
		isFlickerFinished_ = false;
	} else {
		isNight_ = false;
		isFlickerFinished_ = true;
	}

	// ★ フォント演出・ポーズUIクラスの初期化
	font_ = std::make_unique<Font>();
	font_->Initialize(&camera_);
	if (lightManager_) {
		font_->ApplyLightManager(*lightManager_);
	}

	// ★ セレクトシーン用ポーズクラスの初期化（セレクト画面モード Title/Exit の2項目）
	pause_ = std::make_unique<Pause>();
	pause_->Initialize(&camera_);
	pause_->SetMode(Pause::Mode::Select);                       // セレクト画面モードに設定
	pause_->SetScale({0.085f, 0.07f, 0.07f});                   // スケールをさらに小さく設定
	pause_->SetOffsets({0.0f, 8.0f, 3.5f}, {0.0f, 0.0f, 3.5f}); // Z軸距離を 8.0f -> 3.5f にして手前へ移動
	if (lightManager_) {
		pause_->ApplyLightManager(*lightManager_);
	}
}

void SelectScene::Update() {
	// ★ 追加: ポーズクラスの更新（ESC押下判定と看板の降り/上がりアニメーション）
	if (pause_) {
		pause_->Update();

		// ポーズメニュー項目確定時の画面遷移処理
		if (pause_->IsMenuSelected()) {
			Pause::MenuType selected = pause_->GetSelectedMenu();
			pause_->ResetMenuSelection();
			pause_->Close(); // 看板を閉じる

			// ★ 修正: 選択項目を保持してフェードアウトを開始
			pendingPauseMenu_ = selected;
			phase_ = Phase::FadeOut;
			if (fade_) {
				fade_->Start(Fade::Status::FadeOut, 0.8f);
			}
		}
	}

	// ★ 追加: ポーズ中（看板展開中・完全停止中）はセレクトシーンの全挙動を一時停止（フェードアウト中は除く）
	if (pause_ && pause_->IsPaused() && phase_ != Phase::FadeOut) {
		return;
	}

	fade_->Update();
	skydome->Update();

	camera_.UpdateMatrix();

	// ★ ステージ選択切り替えの検出とステージ2専用ライティング演出のリセット
	if (currentSelectIndex_ != previousSelectIndex_) {
		if (currentSelectIndex_ == 2) {
			// ステージ2が選択された時：高速交互切替（フリッカー）演出を開始
			flickerTimer_ = 0.0f;
			currentInterval_ = 0.06f; // 最初は速く切替（0.06秒間隔）
			isNight_ = true;
			isFlickerFinished_ = false;
		} else {
			// ステージ0・1が選択された時：即座に朝のライティングに戻す
			isNight_ = false;
			isFlickerFinished_ = true;
		}
		previousSelectIndex_ = currentSelectIndex_;
	}

	// ★ ステージ2選択時の「朝 ⇔ 夜」ライティング交互切替（フリッカー）演出更新
	if (currentSelectIndex_ == 2) {
		if (!isFlickerFinished_) {
			flickerTimer_ += 1.0f / 60.0f;
			if (flickerTimer_ >= currentInterval_) {
				flickerTimer_ = 0.0f;
				isNight_ = !isNight_; // 朝と夜を交互にトグル

				// 切替間隔を段々遅くする（1.5倍ずつ間隔が伸びる）
				currentInterval_ *= 1.5f;

				// 切替間隔が一定以上（0.7秒以上）になり、かつ「夜」の状態で演出を終了して夜に固定
				if (currentInterval_ >= 0.7f && isNight_) {
					isFlickerFinished_ = true;
				}
			}
		}
		lightManager_->SetNightFactor(isNight_ ? 1.0f : 0.0f);
		lightManager_->SetLightsEnabled(isNight_); // 夜のスポットライト演出も連動
	} else {
		// ステージ0・1選択時は標準の朝のライティング演出
		lightManager_->SetNightFactor(0.0f);
		lightManager_->SetLightsEnabled(false);
	}

	// ★ ライティングの更新
	lightManager_->Update();

	// ★ ポーズUIの更新とESCキー検出
	if (font_) {
		font_->Update();
		if (font_->IsEndSceneRequested()) {
			isEndSceneRequested_ = true;
		}
	}

	// 演出用の共通タイマーを進める
	flashTimer_ += 0.08f;

	// 各ステージモデルの演出パラメータ計算
	for (int32_t i = 0; i < kNumStages; ++i) {
		if (i == currentSelectIndex_) {
			// ★ 選択中：設定されたランダム速度で回転を加算
			worldTransforms_[i].rotation_.x += rotationSpeeds_[i].x;
			worldTransforms_[i].rotation_.y += rotationSpeeds_[i].y;
			worldTransforms_[i].rotation_.z += rotationSpeeds_[i].z;

			if (animationTimers_[i] < 1.0f) {
				animationTimers_[i] += 0.1f;
				if (animationTimers_[i] > 1.0f)
					animationTimers_[i] = 1.0f;
			}

			float scaleFactor = 0.8f + (animationTimers_[i] * 0.3f);
			worldTransforms_[i].scale_ = {scaleFactor, scaleFactor, scaleFactor};
		} else {
			// ★ 非選択時：回転角度を徐々に 0 (正面) に向けて衰減・復帰
			worldTransforms_[i].rotation_.x *= 0.85f;
			worldTransforms_[i].rotation_.y *= 0.85f;
			worldTransforms_[i].rotation_.z *= 0.85f;

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

	// ★ 左右矢印のキー押下による Scale アニメーションの更新
	for (int32_t i = 0; i < 2; ++i) {
		if (arrowAnimationTimers_[i] > 0.0f) {
			arrowAnimationTimers_[i] -= 0.12f;
			if (arrowAnimationTimers_[i] < 0.0f) {
				arrowAnimationTimers_[i] = 0.0f;
			}
		}

		// 押した瞬間にポンッと拡大して滑らかに元の大きさに戻るアニメーション
		float scaleFactor = 1.0f + std::sin(arrowAnimationTimers_[i] * std::numbers::pi_v<float>) * 0.45f;
		arrowTransforms_[i].scale_ = {scaleFactor, scaleFactor, scaleFactor};

		KamataEngine::Matrix4x4 arrowMatrix = MakeAffineMatrix(arrowTransforms_[i].scale_, arrowTransforms_[i].rotation_, arrowTransforms_[i].translation_);
		arrowTransforms_[i].matWorld_ = arrowMatrix;
		arrowTransforms_[i].TransferMatrix();
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
			animationTimers_[currentSelectIndex_] = 0.5f;

			currentSelectIndex_--;
			if (currentSelectIndex_ < 0) {
				currentSelectIndex_ = 2;
			}

			animationTimers_[currentSelectIndex_] = 0.0f;
			arrowAnimationTimers_[0] = 1.0f; // ★ 左矢印のアニメーション開始

			// ★ ステージ選択SE再生
			AudioManager::GetInstance()->PlaySE(SEType::kStage);
		}
		if (KamataEngine::Input::GetInstance()->TriggerKey(DIK_RIGHT) || KamataEngine::Input::GetInstance()->TriggerKey(DIK_D)) {
			animationTimers_[currentSelectIndex_] = 0.5f;

			currentSelectIndex_++;
			if (currentSelectIndex_ > 2) {
				currentSelectIndex_ = 0;
			}

			animationTimers_[currentSelectIndex_] = 0.0f;
			arrowAnimationTimers_[1] = 1.0f; // ★ 右矢印のアニメーション開始

			// ★ ステージ選択SE再生
			AudioManager::GetInstance()->PlaySE(SEType::kStage);
		}

		// ★ 決定キー（SPACE）でステージ確定 & カメラ演出開始
		if (KamataEngine::Input::GetInstance()->PushKey(DIK_SPACE)) {
			if (stageManager_) {
				stageManager_->SetCurrentStageIndex(currentSelectIndex_);
			}

			// カメラ演出の初期設定
			phase_ = Phase::SelectPerformance;
			cameraPerformanceTimer_ = 0.0f;
			cameraStartPos_ = camera_.translation_;

			// 選択されたステージモデルの位置に寄る目標座標を計算
			const KamataEngine::Vector3& stagePos = worldTransforms_[currentSelectIndex_].translation_;
			cameraTargetPos_ = {stagePos.x, stagePos.y, stagePos.z - 3.8f};

			// ★ ステージ確定SE再生
			AudioManager::GetInstance()->PlaySE(SEType::kStage);
		}
		break;

	case Phase::SelectPerformance: {
		// ★ 確定時のステージモデルへのカメラ接近演出
		cameraPerformanceTimer_ += 1.0f / 60.0f;
		constexpr float kPerformanceDuration = 0.8f;
		float t = std::clamp(cameraPerformanceTimer_ / kPerformanceDuration, 0.0f, 1.0f);

		// イージング (EaseOutCubic) で滑らかに接近
		float easeVal = 1.0f - std::pow(1.0f - t, 3.0f);

		camera_.translation_.x = cameraStartPos_.x + (cameraTargetPos_.x - cameraStartPos_.x) * easeVal;
		camera_.translation_.y = cameraStartPos_.y + (cameraTargetPos_.y - cameraStartPos_.y) * easeVal;
		camera_.translation_.z = cameraStartPos_.z + (cameraTargetPos_.z - cameraStartPos_.z) * easeVal;

		// 選択されたステージモデルの回転速度を加速させてアピール演出
		worldTransforms_[currentSelectIndex_].rotation_.y += 0.12f;

		// カメラ演出が完了したらフェードアウト開始
		if (t >= 1.0f) {
			fade_->Start(Fade::Status::FadeOut, 0.8f);
			phase_ = Phase::FadeOut;
		}
		break;
	}

	case Phase::FadeOut:
		if (fade_->IsFinished()) {
			// ★ 修正: ポーズからの遷移要求に応じて各フラグを立てる
			if (pendingPauseMenu_ == Pause::MenuType::Title) {
				isTitleRequested_ = true;
			} else if (pendingPauseMenu_ == Pause::MenuType::Exit) {
				isExitRequested_ = true;
			} else {
				finished_ = true;
			}
		}
		break;
	}

#ifdef _DEBUG
	ImGui::Begin("Select Scene");
	ImGui::Text("Select Stage with Left/Right arrow keys and press SPACE to start.");
	if (stageManager_) {
		int stageCount = stageManager_->GetStageCount();
		if (ImGui::SliderInt("Select Stage Index", &currentSelectIndex_, 0, stageCount - 1)) {
			for (int32_t i = 0; i < kNumStages; ++i) {
				if (i != currentSelectIndex_)
					animationTimers_[i] = 0.0f;
			}
			animationTimers_[currentSelectIndex_] = 0.0f;

			// ★ ステージ選択SE再生
			AudioManager::GetInstance()->PlaySE(SEType::kStage);
		}
		ImGui::Text("Current Selection: %s", stageManager_->GetStageData(currentSelectIndex_).name.c_str());
	}

	// ★ ライティング（光沢の強さなど）の調整
	lightManager_->DrawImGui();
	ImGui::End();
#endif
}

void SelectScene::Draw() {
	skydome->Draw();

	// ★ ステージ選択モデル（Stage1.obj, Stage2.obj, Stage3.obj）の描画処理
	KamataEngine::Model::PreDraw();
	for (int32_t i = 0; i < kNumStages; ++i) {
		if (stageSprites_[i]) {
			stageSprites_[i]->Draw(worldTransforms_[i], camera_);
		}
	}

	// ★ 左右端の操作解説矢印モデルの描画処理
	if (modelArrow_) {
		for (int32_t i = 0; i < 2; ++i) {
			modelArrow_->Draw(arrowTransforms_[i], camera_);
		}
	}
	KamataEngine::Model::PostDraw();

	// ★ ポーズUI（Pauseモデル、ESCモデル）の描画処理
	if (font_) {
		KamataEngine::Model::PreDraw();
		font_->DrawPauseUI();
		KamataEngine::Model::PostDraw();
	}

	// ★ ポーズ看板モデル（PictureFrame）の描画
	if (pause_) {
		KamataEngine::Model::PreDraw();
		pause_->Draw();
		KamataEngine::Model::PostDraw();
	}

	fade_->Draw();
}