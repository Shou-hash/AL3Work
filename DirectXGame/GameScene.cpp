#include "GameScene.h"
#include "AudioManager.h"
#include "DeathParticles.h"
#include "Enemy.h"
#include "GlobalVariables.h"
#include "Goal.h"
#include "HitEffect.h"
#include "Item.h"
#include "Matrix4x4.h"
#include "Player.h"
#include "ShieldEnemy.h"
#include "StageManager.h"
#include <cmath>
#include <numbers>
#ifdef _DEBUG
#include <imgui.h>
#endif

using namespace KamataEngine;

GameScene::~GameScene() {
	delete fade_;

	// 各モデルの解放
	delete modelBlock_;
	delete modelBlockFall_;
	delete modelBlockFallLeft_;
	delete modelBlockFallRight_;
	delete modelBlockLeft_;
	delete modelBlockRight_;
	delete modelBlockAbove_;
	delete modelBlockBelow_;
	delete modelCloud_; // ★ 追加: 雲モデルの解放

	// ブロックデータの解放
	for (auto& line : blockDatas_) {
		for (auto* blockData : line) {
			if (blockData) {
				delete blockData->transform;
				delete blockData;
			}
		}
	}
	blockDatas_.clear();

	// ★ 追加: ステージ上の雲データの解放
	for (auto* cloudData : cloudDatas_) {
		if (cloudData) {
			delete cloudData->transform;
			delete cloudData;
		}
	}
	cloudDatas_.clear();

	for (WorldTransform* worldTransformExplanation : worldTransformExplanations_) {
		delete worldTransformExplanation;
	}
	worldTransformExplanations_.clear();

	delete debugCamera_;
	delete modelSkydome_;
	delete mapChipField_;
	delete modelEnemy_;
	delete modelShieldEnemy_;

	// 4つの各部位のモデルを解放
	delete modelPlayerHead_;
	delete modelPlayerBody_;
	delete modelPlayerLeft_;
	delete modelPlayerRight_;

	delete modelBossHead_;
	delete modelBossBody_;
	delete modelBossLeft_;
	delete modelBossRight_;

	delete modelDeathParticles_;
	delete modelHitEffect_;

	delete modelHammer_;

	delete modelPlayerHp_;
	delete modelItemHp_;
	delete modelGoal_;        // ゴール用モデルの解放
	delete modelExplanation_; // 解説ブロック用モデルの解放

	for (BaseEnemy* enemy : enemies_) {
		delete enemy;
	}
	enemies_.clear();

	delete modelFinalBossBody_;
	for (auto* modelHand : modelFinalBossHands_) {
		delete modelHand;
	}
	for (auto* modelCrystal : modelFinalBossCrystals_) { // ★ 追加
		delete modelCrystal;
	}
	delete modelFinalBossCrystal22_; // ★ 攻撃専用クリスタルの解放

	for (BaseEffect* effect : effects_) {
		delete effect;
	}
	effects_.clear();

	for (Item* item : items_) {
		delete item;
	}
	items_.clear();
}

void GameScene::Initialize(StageManager* stageDataManager) {

	// ★ ゲームシーンに入った時にBGMを再生
	AudioManager::GetInstance()->PlayBGM(BGMType::kGame);

	stageManager_ = stageDataManager;

	// ★ ライトマネージャの初期化
	lightManager_ = std::make_unique<LightManager>();
	lightManager_->Initialize();

	phase_ = Phase::kFadeIn;
	finished_ = false;
	isBossSpawned_ = false;
	isGoalReached_ = false;                 // ゴール到達フラグの初期化
	isBossDefeatedGoalPerfStarted_ = false; // ボス撃破後ゴール演出フラグの初期化
	isBossLightTriggered_ = false;          // ラスボス出現演出による光の演出フラグの初期化

	// ステージ切り替え時に前回のデータを完全にクリアする
	player_.reset();
	goal_.reset(); // 前回のゴールデータクリア

	for (BaseEnemy* enemy : enemies_) {
		delete enemy;
	}
	enemies_.clear();

	for (BaseEffect* effect : effects_) {
		delete effect;
	}
	effects_.clear();

	for (Item* item : items_) {
		delete item;
	}
	items_.clear();

	for (WorldTransform* worldTransformExplanation : worldTransformExplanations_) {
		delete worldTransformExplanation;
	}
	worldTransformExplanations_.clear();

	if (mapChipField_) {
		delete mapChipField_;
		mapChipField_ = nullptr;
	}

	// 調整項目の登録と適用
	Player::RegisterGlobalVariables();
	Enemy::RegisterGlobalVariables();

	GlobalVariables::GetInstance()->LoadFiles();
	Player::ApplyGlobalVariables();
	Enemy::ApplyGlobalVariables();

	fade_ = new Fade();
	fade_->Initialize();
	fade_->Start(Fade::Status::FadeIn, 1.0f);

	mapChipField_ = new MapChipField;

	int currentStageIdx = stageManager_ ? stageManager_->GetCurrentStageIndex() : 0;
	std::string stageFileName = "Resources/stageDatas" + std::to_string(currentStageIdx) + ".csv";
	mapChipField_->LoadMapChipDataFromCSV(stageFileName);

	// 背景ブロックの生成（自動生成による多層視差レイヤーの初期化）
	backgroundBlocks_ = std::make_unique<BackgroundBlocks>();
	backgroundBlocks_->Initialize(mapChipField_);

	// ブロックモデルを種類別に読み込み
	modelBlock_ = Model::CreateFromOBJ("block", true);
	modelBlockFall_ = Model::CreateFromOBJ("blockFall", true);
	modelBlockFallLeft_ = Model::CreateFromOBJ("blockFallLeft", true);
	modelBlockFallRight_ = Model::CreateFromOBJ("blockFallRight", true);
	modelBlockLeft_ = Model::CreateFromOBJ("blockLeft", true);
	modelBlockRight_ = Model::CreateFromOBJ("blockRight", true);
	modelBlockAbove_ = Model::CreateFromOBJ("blockAbove", true);
	modelBlockBelow_ = Model::CreateFromOBJ("blockBelow", true);

	// ★ 追加: 雲モデル (cloud3) の読み込み
	modelCloud_ = Model::CreateFromOBJ("cloud3", true);

	worldTransform_.Initialize();
	camera_.Initialize();
	debugCamera_ = new DebugCamera(1280, 720);
	modelSkydome_ = Model::CreateFromOBJ("skydome", true);

	modelPlayerHead_ = Model::CreateFromOBJ("player_head", true);
	modelPlayerBody_ = Model::CreateFromOBJ("player_body", true);
	modelPlayerLeft_ = Model::CreateFromOBJ("player_left", true);
	modelPlayerRight_ = Model::CreateFromOBJ("player_right", true);

	modelBossBody_ = Model::CreateFromOBJ("bossEnemy_body", true);
	modelBossHead_ = Model::CreateFromOBJ("bossEnemy_head", true);
	modelBossLeft_ = Model::CreateFromOBJ("bossEnemy_left", true);
	modelBossRight_ = Model::CreateFromOBJ("bossEnemy_right", true);

	// ラスボス本体モデル
	modelFinalBossBody_ = Model::CreateFromOBJ("finalBoss", true);

	// ラスボス7本の手のモデル
	for (int i = 0; i < 7; ++i) {
		std::string modelName = "finalBossHand_" + std::to_string(i + 1);
		modelFinalBossHands_[i] = Model::CreateFromOBJ(modelName, true);
	}

	// ラスボス21個のクリスタルのモデル
	for (int i = 0; i < 21; ++i) {
		std::string modelName = "finalBossCrystal_" + std::to_string(i + 1);
		modelFinalBossCrystals_[i] = Model::CreateFromOBJ(modelName, true);
	}
	// ★ 攻撃専用の finalBossCrystal_22 モデルを別に読み込む
	modelFinalBossCrystal22_ = Model::CreateFromOBJ("finalBossCrystal_22", true);

	modelHammer_ = Model::CreateFromOBJ("hummer", true);

	modelEnemy_ = Model::CreateFromOBJ("enemy", true);
	modelShieldEnemy_ = Model::CreateFromOBJ("shieldEnemy", true);
	modelDeathParticles_ = Model::CreateFromOBJ("particle", true);
	modelHitEffect_ = Model::CreateFromOBJ("particle", true);

	modelPlayerHp_ = Model::CreateFromOBJ("itemHP", true);
	modelItemHp_ = Model::CreateFromOBJ("itemHP", true);
	modelGoal_ = Model::CreateFromOBJ("goal", true);               // ゴール用モデルの生成
	modelExplanation_ = Model::CreateFromOBJ("Explanation", true); // 解説ブロック用モデルの生成

	GenerateFieldObjects();

	// ★ 操作解説用チュートリアルの初期化（ステージ0のみ有効化）
	playerTutorial_ = std::make_unique<PlayerTutorial>();
	Vector3 playerInitPos = player_ ? player_->GetWorldTransform().translation_ : Vector3{0.0f, 0.0f, 0.0f};
	playerTutorial_->Initialize(currentStageIdx, playerInitPos);

	// ★ 全モデルに LightManager のライトグループを設定する
	{
		KamataEngine::Model* lightTargets[] = {
		    modelEnemy_,    modelShieldEnemy_, modelSkydome_,        modelPlayerHead_,     modelPlayerBody_, modelPlayerLeft_, modelPlayerRight_, modelBossHead_,   modelBossBody_,
		    modelBossLeft_, modelBossRight_,   modelDeathParticles_, modelHitEffect_,      modelHammer_,     modelPlayerHp_,   modelItemHp_,      modelGoal_,       modelExplanation_,
		    modelBlock_,    modelBlockFall_,   modelBlockFallLeft_,  modelBlockFallRight_, modelBlockLeft_,  modelBlockRight_, modelBlockAbove_,  modelBlockBelow_, modelCloud_,
		};
		for (KamataEngine::Model* model : lightTargets) {
			lightManager_->ApplyToModel(model, LightManager::GroupType::kWorld);
		}

		if (playerTutorial_) {
			playerTutorial_->ApplyLightManager(*lightManager_);
		}

		// ラスボスは専用のライトグループ
		lightManager_->ApplyToModel(modelFinalBossBody_, LightManager::GroupType::kBoss);
		for (KamataEngine::Model* model : modelFinalBossHands_) {
			lightManager_->ApplyToModel(model, LightManager::GroupType::kBoss);
		}

		for (KamataEngine::Model* model : modelFinalBossCrystals_) {
			lightManager_->ApplyToModel(model, LightManager::GroupType::kBoss);
		}
		lightManager_->ApplyToModel(modelFinalBossCrystal22_, LightManager::GroupType::kBoss);

		// 奥行き背景は中景と遠景で別々のライトグループ
		if (backgroundBlocks_) {
			backgroundBlocks_->SetLightGroups(
			    lightManager_->GetLightGroup(LightManager::GroupType::kBackgroundNear), lightManager_->GetLightGroup(LightManager::GroupType::kBackgroundMid),
			    lightManager_->GetLightGroup(LightManager::GroupType::kBackgroundFar));
		}
	}

	HitEffect::SetModel(modelHitEffect_);
	HitEffect::SetCamera(&camera_);

	skydome = std::make_unique<Skydome>();
	skydome->Initialize(modelSkydome_, &camera_);

	cameraController_ = std::make_unique<CameraController>();
	cameraController_->Initialize(&camera_);

	// ★ プレイヤーのモデルにもマップタイルなどと同じライティングを設定する
	if (player_ && lightManager_) {
		player_->ApplyLightManager(*lightManager_);
	}

	// ★ 追加: プレイヤーに雲データを参照させる
	if (player_) {
		player_->SetCloudDatas(&cloudDatas_);
	}

	if (player_) {
		cameraController_->SetTarget(player_.get());
		player_->SetCameraController(cameraController_.get());
		player_->SetModelHammer(modelHammer_);
	}

	int32_t maxHp = 4;
	if (currentStageIdx == 1) {
		maxHp = 5;
	} else if (currentStageIdx == 2) {
		maxHp = 6;
	}

	playerHp_ = std::make_unique<PlayerHp>();
	playerHp_->Initialize(modelPlayerHp_, maxHp);

	if (player_) {
		player_->SetPlayerHp(playerHp_.get());
	}

	// ★ カメラの移動範囲：CSVで実際に使われている横幅に合わせる
	Rect stageArea = {10.0f, static_cast<float>(mapChipField_->GetUsedBlockHorizontal()) - 10.0f, 5.0f, static_cast<float>(mapChipField_->GetNumBlockVertical()) + 80.0f};
	cameraController_->SetMovableArea(stageArea);
	cameraController_->Reset();

	// ステージごとのゴール表示・カメラ演出の制御
	if (goal_) {
		if (currentStageIdx == 0 || currentStageIdx == 1) {
			goal_->SetIsActive(true);
			cameraController_->StartGoalPerformance(goal_->GetWorldTransform().translation_);
		} else if (currentStageIdx == 2) {
			goal_->SetIsActive(false);
		}
	}
}

void GameScene::GenerateFieldObjects() {
	uint32_t numBlockVertical = mapChipField_->GetNumBlockVertical();
	uint32_t numBlockHorizontal = mapChipField_->GetNumBlockHorizontal();

	blockDatas_.resize(numBlockVertical);
	for (uint32_t i = 0; i < numBlockVertical; ++i) {
		blockDatas_[i].resize(numBlockHorizontal, nullptr);
		for (uint32_t j = 0; j < numBlockHorizontal; ++j) {
			MapChipType type = mapChipField_->GetMapChipTypeByIndex(j, i);
			uint8_t subID = mapChipField_->GetMapChipSubIDByIndex(j, i);

			if (type == MapChipType::kBlockFall) {
				BlockData* blockData = new BlockData();
				blockData->transform = new WorldTransform();
				blockData->transform->Initialize();

				Vector3 pos = mapChipField_->GetMapChipPositionByIndex(j, i);
				pos.y += 0.2f;
				blockData->transform->translation_ = pos;

				bool hasLeft = (j > 0) && (mapChipField_->GetMapChipTypeByIndex(j - 1, i) == MapChipType::kBlockFall);
				bool hasRight = (j < numBlockHorizontal - 1) && (mapChipField_->GetMapChipTypeByIndex(j + 1, i) == MapChipType::kBlockFall);

				if (!hasLeft && hasRight) {
					blockData->model = modelBlockFallLeft_;
				} else if (hasLeft && !hasRight) {
					blockData->model = modelBlockFallRight_;
				} else {
					blockData->model = modelBlockFall_;
				}

				blockDatas_[i][j] = blockData;
			}

			if (type == MapChipType::kBlock) {
				BlockData* blockData = new BlockData();
				blockData->transform = new WorldTransform();
				blockData->transform->Initialize();
				blockData->transform->translation_ = mapChipField_->GetMapChipPositionByIndex(j, i);

				switch (mapChipField_->GetBlockFaceByIndex(j, i)) {
				case BlockFace::kAbove:
					blockData->model = modelBlockAbove_;
					break;
				case BlockFace::kBelow:
					blockData->model = modelBlockBelow_;
					break;
				case BlockFace::kRight:
					blockData->model = modelBlockRight_;
					break;
				case BlockFace::kLeft:
					blockData->model = modelBlockLeft_;
					break;
				default:
					blockData->model = modelBlock_;
					break;
				}

				blockDatas_[i][j] = blockData;
			}

			if (type == MapChipType::kCloud && subID == 0) {
				BlockData* cloudData = new BlockData();
				cloudData->transform = new WorldTransform();
				cloudData->transform->Initialize();
				Vector3 pos = mapChipField_->GetMapChipPositionByIndex(j, i);
				cloudData->transform->translation_ = pos;
				cloudData->basePosition = pos;
				cloudData->offsetY = 0.0f;
				cloudData->velocityY = 0.0f;
				cloudData->transform->scale_ = {1.0f, 1.0f, 1.0f};
				cloudData->model = modelCloud_;
				cloudDatas_.push_back(cloudData);
			}

			switch (type) {
			case MapChipType::kPlayer: {
				if (player_ != nullptr) {
					break;
				}
				Vector3 playerPosition = mapChipField_->GetMapChipPositionByIndex(j, i);
				player_ = std::make_unique<Player>();

				player_->Initialize(modelPlayerBody_, &camera_, playerPosition);
				player_->SetMapChipField(mapChipField_);
				player_->SetModelHammer(modelHammer_);

				break;
			}
			case MapChipType::kEnemy: {
				GenerateEnemy(j, i);
				break;
			}
			case MapChipType::kGoal: {
				if (goal_ != nullptr) {
					break;
				}
				Vector3 goalPosition = mapChipField_->GetMapChipPositionByIndex(j, i);
				goal_ = std::make_unique<Goal>();
				goal_->Initialize(modelGoal_, &camera_, goalPosition);
				break;
			}
			case MapChipType::kExplanation: {
				WorldTransform* worldTransform = new WorldTransform();
				worldTransform->Initialize();
				worldTransform->translation_ = mapChipField_->GetMapChipPositionByIndex(j, i);
				worldTransform->rotation_.y = std::numbers::pi_v<float>;
				worldTransformExplanations_.push_back(worldTransform);
				break;
			}
			default:
				break;
			}
		}
	}
}

void GameScene::GenerateEnemy(uint32_t xIndex, uint32_t yIndex) {
	uint8_t subID = mapChipField_->GetMapChipSubIDByIndex(xIndex, yIndex);
	Vector3 enemyPosition = mapChipField_->GetMapChipPositionByIndex(xIndex, yIndex);

	switch (subID) {
	case 0: {
		Enemy* enemy = new Enemy();
		enemy->Initialize(modelEnemy_, &camera_, enemyPosition, mapChipField_);
		enemies_.push_back(enemy);
		break;
	}
	case 1: {
		ShieldEnemy* enemy = new ShieldEnemy();
		enemy->Initialize(modelShieldEnemy_, &camera_, enemyPosition, mapChipField_);
		enemies_.push_back(enemy);
		break;
	}
	case 2: {
		BossEnemy* enemy = new BossEnemy();
		enemy->Initialize(modelBossBody_, modelBossHead_, modelBossLeft_, modelBossRight_, &camera_, enemyPosition, mapChipField_);
		enemies_.push_back(enemy);
		isBossSpawned_ = true;

		if (cameraController_) {
			cameraController_->SetBoss(enemy);
		}
		break;
	}
	case 3: {
		FinalBoss* finalBoss = new FinalBoss();
		finalBoss->Initialize(modelFinalBossBody_, modelFinalBossHands_, modelFinalBossCrystals_, modelFinalBossCrystal22_, &camera_, enemyPosition);
		finalBoss->SetMapChipField(mapChipField_);
		enemies_.push_back(finalBoss);

		if (cameraController_) {
			cameraController_->SetFinalBoss(finalBoss);
		}
		break;
	}
	default:
		break;
	}
}

bool IsCollision(const Player::AABB& a, const BaseEnemy::AABB& b) {
	return (a.min.x <= b.max.x && a.max.x >= b.min.x) && (a.min.y <= b.max.y && a.max.y >= b.min.y) && (a.min.z <= b.max.z && a.max.z >= b.min.z);
}

void GameScene::UpdateClouds() {
	if (!player_) {
		return;
	}

	const Vector3& pPos = player_->GetWorldTransform().translation_;
	float pLeftX = pPos.x - 0.4f;
	float pRightX = pPos.x + 0.4f;
	float pBottomY = pPos.y - 0.1f;
	bool playerOnGround = player_->IsOnGround();

	static float timer = 0.0f;
	timer += 1.0f / 60.0f;

	for (auto* cloudData : cloudDatas_) {
		if (!cloudData || !cloudData->transform)
			continue;

		constexpr float kSpring = 0.08f;
		constexpr float kDamping = 0.88f;

		float sineWave = std::sin(timer * 3.0f + cloudData->basePosition.x) * 0.02f;

		float force = -kSpring * (cloudData->offsetY - sineWave);
		cloudData->velocityY += force;
		cloudData->velocityY *= kDamping;

		float prevOffsetY = cloudData->offsetY;
		cloudData->offsetY += cloudData->velocityY;

		cloudData->transform->translation_.y = cloudData->basePosition.y + cloudData->offsetY;

		float deltaY = cloudData->offsetY - prevOffsetY;

		float cloudMinX = cloudData->basePosition.x - 2.0f;
		float cloudMaxX = cloudData->basePosition.x + 2.0f;
		float cloudTopY = cloudData->transform->translation_.y + 0.5f;

		if (pRightX >= cloudMinX && pLeftX <= cloudMaxX) {
			if (std::abs(pBottomY - cloudTopY) <= 0.25f) {
				if (playerOnGround && std::abs(deltaY) > 0.00001f) {
					player_->AddPositionY(deltaY);
				}
			}
		}
	}
}

void GameScene::Update() {
	GlobalVariables::GetInstance()->Update();
	Player::ApplyGlobalVariables();
	Enemy::ApplyGlobalVariables();

	if (lightManager_) {
		lightManager_->Update();
	}

#ifdef _DEBUG
	ImGui::Begin("Debug");
	if (ImGui::Button("Reload")) {
		reloadRequested_ = true;
	}

	if (stageManager_) {
		int currentIdx = stageManager_->GetCurrentStageIndex();
		int stageCount = stageManager_->GetStageCount();

		if (ImGui::SliderInt("Stage Index", &currentIdx, 0, stageCount - 1)) {
			stageManager_->SetCurrentStageIndex(currentIdx);
			reloadRequested_ = true;
		}
	}

	if (player_) {
		if (ImGui::TreeNode("Player Part Transforms")) {
			if (ImGui::TreeNode("Head")) {
				float* headPos = &(player_->GetWorldTransformHead().translation_.x);
				float* headRot = &(player_->GetWorldTransformHead().rotation_.x);
				float* headScale = &(player_->GetWorldTransformHead().scale_.x);
				ImGui::DragFloat3("Head Position", headPos, 0.01f);
				ImGui::DragFloat3("Head Rotation", headRot, 0.01f);
				ImGui::DragFloat3("Head Scale", headScale, 0.01f);
				ImGui::TreePop();
			}

			if (ImGui::TreeNode("Body")) {
				float* bodyPos = &(player_->GetWorldTransformBody().translation_.x);
				float* bodyRot = &(player_->GetWorldTransformBody().rotation_.x);
				float* bodyScale = &(player_->GetWorldTransformBody().scale_.x);
				ImGui::DragFloat3("Body Position", bodyPos, 0.01f);
				ImGui::DragFloat3("Body Rotation", bodyRot, 0.01f);
				ImGui::DragFloat3("Body Scale", bodyScale, 0.01f);
				ImGui::TreePop();
			}

			if (ImGui::TreeNode("Left Arm/Leg")) {
				float* leftPos = &(player_->GetWorldTransformLeft().translation_.x);
				float* leftRot = &(player_->GetWorldTransformLeft().rotation_.x);
				float* leftScale = &(player_->GetWorldTransformLeft().scale_.x);
				ImGui::DragFloat3("Left Position", leftPos, 0.01f);
				ImGui::DragFloat3("Left Rotation", leftRot, 0.01f);
				ImGui::DragFloat3("Left Scale", leftScale, 0.01f);
				ImGui::TreePop();
			}

			if (ImGui::TreeNode("Right Arm/Leg")) {
				float* rightPos = &(player_->GetWorldTransformRight().translation_.x);
				float* rightRot = &(player_->GetWorldTransformRight().rotation_.x);
				float* rightScale = &(player_->GetWorldTransformRight().scale_.x);
				ImGui::DragFloat3("Right Position", rightPos, 0.01f);
				ImGui::DragFloat3("Right Rotation", rightRot, 0.01f);
				ImGui::DragFloat3("Right Scale", rightScale, 0.01f);
				ImGui::TreePop();
			}

			ImGui::TreePop();
		}
	}

	if (goal_) {
		if (ImGui::TreeNode("Goal Transform")) {
			float* goalPos = &(goal_->GetWorldTransform().translation_.x);
			float* goalRot = &(goal_->GetWorldTransform().rotation_.x);
			float* goalScale = &(goal_->GetWorldTransform().scale_.x);

			ImGui::DragFloat3("Position", goalPos, 0.01f);
			ImGui::DragFloat3("Rotation", goalRot, 0.01f);
			ImGui::DragFloat3("Scale", goalScale, 0.01f);

			ImGui::TreePop();
		}
	}

	for (BaseEnemy* enemy : enemies_) {
		if (FinalBoss* finalBoss = dynamic_cast<FinalBoss*>(enemy)) {
			if (ImGui::TreeNode("FinalBoss Transform")) {
				float* pos = &(finalBoss->GetBasePosition().x);
				float* offset = &(finalBoss->GetDrawOffset().x);
				float* rot = &(finalBoss->GetWorldTransform().rotation_.x);
				float* scale = &(finalBoss->GetWorldTransform().scale_.x);

				ImGui::DragFloat3("Base Position", pos, 0.05f);
				ImGui::DragFloat3("Draw Offset", offset, 0.1f);
				ImGui::DragFloat3("Rotation", rot, 0.01f);
				ImGui::DragFloat3("Scale", scale, 0.01f);

				ImGui::TreePop();
			}
			break;
		}
	}

	if (lightManager_) {
		lightManager_->DrawImGui();
		ImGui::DragFloat("Boss Light Approach Distance", &bossLightApproachDistance_, 0.5f, 0.0f, 500.0f);
	}

	ImGui::End();
#endif

	// 背景ブロックの行列・視差計算更新
	if (backgroundBlocks_) {
		backgroundBlocks_->Update(camera_);
	}

	// ★ 操作解説用チュートリアルの更新
	if (playerTutorial_ && player_) {
		playerTutorial_->Update(player_->GetWorldTransform().translation_);
	}

	ChangePhase();

	switch (phase_) {
	case Phase::kFadeIn:
		UpdateFadeIn();
		break;
	case Phase::kPlay:
		UpdatePlay();
		break;
	case Phase::kDeath:
		UpdateDeath();
		break;
	case Phase::kFadeOut:
		UpdateFadeOut();
		break;
	}

	debugCamera_->Update();

#ifdef _DEBUG
	if (Input::GetInstance()->TriggerKey(DIK_D)) {
		isDebugCameraActive_ = !isDebugCameraActive_;
	}
#endif

	if (isDebugCameraActive_) {
		camera_.matView = debugCamera_->GetCamera().matView;
		camera_.matProjection = debugCamera_->GetCamera().matProjection;
		camera_.TransferMatrix();
	} else {
		camera_.UpdateMatrix();
		camera_.TransferMatrix();
	}

	// ブロックのワールド行列更新
	for (auto& line : blockDatas_) {
		for (auto* blockData : line) {
			if (!blockData || !blockData->transform) {
				continue;
			}
			Matrix4x4 affineMatrix = MakeAffineMatrix(blockData->transform->scale_, blockData->transform->rotation_, blockData->transform->translation_);
			blockData->transform->matWorld_ = affineMatrix;
			blockData->transform->TransferMatrix();
		}
	}

	// ステージタイルの雲のワールド行列更新
	for (auto* cloudData : cloudDatas_) {
		if (!cloudData || !cloudData->transform) {
			continue;
		}
		Matrix4x4 affineMatrix = MakeAffineMatrix(cloudData->transform->scale_, cloudData->transform->rotation_, cloudData->transform->translation_);
		cloudData->transform->matWorld_ = affineMatrix;
		cloudData->transform->TransferMatrix();
	}

	for (WorldTransform* worldTransformExplanation : worldTransformExplanations_) {
		if (!worldTransformExplanation) {
			continue;
		}
		Matrix4x4 affineMatrix = MakeAffineMatrix(worldTransformExplanation->scale_, worldTransformExplanation->rotation_, worldTransformExplanation->translation_);
		worldTransformExplanation->matWorld_ = affineMatrix;
		worldTransformExplanation->TransferMatrix();
	}

	if (lightManager_) {
		UpdateLightEffects();
		lightManager_->TransferBuffer();
	}
}

// ★ 光の演出の更新
void GameScene::UpdateLightEffects() {
	if (!lightManager_) {
		return;
	}

	FinalBoss* finalBoss = nullptr;
	for (BaseEnemy* enemy : enemies_) {
		if (FinalBoss* boss = dynamic_cast<FinalBoss*>(enemy)) {
			finalBoss = boss;
			break;
		}
	}

	const bool hasPlayer = (player_ != nullptr);
	Vector3 playerPos = {0.0f, 0.0f, 0.0f};
	if (hasPlayer) {
		playerPos = player_->GetWorldTransform().translation_;
	}

	const bool bossDefeated = (finalBoss && finalBoss->IsDefeated());

	if (finalBoss && !isBossLightTriggered_) {
		if (finalBoss->GetState() != FinalBoss::State::kStandby) {
			isBossLightTriggered_ = true;
		}
	}
	lightManager_->SetLightsEnabled(isBossLightTriggered_ && !bossDefeated);

	float night = 0.0f;
	bool isFrenzy = false;
	if (finalBoss) {
		if (finalBoss->IsDefeated()) {
			night = 0.0f;
		} else {
			night = finalBoss->GetSpawnProgress();
			isFrenzy = (finalBoss->GetState() == FinalBoss::State::kFrenzyInit || finalBoss->GetState() == FinalBoss::State::kFrenzy);
		}
	}
	lightManager_->SetNightFactor(night);
	lightManager_->SetBossFrenzy(isFrenzy);

	lightManager_->SetPlayerSpot(hasPlayer, playerPos);

	constexpr float kEnemySpotRange = 40.0f;
	bool hasEnemy = false;
	bool hasShieldEnemy = false;
	Vector3 enemyPos = {0.0f, 0.0f, 0.0f};
	Vector3 shieldEnemyPos = {0.0f, 0.0f, 0.0f};
	float nearEnemyDistance = kEnemySpotRange;
	float nearShieldEnemyDistance = kEnemySpotRange;

	if (hasPlayer) {
		for (BaseEnemy* enemy : enemies_) {
			if (!enemy || enemy->IsDead()) {
				continue;
			}
			const Vector3& pos = enemy->GetWorldTransform().translation_;
			float distanceX = std::fabs(pos.x - playerPos.x);

			if (Enemy* normalEnemy = dynamic_cast<Enemy*>(enemy)) {
				if (!normalEnemy->IsItemState() && distanceX < nearEnemyDistance) {
					nearEnemyDistance = distanceX;
					enemyPos = pos;
					hasEnemy = true;
				}
			} else if (ShieldEnemy* shieldEnemy = dynamic_cast<ShieldEnemy*>(enemy)) {
				if (!shieldEnemy->IsItemState() && distanceX < nearShieldEnemyDistance) {
					nearShieldEnemyDistance = distanceX;
					shieldEnemyPos = pos;
					hasShieldEnemy = true;
				}
			}
		}
	}
	lightManager_->SetEnemySpot(hasEnemy, enemyPos);
	lightManager_->SetShieldEnemySpot(hasShieldEnemy, shieldEnemyPos);

	lightManager_->SetBossGlow(finalBoss && !finalBoss->IsDefeatAnimationFinished() && finalBoss->GetState() != FinalBoss::State::kStandby);
	if (finalBoss) {
		lightManager_->SetCrystalPointLight(finalBoss->IsCrystal22LightActive(), finalBoss->GetCrystal22Position());

		std::vector<Vector3> meteorPositions = finalBoss->GetActiveMeteorPositions();
		lightManager_->SetMeteorPointLights(finalBoss->IsMeteorShowerActive(), meteorPositions);
	}
}

void GameScene::StartExecution(BossEnemy* boss) {
	executionBoss_ = boss;
	executionPhase_ = ExecutionPhase::kZoomIn;
	executionTimer_ = 0.0f;
	executionIdleTimer_ = 0.0f;
	executionHitCount_ = 0;
	executionInputBuffered_ = false;

	const Vector3 bossPos = boss->GetWorldTransform().translation_;
	const Vector3 playerPos = player_->GetWorldTransform().translation_;

	executionPushDirX_ = (playerPos.x < bossPos.x) ? 1.0f : -1.0f;
	const float kPlayerOffsetX = 3.2f;
	Vector3 executionPos = {bossPos.x - executionPushDirX_ * kPlayerOffsetX, bossPos.y, bossPos.z};
	player_->BeginExecution(executionPos, executionPushDirX_ > 0.0f ? LRDirection::kRight : LRDirection::kLeft);

	boss->BeginExecution();
	cameraController_->StartExecution(bossPos);
}

void GameScene::UpdateExecution() {
	const float dt = 1.0f / 60.0f;
	executionTimer_ += dt;

	skydome->Update();

	if (player_) {
		player_->Update();
	}

	UpdateClouds();

	if (executionBoss_) {
		executionBoss_->Update();
	}

	for (auto it = effects_.begin(); it != effects_.end();) {
		if (*it) {
			(*it)->Update();
			if ((*it)->IsFinished()) {
				delete (*it);
				it = effects_.erase(it);
			} else {
				++it;
			}
		} else {
			it = effects_.erase(it);
		}
	}

	auto onHit = [&](float strength, float pushDirX) {
		if (!executionBoss_) {
			return;
		}
		HitEffect* newEffect = new HitEffect();
		newEffect->Initialize(executionBoss_->GetWorldTransform().translation_);
		effects_.push_back(newEffect);
		AudioManager::GetInstance()->PlaySE(SEType::kHummer);
		executionBoss_->OnExecutionHit(pushDirX);
		cameraController_->ExecutionHit(strength);
	};

	const int32_t kHitMax = 6;
	const float kIdleLimit = 3.0f;
	const float kFinaleChargeTime = 1.0f;
	const float kLaunchTime = 2.4f;

	switch (executionPhase_) {
	case ExecutionPhase::kZoomIn:
		if (executionTimer_ >= 0.9f) {
			executionPhase_ = ExecutionPhase::kHammering;
			executionTimer_ = 0.0f;
			executionIdleTimer_ = 0.0f;
		}
		break;

	case ExecutionPhase::kHammering: {
		if (KamataEngine::Input::GetInstance()->TriggerKey(DIK_E)) {
			executionInputBuffered_ = true;
			executionIdleTimer_ = 0.0f;
		}
		executionIdleTimer_ += dt;
		if (executionIdleTimer_ >= kIdleLimit) {
			executionInputBuffered_ = true;
			executionIdleTimer_ = 0.0f;
		}

		if (player_->ConsumeExecutionHit()) {
			++executionHitCount_;
			onHit(1.0f, executionPushDirX_ * 0.5f);
		}

		if (!player_->IsExecutionSwinging()) {
			if (executionHitCount_ >= kHitMax) {
				executionPhase_ = ExecutionPhase::kFinaleCharge;
				executionTimer_ = 0.0f;
				executionInputBuffered_ = false;
				cameraController_->SetExecutionZoom(5.5f);
			} else if (executionInputBuffered_) {
				executionInputBuffered_ = false;
				player_->StartExecutionSwing(static_cast<uint32_t>(executionHitCount_ % 4));
			}
		}
		break;
	}

	case ExecutionPhase::kFinaleCharge: {
		if (static_cast<int>(executionTimer_ / 0.25f) != static_cast<int>((executionTimer_ - dt) / 0.25f)) {
			executionBoss_->OnExecutionHit(0.0f);
		}
		if (executionTimer_ >= kFinaleChargeTime) {
			executionPhase_ = ExecutionPhase::kFinaleSwing;
			executionTimer_ = 0.0f;
			player_->StartExecutionSwing(4);
		}
		break;
	}

	case ExecutionPhase::kFinaleSwing: {
		bool isHit = player_->ConsumeExecutionHit();
		if (isHit || executionTimer_ >= 1.5f) {
			onHit(2.5f, executionPushDirX_);
			for (int i = 0; i < 2; ++i) {
				HitEffect* bigEffect = new HitEffect();
				Vector3 effectPos = executionBoss_->GetWorldTransform().translation_;
				effectPos.x += (i == 0 ? -0.6f : 0.6f);
				effectPos.y += 0.6f * static_cast<float>(i);
				bigEffect->Initialize(effectPos);
				effects_.push_back(bigEffect);
			}
			executionBoss_->StartLaunch(executionPushDirX_);
			executionPhase_ = ExecutionPhase::kLaunch;
			executionTimer_ = 0.0f;
		}
		break;
	}

	case ExecutionPhase::kLaunch: {
		if (executionBoss_) {
			cameraController_->SetExecutionFocus(executionBoss_->GetWorldTransform().translation_);
		}
		cameraController_->SetExecutionZoom(5.5f + executionTimer_ * 4.0f);

		if (executionTimer_ >= kLaunchTime) {
			if (executionBoss_) {
				executionBoss_->OnDead();
				executionBoss_ = nullptr;
			}
			player_->EndExecution();
			cameraController_->EndExecution();
			executionPhase_ = ExecutionPhase::kNone;
		}
		break;
	}

	default:
		break;
	}

	if (!isDebugCameraActive_ && cameraController_) {
		cameraController_->Update();
	}
	if (playerHp_) {
		playerHp_->Update(camera_.translation_);
	}
}

void GameScene::ChangePhase() {
	switch (phase_) {
	case Phase::kFadeIn:
		if (fade_ && fade_->IsFinished()) {
			phase_ = Phase::kPlay;
		}
		break;

	case Phase::kPlay:
		if (player_ && player_->IsDeathAnimationFinished()) {
			phase_ = Phase::kDeath;

			DeathParticles* deathParticles = new DeathParticles();
			KamataEngine::Vector3 deathPosition = player_->GetWorldTransform().translation_;
			deathParticles->Initialize(modelDeathParticles_, &camera_, deathPosition);
			effects_.push_back(deathParticles);
		} else if (isGoalReached_) {
			phase_ = Phase::kFadeOut;
			if (fade_) {
				fade_->Start(Fade::Status::FadeOut, 1.0f);
			}
		}
		break;

	case Phase::kDeath:
		if (effects_.empty()) {
			phase_ = Phase::kFadeOut;
			if (fade_) {
				fade_->Start(Fade::Status::FadeOut, 1.0f);
			}
		}
		break;

	case Phase::kFadeOut:
		if (fade_ && fade_->IsFinished()) {
			finished_ = true;
		}
		break;
	}
}

void GameScene::UpdateFadeIn() {
	if (fade_) {
		fade_->Update();
	}
	skydome->Update();

	if (!isDebugCameraActive_ && cameraController_) {
		cameraController_->Update();
	}
}

void GameScene::UpdatePlay() {
	if (executionPhase_ != ExecutionPhase::kNone) {
		UpdateExecution();
		return;
	}

	skydome->Update();

	if (player_) {
		player_->Update();
	}

	UpdateClouds();

	if (goal_) {
		goal_->Update();

		if (player_ && !player_->IsDead() && goal_->IsActive()) {
			Player::AABB playerAABB = player_->GetAABB();
			Goal::AABB goalAABB = goal_->GetAABB();

			if (playerAABB.min.x < goalAABB.max.x && playerAABB.max.x > goalAABB.min.x && playerAABB.min.y < goalAABB.max.y && playerAABB.max.y > goalAABB.min.y && playerAABB.min.z < goalAABB.max.z &&
			    playerAABB.max.z > goalAABB.min.z) {
				isGoalReached_ = true;
			}
		}
	}

	for (BaseEnemy* enemy : enemies_) {
		if (enemy) {
			if (BossEnemy* boss = dynamic_cast<BossEnemy*>(enemy)) {
				boss->SetPlayer(player_.get());
			}
			if (FinalBoss* finalBoss = dynamic_cast<FinalBoss*>(enemy)) {
				finalBoss->SetPlayer(player_.get());
				for (BaseEnemy* bossCandidate : enemies_) {
					if (BossEnemy* bossEnemy = dynamic_cast<BossEnemy*>(bossCandidate)) {
						bossEnemy->SetHpBarVisible(finalBoss->IsCrystal22Landed());
					}
				}
			}
			enemy->Update();

			if (Enemy* normalEnemy = dynamic_cast<Enemy*>(enemy)) {
				if (normalEnemy->IsItemSpawnRequested()) {
					Item* newItem = new Item();
					newItem->Initialize(modelItemHp_, &camera_, enemy->GetWorldTransform().translation_, mapChipField_);
					items_.push_back(newItem);
					normalEnemy->ResetItemSpawnRequest();
				}
			} else if (ShieldEnemy* shieldEnemy = dynamic_cast<ShieldEnemy*>(enemy)) {
				if (shieldEnemy->IsItemSpawnRequested()) {
					Item* newItem = new Item();
					newItem->Initialize(modelItemHp_, &camera_, enemy->GetWorldTransform().translation_, mapChipField_);
					items_.push_back(newItem);
					shieldEnemy->ResetItemSpawnRequest();
				}
			}
		}
	}

	for (auto it = items_.begin(); it != items_.end();) {
		Item* item = *it;
		if (item) {
			item->Update();

			if (player_ && !item->IsDead()) {
				Player::AABB playerAABB = player_->GetAABB();
				Item::AABB itemAABB = item->GetAABB();

				if (playerAABB.min.x < itemAABB.max.x && playerAABB.max.x > itemAABB.min.x && playerAABB.min.y < itemAABB.max.y && playerAABB.max.y > itemAABB.min.y &&
				    playerAABB.min.z < itemAABB.max.z && playerAABB.max.z > itemAABB.min.z) {

					if (playerHp_) {
						playerHp_->IncreaseHp();
					}
					item->OnCollision(player_.get());
				}
			}

			if (item->IsDead()) {
				delete item;
				it = items_.erase(it);
			} else {
				++it;
			}
		} else {
			it = items_.erase(it);
		}
	}

	if (player_) {
		auto attackAABB = player_->GetAttackAABB();
		if (!attackAABB.has_value()) {
			crystal22AttackConsumed_ = false;
		}
		if (attackAABB.has_value()) {
			for (BaseEnemy* enemy : enemies_) {
				if (FinalBoss* finalBoss = dynamic_cast<FinalBoss*>(enemy)) {
					const Vector3& crystalPos = finalBoss->GetCrystal22Position();
					const auto& attack = attackAABB.value();
					const bool crystalHit = crystalPos.x >= attack.min.x - 1.2f && crystalPos.x <= attack.max.x + 1.2f && crystalPos.y >= attack.min.y - 1.2f && crystalPos.y <= attack.max.y + 1.2f;
					if (finalBoss->IsCrystal22Landed() && crystalHit && !crystal22AttackConsumed_) {
						finalBoss->HitCrystal22();
						crystal22AttackConsumed_ = true;
						AudioManager::GetInstance()->PlaySE(SEType::kHummer);
					}
				}
				if (enemy && !enemy->IsDead() && IsCollision(attackAABB.value(), enemy->GetAABB())) {
					BossEnemy* boss = dynamic_cast<BossEnemy*>(enemy);
					bool isHitSuccess = true;
					if (boss) {
						if (boss->IsAttacking()) {
							isHitSuccess = false;
						}
					}

					if (isHitSuccess) {
						if (player_->GetBehavior() == Behavior::kHammerSkill) {
							AudioManager::GetInstance()->PlaySE(SEType::kHummer);
						} else if (player_->GetBehavior() == Behavior::kAttack) {
							AudioManager::GetInstance()->PlaySE(SEType::kDash);
						}
					}

					enemy->OnCollision(player_.get());

					if (dynamic_cast<Enemy*>(enemy)) {
						HitEffect* newEffect = new HitEffect();
						newEffect->Initialize(enemy->GetWorldTransform().translation_);
						effects_.push_back(newEffect);
						enemy->OnDead();
					} else if (boss) {
						HitEffect* newEffect = new HitEffect();
						newEffect->Initialize(enemy->GetWorldTransform().translation_);
						effects_.push_back(newEffect);
					}
				}
			}
		}
	}

	for (auto it = effects_.begin(); it != effects_.end();) {
		if (*it) {
			(*it)->Update();
			if ((*it)->IsFinished()) {
				delete (*it);
				it = effects_.erase(it);
			} else {
				++it;
			}
		} else {
			it = effects_.erase(it);
		}
	}

	for (auto it = enemies_.begin(); it != enemies_.end();) {
		BaseEnemy* enemy = *it;
		if (enemy && enemy->IsDead()) {
			if (dynamic_cast<BossEnemy*>(enemy)) {
				if (cameraController_) {
					cameraController_->SetBoss(nullptr);
				}
			}

			delete enemy;
			it = enemies_.erase(it);
		} else {
			++it;
		}
	}

	if (executionPhase_ == ExecutionPhase::kNone && player_ && !player_->IsDead() && cameraController_) {
		for (BaseEnemy* enemy : enemies_) {
			BossEnemy* boss = dynamic_cast<BossEnemy*>(enemy);
			if (boss && boss->IsDefeated() && !boss->IsDead()) {
				StartExecution(boss);
				break;
			}
		}
	}

	if (!isBossDefeatedGoalPerfStarted_) {
		for (BaseEnemy* enemy : enemies_) {
			if (FinalBoss* finalBoss = dynamic_cast<FinalBoss*>(enemy)) {
				if (finalBoss->IsDefeatAnimationFinished() && goal_) {
					goal_->SetIsActive(true);
					if (cameraController_) {
						cameraController_->StartGoalPerformance(goal_->GetWorldTransform().translation_);
					}
					isBossDefeatedGoalPerfStarted_ = true;
					break;
				}
			}
		}
	}

	if (!isDebugCameraActive_ && cameraController_) {
		cameraController_->Update();
	}

	if (playerHp_) {
		playerHp_->Update(camera_.translation_);
	}

	if (player_) {
		player_->CheckEnemyCollision(enemies_);
	}
}

void GameScene::UpdateDeath() {
	skydome->Update();

	for (BaseEnemy* enemy : enemies_) {
		if (enemy) {
			enemy->Update();
		}
	}

	for (auto it = effects_.begin(); it != effects_.end();) {
		if (*it) {
			(*it)->Update();
			if ((*it)->IsFinished()) {
				delete (*it);
				it = effects_.erase(it);
			} else {
				++it;
			}
		} else {
			it = effects_.erase(it);
		}
	}
}

void GameScene::UpdateFadeOut() {
	if (fade_) {
		fade_->Update();
	}
	skydome->Update();

	for (BaseEnemy* enemy : enemies_) {
		if (enemy) {
			enemy->Update();
		}
	}

	for (auto it = effects_.begin(); it != effects_.end();) {
		if (*it) {
			(*it)->Update();
			if ((*it)->IsFinished()) {
				delete (*it);
				it = effects_.erase(it);
			} else {
				++it;
			}
		} else {
			it = effects_.erase(it);
		}
	}
}

void GameScene::Draw() {

	if (skydome) {
		skydome->Draw();
	}

	// 背景ブロックおよび奥行き雲の描画
	if (backgroundBlocks_) {
		backgroundBlocks_->Draw(camera_);
	}

	// ブロックの分別描画
	Model::PreDraw();
	for (auto& line : blockDatas_) {
		for (auto* blockData : line) {
			if (!blockData || !blockData->transform || !blockData->model) {
				continue;
			}
			if (std::fabs(blockData->transform->translation_.x - camera_.translation_.x) > 45.0f) {
				continue;
			}
			blockData->model->Draw(*blockData->transform, camera_);
		}
	}
	Model::PostDraw();

	// ステージ上に配置された C0 雲タイルの描画
	Model::PreDraw();
	for (auto* cloudData : cloudDatas_) {
		if (!cloudData || !cloudData->transform || !cloudData->model) {
			continue;
		}
		if (std::fabs(cloudData->transform->translation_.x - camera_.translation_.x) > 45.0f) {
			continue;
		}
		cloudData->model->Draw(*cloudData->transform, camera_);
	}
	Model::PostDraw();

	// 解説ブロックの描画
	for (WorldTransform* worldTransformExplanation : worldTransformExplanations_) {
		if (!worldTransformExplanation) {
			continue;
		}
		Model::PreDraw();
		modelExplanation_->Draw(*worldTransformExplanation, camera_);
		Model::PostDraw();
	}

	// ゴールの描画
	if (goal_) {
		Model::PreDraw();
		goal_->Draw();
		Model::PostDraw();
	}

	for (BaseEffect* effect : effects_) {
		if (effect) {
			Model::PreDraw();
			effect->Draw();
			Model::PostDraw();
		}
	}

	for (Item* item : items_) {
		if (item) {
			Model::PreDraw();
			item->Draw();
			Model::PostDraw();
		}
	}

	if (player_) {
		Model::PreDraw();
		player_->Draw();
		Model::PostDraw();
	}

	// ★ 操作解説用チュートリアル（矢印）の描画
	if (playerTutorial_) {
		Model::PreDraw();
		playerTutorial_->Draw(camera_);
		Model::PostDraw();
	}

	if (playerHp_) {
		Model::PreDraw();
		playerHp_->Draw(camera_);
		Model::PostDraw();
	}

	for (BaseEnemy* enemy : enemies_) {
		if (enemy) {
			Model::PreDraw();
			enemy->Draw();
			Model::PostDraw();
		}
	}

	if (fade_) {
		Model::PreDraw();
		fade_->Draw();
		Model::PostDraw();
	}
}