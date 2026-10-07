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
#include <numbers> // ★追加：π参照用
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

	phase_ = Phase::kFadeIn;
	finished_ = false;
	isBossSpawned_ = false;
	isGoalReached_ = false;                 // ゴール到達フラグの初期化
	isBossDefeatedGoalPerfStarted_ = false; // ボス撃破後ゴール演出フラグの初期化

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

	// 背景ブロックの生成（多層視差レイヤーの初期化）
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

	modelHammer_ = Model::CreateFromOBJ("hummer", true);

	modelEnemy_ = Model::CreateFromOBJ("enemy", true);
	modelShieldEnemy_ = Model::CreateFromOBJ("shieldEnemy", true);
	modelDeathParticles_ = Model::CreateFromOBJ("particle", true);
	modelHitEffect_ = Model::CreateFromOBJ("particle", true);

	modelPlayerHp_ = Model::CreateFromOBJ("itemHP", true);
	modelItemHp_ = Model::CreateFromOBJ("itemHP", true);
	modelGoal_ = Model::CreateFromOBJ("goal", true);               // ゴール用モデルの生成
	modelExplanation_ = Model::CreateFromOBJ("Explanation", true); // 解説ブロック用モデルの生成

	HitEffect::SetModel(modelHitEffect_);
	HitEffect::SetCamera(&camera_);

	skydome = std::make_unique<Skydome>();
	skydome->Initialize(modelSkydome_, &camera_);

	cameraController_ = std::make_unique<CameraController>();
	cameraController_->Initialize(&camera_);

	GenerateFieldObjects();

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

	Rect stageArea = {10.0f, 90.0f, 5.0f, 100.0f};
	cameraController_->SetMovableArea(stageArea);
	cameraController_->Reset();

	// ステージごとのゴール表示・カメラ演出の制御
	if (goal_) {
		if (currentStageIdx == 0 || currentStageIdx == 1) {
			// ステージ0, 1: ゴールを表示・判定有効にしてカメラ演出開始
			goal_->SetIsActive(true);
			cameraController_->StartGoalPerformance(goal_->GetWorldTransform().translation_);
		} else if (currentStageIdx == 2) {
			// ステージ2: 最初はゴールを非表示・判定無効にする
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

			// ★ すり抜けブロック (kBlockFall / B1) の生成処理を追加
			if (type == MapChipType::kBlockFall) {
				BlockData* blockData = new BlockData();
				blockData->transform = new WorldTransform();
				blockData->transform->Initialize();

				// 描画用の座標を少し上げる（例: +0.2f）
				// ※マップチップデータ(当たり判定用)は元の座標をそのまま利用しているため衝突判定は変わりません
				Vector3 pos = mapChipField_->GetMapChipPositionByIndex(j, i);
				pos.y += 0.2f;
				blockData->transform->translation_ = pos;

				// 左右に隣接するすり抜けブロックの判定
				bool hasLeft = (j > 0) && (mapChipField_->GetMapChipTypeByIndex(j - 1, i) == MapChipType::kBlockFall);
				bool hasRight = (j < numBlockHorizontal - 1) && (mapChipField_->GetMapChipTypeByIndex(j + 1, i) == MapChipType::kBlockFall);

				if (!hasLeft && hasRight) {
					// 左端
					blockData->model = modelBlockFallLeft_;
				} else if (hasLeft && !hasRight) {
					// 右端
					blockData->model = modelBlockFallRight_;
				} else {
					// 中央、または単独
					blockData->model = modelBlockFall_;
				}

				blockDatas_[i][j] = blockData;
			}

			if (type == MapChipType::kBlock) {
				BlockData* blockData = new BlockData();
				blockData->transform = new WorldTransform();
				blockData->transform->Initialize();
				blockData->transform->translation_ = mapChipField_->GetMapChipPositionByIndex(j, i);

				// --- 周囲の隣接判定 ---
				// yIndex: -1 が上、+1 が下
				// xIndex: -1 が左、+1 が右
				bool hasUp = (i > 0) && (mapChipField_->GetMapChipTypeByIndex(j, i - 1) == MapChipType::kBlock);
				bool hasDown = (i < numBlockVertical - 1) && (mapChipField_->GetMapChipTypeByIndex(j, i + 1) == MapChipType::kBlock);
				bool hasLeft = (j > 0) && (mapChipField_->GetMapChipTypeByIndex(j - 1, i) == MapChipType::kBlock);
				bool hasRight = (j < numBlockHorizontal - 1) && (mapChipField_->GetMapChipTypeByIndex(j + 1, i) == MapChipType::kBlock);

				// 要求されたルールに基づく判定 (地面/天井を優先)
				if (!hasDown) {
					// 下が空き（地面）
					blockData->model = modelBlockAbove_;
				} else if (!hasUp) {
					// 上が空き（天井）
					blockData->model = modelBlockBelow_;
				} else if (!hasLeft) {
					// 左が空き（左端の壁）
					blockData->model = modelBlockRight_;
				} else if (!hasRight) {
					// 右が空き（右端の壁）
					blockData->model = modelBlockLeft_;
				} else {
					// 四方が囲まれている場合
					blockData->model = modelBlock_;
				}

				blockDatas_[i][j] = blockData;
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
			case MapChipType::kGoal: { // ゴールの生成処理
				if (goal_ != nullptr) {
					break;
				}
				Vector3 goalPosition = mapChipField_->GetMapChipPositionByIndex(j, i);
				goal_ = std::make_unique<Goal>();
				goal_->Initialize(modelGoal_, &camera_, goalPosition);
				break;
			}
			case MapChipType::kExplanation: { // 解説ブロックの生成処理
				WorldTransform* worldTransform = new WorldTransform();
				worldTransform->Initialize();
				worldTransform->translation_ = mapChipField_->GetMapChipPositionByIndex(j, i);
				worldTransform->rotation_.y = std::numbers::pi_v<float>; // ★ Y軸回転を180度（πラジアン）に設定
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

		// カメラコントローラーにボスを登録
		if (cameraController_) {
			cameraController_->SetBoss(enemy);
		}
		break;
	}
	case 3: { // ★ E3 (ラスボス FinalBoss)
		FinalBoss* finalBoss = new FinalBoss();
		finalBoss->Initialize(modelFinalBossBody_, modelFinalBossHands_, &camera_, enemyPosition);
		enemies_.push_back(finalBoss);

		// ★ カメラコントローラーにラスボスを登録
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

void GameScene::Update() {
	GlobalVariables::GetInstance()->Update();
	Player::ApplyGlobalVariables();
	Enemy::ApplyGlobalVariables();

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

	// ★ ラスボスの ImGui 調整用UI
	for (BaseEnemy* enemy : enemies_) {
		if (FinalBoss* finalBoss = dynamic_cast<FinalBoss*>(enemy)) {
			if (ImGui::TreeNode("FinalBoss Transform")) {
				float* pos = &(finalBoss->GetBasePosition().x);
				float* offset = &(finalBoss->GetDrawOffset().x); // ★ 描画オフセットの調整項目
				float* rot = &(finalBoss->GetWorldTransform().rotation_.x);
				float* scale = &(finalBoss->GetWorldTransform().scale_.x);

				ImGui::DragFloat3("Base Position", pos, 0.05f);
				ImGui::DragFloat3("Draw Offset", offset, 0.1f); // ★ ここで描画位置だけ変更可能
				ImGui::DragFloat3("Rotation", rot, 0.01f);
				ImGui::DragFloat3("Scale", scale, 0.01f);

				ImGui::TreePop();
			}
			break;
		}
	}

	ImGui::End();
#endif

	// 背景ブロックの行列・視差計算更新
	if (backgroundBlocks_) {
		backgroundBlocks_->Update(camera_);
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

	for (WorldTransform* worldTransformExplanation : worldTransformExplanations_) {
		if (!worldTransformExplanation) {
			continue;
		}
		Matrix4x4 affineMatrix = MakeAffineMatrix(worldTransformExplanation->scale_, worldTransformExplanation->rotation_, worldTransformExplanation->translation_);
		worldTransformExplanation->matWorld_ = affineMatrix;
		worldTransformExplanation->TransferMatrix();
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
		// ★ プレイヤーの散らばり演出が終了したら Phase::kDeath (または FadeOut) へ移行
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
	skydome->Update();

	if (player_) {
		player_->Update();
	}

	if (playerHp_) {
		playerHp_->Update(camera_.translation_);
	}

	// ボス撃破時のゴール出現およびカメラ演出処理
	if (isBossSpawned_ && goal_) {
		bool bossAlive = false;
		for (BaseEnemy* enemy : enemies_) {
			if (dynamic_cast<BossEnemy*>(enemy) && !enemy->IsDead()) {
				bossAlive = true;
				break;
			}
		}
		if (!bossAlive && !isBossDefeatedGoalPerfStarted_) {
			isBossDefeatedGoalPerfStarted_ = true;
			goal_->SetIsActive(true); // ゴール出現（表示＆当たり判定を有効化）
			if (cameraController_) {
				cameraController_->StartGoalPerformance(goal_->GetWorldTransform().translation_); // カメラをゴール位置へ移動＆ズーム演出
			}
		}
	}

	// ゴールの更新と衝突判定
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
			// ボスにプレイヤーポインタを設定
			if (BossEnemy* boss = dynamic_cast<BossEnemy*>(enemy)) {
				boss->SetPlayer(player_.get());
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
		if (attackAABB.has_value()) {
			for (BaseEnemy* enemy : enemies_) {
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
			// 削除する敵がボスの場合は CameraController のポインタをクリア
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

	if (!isDebugCameraActive_ && cameraController_) {
		cameraController_->Update();
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

	if (backgroundBlocks_) {
		backgroundBlocks_->Draw(camera_);
	}

	// ブロックの分別描画
	for (auto& line : blockDatas_) {
		for (auto* blockData : line) {
			if (!blockData || !blockData->transform || !blockData->model) {
				continue;
			}
			Model::PreDraw();
			blockData->model->Draw(*blockData->transform, camera_);
			Model::PostDraw();
		}
	}

	// 解説ブロックの描画
	for (WorldTransform* worldTransformExplanation : worldTransformExplanations_) {
		if (!worldTransformExplanation) {
			continue;
		}
		Model::PreDraw();
		modelExplanation_->Draw(*worldTransformExplanation, camera_);
		Model::PostDraw();
	}

	skydome->Draw();

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