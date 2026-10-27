#include "Pause.h"
#include "AudioManager.h"
#include "LightManager.h"
#include "Matrix4x4.h"
#include <algorithm>
#include <cmath>
#include <numbers>

using namespace KamataEngine;

Pause::~Pause() {
	delete modelPictureFrame_;
	delete modelMoveExplanation_;
	delete modelAttackExplanation_;
	delete modelDashExplanation_;

	// ★ メニュー選択モデルの解放
	for (int32_t i = 0; i < kNumMenus; ++i) {
		delete menuModels_[i];
	}
}

void Pause::Initialize(Camera* camera) {
	camera_ = camera;

	// ★ 看板モデル "PictureFrame" の読み込み
	modelPictureFrame_ = Model::CreateFromOBJ("PictureFrame", true);

	// ★ 操作説明用モデルの読み込み（MoveExplanation, AttackExplanation, DashExplanation）
	modelMoveExplanation_ = Model::CreateFromOBJ("MoveExplanation", true);
	modelAttackExplanation_ = Model::CreateFromOBJ("AttackExplanation", true);
	modelDashExplanation_ = Model::CreateFromOBJ("DashExplanation", true);

	// ★ メニュー選択用モデルの読み込み（Return, Retry, Title, Exit）
	menuModels_[0] = Model::CreateFromOBJ("Return", true);
	menuModels_[1] = Model::CreateFromOBJ("Retry", true);
	menuModels_[2] = Model::CreateFromOBJ("Title", true);
	menuModels_[3] = Model::CreateFromOBJ("Exit", true);

	worldTransform_.Initialize();
	// ★ デフォルトのScale設定
	worldTransform_.scale_ = {0.5f, 0.5f, 0.5f};
	worldTransform_.rotation_ = {0.0f, std::numbers::pi_v<float>, 0.0f};

	// ★ 操作説明用 WorldTransform の初期化
	worldTransformMove_.Initialize();
	worldTransformAttack_.Initialize();
	worldTransformDash_.Initialize();

	// ★ メニュー選択用 WorldTransform の初期化
	for (int32_t i = 0; i < kNumMenus; ++i) {
		menuWorldTransforms_[i].Initialize();
		menuWorldTransforms_[i].rotation_ = {0.0f, std::numbers::pi_v<float>, 0.0f};
		menuAnimationTimers_[i] = 0.0f;
	}

	isPaused_ = false;
	isTargetPaused_ = false;
	animTimer_ = 0.0f;

	mode_ = Mode::Game;
	currentSelectIndex_ = 0;
	selectedMenu_ = MenuType::Return;
	isMenuSelected_ = false;

	// ★ 各操作説明モデルの個別の拡大倍率(1.5倍)と左上寄りの相対オフセット(間隔狭め)の初期設定
	scaleMultiplierMove_ = {8.5f, 10.5f, 10.5f};
	scaleMultiplierAttack_ = {8.5f, 10.5f, 10.5f};
	scaleMultiplierDash_ = {8.5f, 10.5f, 10.5f};

	offsetMove_ = {-1.5f, 0.65f, -0.05f};
	offsetAttack_ = {-1.6f, 0.30f, -0.05f};
	offsetDash_ = {-1.2f, -0.15f, -0.05f};
}

void Pause::SetMode(Mode mode) {
	mode_ = mode;
	currentSelectIndex_ = 0;
}

void Pause::ApplyLightManager(const LightManager& lightManager) {
	if (modelPictureFrame_) {
		lightManager.ApplyToModel(modelPictureFrame_, LightManager::GroupType::kWorld);
	}
	if (modelMoveExplanation_) {
		lightManager.ApplyToModel(modelMoveExplanation_, LightManager::GroupType::kWorld);
	}
	if (modelAttackExplanation_) {
		lightManager.ApplyToModel(modelAttackExplanation_, LightManager::GroupType::kWorld);
	}
	if (modelDashExplanation_) {
		lightManager.ApplyToModel(modelDashExplanation_, LightManager::GroupType::kWorld);
	}

	// ★ メニュー選択モデルへのライティング適用
	for (int32_t i = 0; i < kNumMenus; ++i) {
		if (menuModels_[i]) {
			lightManager.ApplyToModel(menuModels_[i], LightManager::GroupType::kWorld);
		}
	}
}

void Pause::Update() {
	float dt = 1.0f / 60.0f;

	// ★ ESCキー押下でポーズ状態をトグル切り替え
	if (Input::GetInstance()->TriggerKey(DIK_ESCAPE)) {
		isTargetPaused_ = !isTargetPaused_;
		if (isTargetPaused_) {
			// ポーズ開始時にメニュー選択をデフォルトに戻す
			currentSelectIndex_ = 0;
			isMenuSelected_ = false;
			for (int32_t i = 0; i < kNumMenus; ++i) {
				menuAnimationTimers_[i] = (i == 0) ? 1.0f : 0.0f;
			}
		}
	}

	// ★ タイマー進行およびポーズフラグ更新
	if (isTargetPaused_) {
		if (animTimer_ < kAnimDuration) {
			animTimer_ += dt;
			if (animTimer_ > kAnimDuration) {
				animTimer_ = kAnimDuration;
			}
		}
		// 看板が降り始めている間〜展開中はゲーム挙動を停止する
		isPaused_ = true;
	} else {
		if (animTimer_ > 0.0f) {
			animTimer_ -= dt;
			if (animTimer_ < 0.0f) {
				animTimer_ = 0.0f;
			}
		} else {
			// 看板が完全に画面外へ上がりきったらポーズ解除
			isPaused_ = false;
		}
	}

	// モードに応じた有効メニュー項目数の取得
	const int32_t activeMenuCount = (mode_ == Mode::Select) ? 2 : 4;

	// ★ ポーズ展開中のメニュー選択操作（左右キー / A・Dキー / SPACE・ENTER確定）
	if (isPaused_ && isTargetPaused_ && animTimer_ >= kAnimDuration * 0.5f) {
		if (Input::GetInstance()->TriggerKey(DIK_LEFT) || Input::GetInstance()->TriggerKey(DIK_A)) {
			currentSelectIndex_--;
			if (currentSelectIndex_ < 0) {
				currentSelectIndex_ = activeMenuCount - 1;
			}
			AudioManager::GetInstance()->PlaySE(SEType::kStage);
		}

		if (Input::GetInstance()->TriggerKey(DIK_RIGHT) || Input::GetInstance()->TriggerKey(DIK_D)) {
			currentSelectIndex_++;
			if (currentSelectIndex_ >= activeMenuCount) {
				currentSelectIndex_ = 0;
			}
			AudioManager::GetInstance()->PlaySE(SEType::kStage);
		}

		// 決定キー（SPACE または ENTER）で項目を確定
		if (Input::GetInstance()->TriggerKey(DIK_SPACE) || Input::GetInstance()->TriggerKey(DIK_RETURN)) {
			if (mode_ == Mode::Select) {
				// セレクト画面モード: 0: Title, 1: Exit
				selectedMenu_ = (currentSelectIndex_ == 0) ? MenuType::Title : MenuType::Exit;
			} else {
				// ゲーム画面モード: 0: Return, 1: Retry, 2: Title, 3: Exit
				selectedMenu_ = static_cast<MenuType>(currentSelectIndex_);
			}
			isMenuSelected_ = true;
			AudioManager::GetInstance()->PlaySE(SEType::kStage);
		}
	}

	// ★ 看板モデルの位置・行列計算（カメラの追従遅れによるズレを防止）
	if ((isPaused_ || animTimer_ > 0.0f) && camera_) {
		float progress = std::clamp(animTimer_ / kAnimDuration, 0.0f, 1.0f);
		// EaseOutCubic による滑らかな降下・上昇補間
		float easeVal = 1.0f - std::pow(1.0f - progress, 3.0f);

		Vector3 currentOffset;
		currentOffset.x = hiddenOffset_.x + (visibleOffset_.x - hiddenOffset_.x) * easeVal;
		currentOffset.y = hiddenOffset_.y + (visibleOffset_.y - hiddenOffset_.y) * easeVal;
		currentOffset.z = hiddenOffset_.z + (visibleOffset_.z - hiddenOffset_.z) * easeVal;

		// 最新のカメラ座標にオフセットを加算
		worldTransform_.translation_.x = camera_->translation_.x + currentOffset.x;
		worldTransform_.translation_.y = camera_->translation_.y + currentOffset.y;
		worldTransform_.translation_.z = camera_->translation_.z + currentOffset.z;

		// 行列計算および転送
		worldTransform_.matWorld_ = MakeAffineMatrix(worldTransform_.scale_, worldTransform_.rotation_, worldTransform_.translation_);
		worldTransform_.TransferMatrix();

		// ★ 操作説明モデル（MoveExplanation, AttackExplanation, DashExplanation）の追従・行列計算（各XYZ軸毎に個別計算）
		// 1. MoveExplanation（移動解説）
		worldTransformMove_.scale_ = {worldTransform_.scale_.x * scaleMultiplierMove_.x, worldTransform_.scale_.y * scaleMultiplierMove_.y, worldTransform_.scale_.z * scaleMultiplierMove_.z};
		worldTransformMove_.rotation_ = worldTransform_.rotation_;
		worldTransformMove_.translation_ = {worldTransform_.translation_.x + offsetMove_.x, worldTransform_.translation_.y + offsetMove_.y, worldTransform_.translation_.z + offsetMove_.z};
		worldTransformMove_.matWorld_ = MakeAffineMatrix(worldTransformMove_.scale_, worldTransformMove_.rotation_, worldTransformMove_.translation_);
		worldTransformMove_.TransferMatrix();

		// 2. AttackExplanation（攻撃解説）
		worldTransformAttack_.scale_ = {worldTransform_.scale_.x * scaleMultiplierAttack_.x, worldTransform_.scale_.y * scaleMultiplierAttack_.y, worldTransform_.scale_.z * scaleMultiplierAttack_.z};
		worldTransformAttack_.rotation_ = worldTransform_.rotation_;
		worldTransformAttack_.translation_ = {worldTransform_.translation_.x + offsetAttack_.x, worldTransform_.translation_.y + offsetAttack_.y, worldTransform_.translation_.z + offsetAttack_.z};
		worldTransformAttack_.matWorld_ = MakeAffineMatrix(worldTransformAttack_.scale_, worldTransformAttack_.rotation_, worldTransformAttack_.translation_);
		worldTransformAttack_.TransferMatrix();

		// 3. DashExplanation（ダッシュ解説）
		worldTransformDash_.scale_ = {worldTransform_.scale_.x * scaleMultiplierDash_.x, worldTransform_.scale_.y * scaleMultiplierDash_.y, worldTransform_.scale_.z * scaleMultiplierDash_.z};
		worldTransformDash_.rotation_ = worldTransform_.rotation_;
		worldTransformDash_.translation_ = {worldTransform_.translation_.x + offsetDash_.x, worldTransform_.translation_.y + offsetDash_.y, worldTransform_.translation_.z + offsetDash_.z};
		worldTransformDash_.matWorld_ = MakeAffineMatrix(worldTransformDash_.scale_, worldTransformDash_.rotation_, worldTransformDash_.translation_);
		worldTransformDash_.TransferMatrix();

		// ★ メニュー選択モデルの追従・行列計算（画面下に左から右へ描画順に均等配置）
		float menuStartX = (mode_ == Mode::Select) ? -0.45f : -1.35f;
		float menuSpacingX = 0.90f;

		for (int32_t activeIdx = 0; activeIdx < activeMenuCount; ++activeIdx) {
			int32_t modelIdx = activeIdx;
			if (mode_ == Mode::Select) {
				// セレクト画面モード: 0番目に Title(モデル2), 1番目に Exit(モデル3) を割り当てる
				modelIdx = (activeIdx == 0) ? 2 : 3;
			}

			if (activeIdx == currentSelectIndex_) {
				if (menuAnimationTimers_[modelIdx] < 1.0f) {
					menuAnimationTimers_[modelIdx] += 0.1f;
					if (menuAnimationTimers_[modelIdx] > 1.0f) {
						menuAnimationTimers_[modelIdx] = 1.0f;
					}
				}
			} else {
				if (menuAnimationTimers_[modelIdx] > 0.0f) {
					menuAnimationTimers_[modelIdx] -= 0.1f;
					if (menuAnimationTimers_[modelIdx] < 0.0f) {
						menuAnimationTimers_[modelIdx] = 0.0f;
					}
				}
			}

			float scaleFactor = 0.85f + (menuAnimationTimers_[modelIdx] * 0.30f);
			menuWorldTransforms_[modelIdx].scale_ = {
			    worldTransform_.scale_.x * scaleMultiplierMenu_.x * scaleFactor, worldTransform_.scale_.y * scaleMultiplierMenu_.y * scaleFactor,
			    worldTransform_.scale_.z * scaleMultiplierMenu_.z * scaleFactor};

			menuWorldTransforms_[modelIdx].rotation_ = worldTransform_.rotation_;

			// 左から右への位置オフセット指定
			float offsetX = menuStartX + static_cast<float>(activeIdx) * menuSpacingX;
			menuWorldTransforms_[modelIdx].translation_ = {worldTransform_.translation_.x + offsetX, worldTransform_.translation_.y + menuPosY_, worldTransform_.translation_.z + menuPosZ_};

			menuWorldTransforms_[modelIdx].matWorld_ = MakeAffineMatrix(menuWorldTransforms_[modelIdx].scale_, menuWorldTransforms_[modelIdx].rotation_, menuWorldTransforms_[modelIdx].translation_);
			menuWorldTransforms_[modelIdx].TransferMatrix();
		}
	}
}

void Pause::Draw() {
	// 非ポーズ時かつ上がりきっている場合は描画しない
	if ((!isPaused_ && animTimer_ <= 0.0f) || !camera_) {
		return;
	}

	// ★ 看板モデルの描画
	if (modelPictureFrame_) {
		modelPictureFrame_->Draw(worldTransform_, *camera_);
	}

	// ★ 操作説明モデルの描画（上から順: Move -> Attack -> Dash）
	if (modelMoveExplanation_) {
		modelMoveExplanation_->Draw(worldTransformMove_, *camera_);
	}
	if (modelAttackExplanation_) {
		modelAttackExplanation_->Draw(worldTransformAttack_, *camera_);
	}
	if (modelDashExplanation_) {
		modelDashExplanation_->Draw(worldTransformDash_, *camera_);
	}

	// ★ メニュー選択モデルの描画（画面下に左から右へ描画）
	const int32_t activeMenuCount = (mode_ == Mode::Select) ? 2 : 4;
	for (int32_t activeIdx = 0; activeIdx < activeMenuCount; ++activeIdx) {
		int32_t modelIdx = activeIdx;
		if (mode_ == Mode::Select) {
			modelIdx = (activeIdx == 0) ? 2 : 3;
		}
		if (menuModels_[modelIdx]) {
			menuModels_[modelIdx]->Draw(menuWorldTransforms_[modelIdx], *camera_);
		}
	}
}