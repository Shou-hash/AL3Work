#include "TitleScene.h"
#include "AudioManager.h"
#include "Kamataengine.h"
#include "Matrix4x4.h"
#include "ModelCache.h"
#include <algorithm>
#include <cmath>
#include <numbers>
#ifdef _DEBUG
#include <imgui.h>
#endif

namespace {
// ★ 軽量化: カメラからこの横方向の距離の列だけ、ブロックを用意して描画する（タイトルのカメラは固定の奥行きなので、これ以上遠くは画面に映らない）
constexpr float kTitleStageRangeX = 24.0f;
} // namespace

TitleScene::~TitleScene() {
	delete fade_;
	delete player_;

	// ★ 軽量化: モデルは ModelCache で使い回しているため、ここでは解放しない
	delete mapChipField_;
	for (auto& worldTransformBlockLine : worldTransformBlocks_) {
		for (auto* worldTransformBlock : worldTransformBlockLine) {
			delete worldTransformBlock;
		}
	}
	worldTransformBlocks_.clear();
}

// ★ 軽量化: カメラの周辺の列のブロックだけ WorldTransform を作る
//   ブロックは動かないので、作るときに行列を 1 回だけ計算して転送する（毎フレームの更新は不要）
void TitleScene::EnsureBlockTransforms(float centerX) {
	if (!mapChipField_ || columnReady_.empty()) {
		return;
	}

	const int32_t numColumns = static_cast<int32_t>(columnReady_.size());
	const int32_t minCol = (std::max)(0, static_cast<int32_t>(std::floor(centerX - kTitleStageRangeX)));
	const int32_t maxCol = (std::min)(numColumns - 1, static_cast<int32_t>(std::ceil(centerX + kTitleStageRangeX)));

	for (int32_t j = minCol; j <= maxCol; ++j) {
		if (columnReady_[j]) {
			continue;
		}
		columnReady_[j] = true;

		for (size_t i = 0; i < worldTransformBlocks_.size(); ++i) {
			if (!blockModels_[i][j] || worldTransformBlocks_[i][j]) {
				continue;
			}
			KamataEngine::WorldTransform* worldTransform = new KamataEngine::WorldTransform();
			worldTransform->Initialize();
			worldTransform->translation_ = mapChipField_->GetMapChipPositionByIndex(j, static_cast<uint32_t>(i));
			worldTransform->matWorld_ = MakeAffineMatrix(worldTransform->scale_, worldTransform->rotation_, worldTransform->translation_);
			worldTransform->TransferMatrix();
			worldTransformBlocks_[i][j] = worldTransform;
		}
	}
}

void TitleScene::Initialize() {
	AudioManager::GetInstance()->PlayBGM(BGMType::kTitle);

	finished_ = false;
	phase_ = Phase::FadeIn;

	fade_ = new Fade();
	fade_->Initialize();
	fade_->Start(Fade::Status::FadeIn, 1.0f);

	camera_.Initialize();
	camera_.translation_ = {10.0f, 5.0f, -15.0f};

	mapChipField_ = new MapChipField();
	mapChipField_->LoadMapChipDataFromCSV("Resources/titleStageDatas.csv");
	// ★ 軽量化: モデルは ModelCache で使い回す（ゲームシーンと同じモデルを二重に読み込まない）
	model_ = ModelCache::Load("block");

	// ★ ブロックの向き別モデルを読み込み
	modelBlockAbove_ = ModelCache::Load("blockAbove");
	modelBlockBelow_ = ModelCache::Load("blockBelow");
	modelBlockLeft_ = ModelCache::Load("blockLeft");
	modelBlockRight_ = ModelCache::Load("blockRight");

	// ★ 奥行き背景（Resources/titleBackgroundDatas.csv から配置）
	backgroundBlocks_ = std::make_unique<BackgroundBlocks>();
	backgroundBlocks_->Initialize(mapChipField_, "Resources/titleBackgroundDatas.csv");

	uint32_t numBlockVertical = mapChipField_->GetNumBlockVertical();
	uint32_t numBlockHorizontal = mapChipField_->GetNumBlockHorizontal();

	blockModels_.assign(numBlockVertical, std::vector<KamataEngine::Model*>(numBlockHorizontal, nullptr));
	worldTransformBlocks_.resize(numBlockVertical);
	columnReady_.assign(numBlockHorizontal, false);
	for (uint32_t i = 0; i < numBlockVertical; ++i) {
		worldTransformBlocks_[i].resize(numBlockHorizontal, nullptr);
		for (uint32_t j = 0; j < numBlockHorizontal; ++j) {
			MapChipType type = mapChipField_->GetMapChipTypeByIndex(j, i);
			if (type == MapChipType::kBlock) {
				// ★ 軽量化: WorldTransform はカメラの周辺に来たときに EnsureBlockTransforms で作る（ここではモデルの種類だけ決める）

				// ★ ブロックの向きの判定（GameScene と共通）。空白に一番近いブロックだけが向きを持つ
				switch (mapChipField_->GetBlockFaceByIndex(j, i)) {
				case BlockFace::kAbove:
					blockModels_[i][j] = modelBlockAbove_; // 上が空白（地面）
					break;
				case BlockFace::kBelow:
					blockModels_[i][j] = modelBlockBelow_; // 下が空白（天井）
					break;
				case BlockFace::kRight:
					blockModels_[i][j] = modelBlockRight_; // 右が空白（左壁）
					break;
				case BlockFace::kLeft:
					blockModels_[i][j] = modelBlockLeft_; // 左が空白（右壁）
					break;
				default:
					blockModels_[i][j] = model_; // 通常のブロック
					break;
				}
			}
		}
	}

	modelPlayerHead_ = ModelCache::Load("player_head");
	modelPlayerBody_ = ModelCache::Load("player_body");
	modelPlayerLeft_ = ModelCache::Load("player_left");
	modelPlayerRight_ = ModelCache::Load("player_right");

	player_ = new Player();
	player_->Initialize(modelPlayerBody_, &camera_, {5.0f, 4.0f, 0.0f});
	player_->SetMapChipField(mapChipField_);

	modelSkydome_ = ModelCache::Load("skydome");
	skydome = std::make_unique<Skydome>();
	skydome->Initialize(modelSkydome_, &camera_);

	// ★ タイトル名モデルの初期化（Scale拡大 & Y軸180度回転）
	modelTitleName_ = ModelCache::Load("TitleName");
	worldTransformTitleName_.Initialize();
	worldTransformTitleName_.scale_ = {2.0f, 2.0f, 2.0f};             // ★ スケールを大きく変更 (画面サイズに合わせて数値を調整してください)
	worldTransformTitleName_.rotation_.y = std::numbers::pi_v<float>; // ★ Y軸回転を180度(π rad)に設定
	worldTransformTitleName_.translation_ = {10.0f, 8.0f, 0.0f};

	// ★ 朝のライティングを設定（ラスボスの夜・赤いスポットライトは使わない）
	lightManager_ = std::make_unique<LightManager>();
	lightManager_->Initialize();
	lightManager_->ApplyToModel(model_, LightManager::GroupType::kWorld);
	lightManager_->ApplyToModel(modelBlockAbove_, LightManager::GroupType::kWorld);
	lightManager_->ApplyToModel(modelBlockBelow_, LightManager::GroupType::kWorld);
	lightManager_->ApplyToModel(modelBlockLeft_, LightManager::GroupType::kWorld);
	lightManager_->ApplyToModel(modelBlockRight_, LightManager::GroupType::kWorld);
	lightManager_->ApplyToModel(modelPlayerHead_, LightManager::GroupType::kWorld);
	lightManager_->ApplyToModel(modelPlayerBody_, LightManager::GroupType::kWorld);
	lightManager_->ApplyToModel(modelPlayerLeft_, LightManager::GroupType::kWorld);
	lightManager_->ApplyToModel(modelPlayerRight_, LightManager::GroupType::kWorld);
	lightManager_->ApplyToModel(modelSkydome_, LightManager::GroupType::kWorld);
	lightManager_->ApplyToModel(modelTitleName_, LightManager::GroupType::kWorld);
	backgroundBlocks_->SetLightGroups(
	    lightManager_->GetLightGroup(LightManager::GroupType::kBackgroundNear), lightManager_->GetLightGroup(LightManager::GroupType::kBackgroundMid),
	    lightManager_->GetLightGroup(LightManager::GroupType::kBackgroundFar));

	// ★ 軽量化: 最初に画面に映る周辺のブロックだけ用意する（残りはカメラが近づいたときに用意する）
	EnsureBlockTransforms(camera_.translation_.x);
}

void TitleScene::Update() {
	fade_->Update();
	skydome->Update();

	if (player_) {
		KamataEngine::WorldTransform& transformRoot = const_cast<KamataEngine::WorldTransform&>(player_->GetWorldTransform());
		KamataEngine::WorldTransform& transformHead = const_cast<KamataEngine::WorldTransform&>(player_->GetWorldTransformHead());
		KamataEngine::WorldTransform& transformBody = const_cast<KamataEngine::WorldTransform&>(player_->GetWorldTransformBody());
		KamataEngine::WorldTransform& transformLeft = const_cast<KamataEngine::WorldTransform&>(player_->GetWorldTransformLeft());
		KamataEngine::WorldTransform& transformRight = const_cast<KamataEngine::WorldTransform&>(player_->GetWorldTransformRight());

		Player::CollisionMapInfo collisionMapInfo;
		collisionMapInfo.moveAmount.x = 0.04f;
		collisionMapInfo.moveAmount.z = 0.0f;

		bool realGrounded = false;
		KamataEngine::Vector3 footPos = {transformRoot.translation_.x, transformRoot.translation_.y - 0.55f, transformRoot.translation_.z};
		MapChipField::IndexSet footIndex = mapChipField_->GetMapChipIndexByPosition(footPos);
		if (mapChipField_->GetMapChipTypeByIndex(footIndex.x, footIndex.y) == MapChipType::kBlock) {
			realGrounded = true;
		}

		static float currentVelocityY = 0.0f;
		if (!realGrounded) {
			currentVelocityY += -0.015f;
		} else {
			currentVelocityY = 0.0f;
		}

		if (realGrounded) {
			KamataEngine::Vector3 checkPos = {transformRoot.translation_.x + 0.6f, transformRoot.translation_.y, transformRoot.translation_.z};
			MapChipField::IndexSet indexSet = mapChipField_->GetMapChipIndexByPosition(checkPos);
			MapChipType frontTile = mapChipField_->GetMapChipTypeByIndex(indexSet.x, indexSet.y);
			if (frontTile == MapChipType::kBlock) {
				currentVelocityY = 0.35f;
				realGrounded = false;
			}
		}

		collisionMapInfo.moveAmount.y = currentVelocityY;
		collisionMapInfo.onGround = realGrounded;

		player_->MapCollision(collisionMapInfo);

		transformRoot.translation_.x += collisionMapInfo.moveAmount.x;
		transformRoot.translation_.y += collisionMapInfo.moveAmount.y;
		transformRoot.translation_.z += collisionMapInfo.moveAmount.z;

		player_->ApplyGroundingStatus(collisionMapInfo);

		currentVelocityY = collisionMapInfo.moveAmount.y;
		if (collisionMapInfo.onGround) {
			currentVelocityY = 0.0f;
		}

		// ★ ステージの右端の手前まで進んだら、最初の位置に戻してループさせる（ステージの横幅に合わせる）
		if (transformRoot.translation_.x > static_cast<float>(mapChipField_->GetUsedBlockHorizontal()) - 15.0f) {
			transformRoot.translation_.x = 2.0f;
			transformRoot.translation_.y = 10.0f;
			currentVelocityY = 0.0f;

			collisionMapInfo.onGround = false;
			player_->ApplyGroundingStatus(collisionMapInfo);
		}

		camera_.translation_.x = transformRoot.translation_.x;

		static float titleWalkTimer = 0.0f;
		if (collisionMapInfo.onGround) {
			titleWalkTimer += 0.03f;
		} else {
			titleWalkTimer = 0.0f;
		}

		float pi = std::numbers::pi_v<float>;
		float walkTilt = std::sin(titleWalkTimer * 2.0f * pi) * 0.75f;

		if (!collisionMapInfo.onGround) {
			walkTilt = 0.3f;
		}

		transformLeft.rotation_.x = walkTilt;
		transformRight.rotation_.x = -walkTilt;

		transformRoot.matWorld_ = MakeAffineMatrix(transformRoot.scale_, transformRoot.rotation_, transformRoot.translation_);
		transformRoot.TransferMatrix();

		transformBody.matWorld_ = Multiply(MakeAffineMatrix(transformBody.scale_, transformBody.rotation_, transformBody.translation_), transformRoot.matWorld_);
		transformHead.matWorld_ = Multiply(MakeAffineMatrix(transformHead.scale_, transformHead.rotation_, transformHead.translation_), transformBody.matWorld_);

		KamataEngine::Vector3 centerOffset = {0.0f, -0.5f, 0.0f};

		KamataEngine::Matrix4x4 rotateLeft = MakeAffineMatrix({1.0f, 1.0f, 1.0f}, transformLeft.rotation_, {0.0f, 0.0f, 0.0f});
		KamataEngine::Matrix4x4 preTranslateLeft = MakeAffineMatrix({1.0f, 1.0f, 1.0f}, {0.0f, 0.0f, 0.0f}, {-centerOffset.x, -centerOffset.y, -centerOffset.z});
		KamataEngine::Vector3 finalTranslationLeft = {transformLeft.translation_.x + centerOffset.x, transformLeft.translation_.y + centerOffset.y, transformLeft.translation_.z + centerOffset.z};
		KamataEngine::Matrix4x4 postTranslateLeft = MakeAffineMatrix(transformLeft.scale_, {0.0f, 0.0f, 0.0f}, finalTranslationLeft);
		transformLeft.matWorld_ = Multiply(postTranslateLeft, Multiply(rotateLeft, preTranslateLeft));
		transformLeft.matWorld_ = Multiply(transformLeft.matWorld_, transformBody.matWorld_);

		KamataEngine::Matrix4x4 rotateRight = MakeAffineMatrix({1.0f, 1.0f, 1.0f}, transformRight.rotation_, {0.0f, 0.0f, 0.0f});
		KamataEngine::Matrix4x4 preTranslateRight = MakeAffineMatrix({1.0f, 1.0f, 1.0f}, {0.0f, 0.0f, 0.0f}, {-centerOffset.x, -centerOffset.y, -centerOffset.z});
		KamataEngine::Vector3 finalTranslationRight = {transformRight.translation_.x + centerOffset.x, transformRight.translation_.y + centerOffset.y, transformRight.translation_.z + centerOffset.z};
		KamataEngine::Matrix4x4 postTranslateRight = MakeAffineMatrix(transformRight.scale_, {0.0f, 0.0f, 0.0f}, finalTranslationRight);
		transformRight.matWorld_ = Multiply(postTranslateRight, Multiply(rotateRight, preTranslateRight));
		transformRight.matWorld_ = Multiply(transformRight.matWorld_, transformBody.matWorld_);

		transformBody.TransferMatrix();
		transformHead.TransferMatrix();
		transformLeft.TransferMatrix();
		transformRight.TransferMatrix();
	}

	// ★ 軽量化: ブロックは動かないので毎フレームの行列更新はしない。カメラの周辺に来た列のブロックだけ用意する
	EnsureBlockTransforms(camera_.translation_.x);

	// ★ タイトル名モデルの演出処理（待機時の上下移動 ＆ SPACE押下時の回転）
	static float titleFloatingTimer = 0.0f;
	if (phase_ == Phase::FadeOut) {
		// SPACE押下後（フェードアウトフェーズ）：くるくる回転
		worldTransformTitleName_.rotation_.y += 0.15f;
	} else {
		// 待機時（FadeIn / Normalフェーズ）：Sin波で上下ゆらゆら移動
		titleFloatingTimer += 0.05f;
		worldTransformTitleName_.translation_.y = 8.0f + std::sin(titleFloatingTimer) * 0.3f;
	}

	// タイトル名モデルの追従と行列更新処理
	worldTransformTitleName_.translation_.x = camera_.translation_.x;
	worldTransformTitleName_.matWorld_ = MakeAffineMatrix(worldTransformTitleName_.scale_, worldTransformTitleName_.rotation_, worldTransformTitleName_.translation_);
	worldTransformTitleName_.TransferMatrix();

	camera_.UpdateMatrix();

	// ★ 奥行き背景の更新と、朝のライティングの更新
	backgroundBlocks_->Update(camera_);
	lightManager_->Update();

	switch (phase_) {
	case Phase::FadeIn:
		if (fade_->IsFinished())
			phase_ = Phase::Normal;
		break;
	case Phase::Normal:
		if (KamataEngine::Input::GetInstance()->PushKey(DIK_SPACE)) {
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
	// ★ ライティング（光沢の強さなど）の調整用ウィンドウ
	ImGui::Begin("Title Scene");
	lightManager_->DrawImGui();
	ImGui::End();
#endif
}

void TitleScene::Draw() {
	// ★ 奥行き背景を最初に描画
	backgroundBlocks_->Draw(camera_);

	// ★ ブロックは向きに応じたモデルで描画
	//   軽量化: PreDraw / PostDraw はブロック 1 個ごとではなく全体で 1 回だけ。カメラの周辺の列だけ描画する
	{
		const int32_t minCol = (std::max)(0, static_cast<int32_t>(std::floor(camera_.translation_.x - kTitleStageRangeX)));
		KamataEngine::Model::PreDraw();
		for (size_t i = 0; i < worldTransformBlocks_.size(); ++i) {
			const int32_t maxCol = (std::min)(static_cast<int32_t>(worldTransformBlocks_[i].size()) - 1, static_cast<int32_t>(std::ceil(camera_.translation_.x + kTitleStageRangeX)));
			for (int32_t j = minCol; j <= maxCol; ++j) {
				KamataEngine::WorldTransform* worldTransformBlock = worldTransformBlocks_[i][j];
				if (!worldTransformBlock) {
					continue;
				}
				KamataEngine::Model* blockModel = blockModels_[i][j] ? blockModels_[i][j] : model_;
				blockModel->Draw(*worldTransformBlock, camera_);
			}
		}
		KamataEngine::Model::PostDraw();
	}

	skydome->Draw();

	if (modelTitleName_) {
		KamataEngine::Model::PreDraw();
		modelTitleName_->Draw(worldTransformTitleName_, camera_);
		KamataEngine::Model::PostDraw();
	}

	if (player_) {
		KamataEngine::Model::PreDraw();
		player_->Draw();
		KamataEngine::Model::PostDraw();
	}
	fade_->Draw();
}
