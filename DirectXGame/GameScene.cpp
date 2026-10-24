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
	isBossLightTriggered_ = false;          // ラスボス出現演出による光の演出フラグの初期化（出現演出が始まるまでは赤いスポットライトを消す）

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

	// 背景ブロックの生成（現在のステージインデックスに対応するCSVパスを指定して初期化）
	backgroundBlocks_ = std::make_unique<BackgroundBlocks>();
	std::string bgCsvPath = "Resources/backgroundDatas" + std::to_string(currentStageIdx) + ".csv";
	backgroundBlocks_->Initialize(mapChipField_, bgCsvPath);

	// ブロックモデルを種類別に読み込み
	modelBlock_ = Model::CreateFromOBJ("block", true);
	modelBlockFall_ = Model::CreateFromOBJ("blockFall", true);
	modelBlockFallLeft_ = Model::CreateFromOBJ("blockFallLeft", true);
	modelBlockFallRight_ = Model::CreateFromOBJ("blockFallRight", true);
	modelBlockLeft_ = Model::CreateFromOBJ("blockRight", true);
	modelBlockRight_ = Model::CreateFromOBJ("blockLeft", true);
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

	// ★ 全モデルに LightManager のライトグループを設定する
	//   （Model::Draw は model->lightGroup_ をバインドするため、設定しないとデフォルトライトで描画される）
	{
		KamataEngine::Model* lightTargets[] = {
		    modelEnemy_,    modelShieldEnemy_, modelSkydome_,        modelPlayerHead_,     modelPlayerBody_, modelPlayerLeft_, modelPlayerRight_, modelBossHead_,   modelBossBody_,
		    modelBossLeft_, modelBossRight_,   modelDeathParticles_, modelHitEffect_,      modelHammer_,     modelPlayerHp_,   modelItemHp_,      modelGoal_,       modelExplanation_,
		    modelBlock_,    modelBlockFall_,   modelBlockFallLeft_,  modelBlockFallRight_, modelBlockLeft_,  modelBlockRight_, modelBlockAbove_,  modelBlockBelow_, modelCloud_,
		};
		for (KamataEngine::Model* model : lightTargets) {
			lightManager_->ApplyToModel(model, LightManager::GroupType::kWorld);
		}

		// ラスボスは専用のライトグループ（専用スポットライト・暗転演出）
		lightManager_->ApplyToModel(modelFinalBossBody_, LightManager::GroupType::kBoss);
		for (KamataEngine::Model* model : modelFinalBossHands_) {
			lightManager_->ApplyToModel(model, LightManager::GroupType::kBoss);
		}

		for (KamataEngine::Model* model : modelFinalBossCrystals_) { // ★ 追加
			lightManager_->ApplyToModel(model, LightManager::GroupType::kBoss);
		}
		lightManager_->ApplyToModel(modelFinalBossCrystal22_, LightManager::GroupType::kBoss);
		// 奥行き背景は中景と遠景で別々のライトグループ（遠景ほど暗く、彩度も低い）
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

	GenerateFieldObjects();

	// ★ プレイヤーのモデルにもマップタイルなどと同じライティング（平行光源）を設定する
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

	// ★ カメラの移動範囲：CSVで実際に使われている横幅に合わせる（ステージごとに横幅が違うため）
	Rect stageArea = {10.0f, static_cast<float>(mapChipField_->GetUsedBlockHorizontal()) - 10.0f, 5.0f, static_cast<float>(mapChipField_->GetNumBlockVertical()) + 80.0f};
	cameraController_->SetMovableArea(stageArea);
	cameraController_->Reset();

	// ステージごとのゴール表示・カメラ演出の制御
	if (goal_) {
		if (currentStageIdx == 0 || currentStageIdx == 1) {
			// ステージ0, 1: ゴールを表示・判定有効にしてカメラ演出開始（中ボスを倒さなくてもたどり着ける）
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
			uint8_t subID = mapChipField_->GetMapChipSubIDByIndex(j, i);

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

				// --- ブロックの向きの判定（MapChipField の共通判定を使用） ---
				// 空白に一番近いブロックだけが向きを持ち、それ以外は通常のブロックになる
				switch (mapChipField_->GetBlockFaceByIndex(j, i)) {
				case BlockFace::kAbove:
					// 上が空白（地面のブロック）
					blockData->model = modelBlockAbove_;
					break;
				case BlockFace::kBelow:
					// 下が空白（天井のブロック）
					blockData->model = modelBlockBelow_;
					break;
				case BlockFace::kRight:
					// 右が空白（左壁のブロック。右に向いている）
					blockData->model = modelBlockRight_;
					break;
				case BlockFace::kLeft:
					// 左が空白（右壁のブロック。左に向いている）
					blockData->model = modelBlockLeft_;
					break;
				default:
					// 空白に面していない通常のブロック
					blockData->model = modelBlock_;
					break;
				}

				blockDatas_[i][j] = blockData;
			}

			// ★ C0 (kCloud / subID: 0) がマップチップに指定された場合の生成処理（基準位置と揺れ用パラメータを初期化）
			if (type == MapChipType::kCloud && subID == 0) {
				BlockData* cloudData = new BlockData();
				cloudData->transform = new WorldTransform();
				cloudData->transform->Initialize();
				Vector3 pos = mapChipField_->GetMapChipPositionByIndex(j, i);
				cloudData->transform->translation_ = pos;
				cloudData->basePosition = pos; // ★ 追加: 初期基準位置の保存
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
		finalBoss->Initialize(modelFinalBossBody_, modelFinalBossHands_, modelFinalBossCrystals_, modelFinalBossCrystal22_, &camera_, enemyPosition); // ★ 引数追加
		finalBoss->SetMapChipField(mapChipField_); // ★ 追加: 地形衝突判定用マップチップフィールドを設定
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

// ★ 追加: 雲のふわふわ揺れアニメーションとプレイヤーの動的追従処理
void GameScene::UpdateClouds() {
	if (!player_) {
		return;
	}

	const Vector3& pPos = player_->GetWorldTransform().translation_;
	float pLeftX = pPos.x - 0.4f;
	float pRightX = pPos.x + 0.4f;
	float pBottomY = pPos.y - 0.1f;
	bool playerOnGround = player_->IsOnGround();

	// 時間経過による自動的な微揺れタイマーなどを加算するか、サイン波でベースを動かす
	static float timer = 0.0f;
	timer += 1.0f / 60.0f;

	for (auto* cloudData : cloudDatas_) {
		if (!cloudData || !cloudData->transform)
			continue;

		// バネ運動
		constexpr float kSpring = 0.08f;
		constexpr float kDamping = 0.88f;

		// 常にゆらゆら動く微小な外力を加える場合
		float sineWave = std::sin(timer * 3.0f + cloudData->basePosition.x) * 0.02f;

		float force = -kSpring * (cloudData->offsetY - sineWave);
		cloudData->velocityY += force;
		cloudData->velocityY *= kDamping;

		float prevOffsetY = cloudData->offsetY;
		cloudData->offsetY += cloudData->velocityY;

		cloudData->transform->translation_.y = cloudData->basePosition.y + cloudData->offsetY;

		// オフセットの変化量
		float deltaY = cloudData->offsetY - prevOffsetY;

		// プレイヤーが雲の上に立っている場合、雲の上下運動に合わせてプレイヤー位置も追従させる
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

	// ★ LightManager の更新
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

	// ★ LightManager 自身の ImGui 描画関数を呼ぶ
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

	// ★ 光の演出（点灯・夜・追従スポットライト）を更新してからライトを転送する
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

	// ラスボスを探す
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

	// ★ ラスボスを倒したかどうか（倒したら夜から朝のライティングへ戻し、スポットライトも消す）
	const bool bossDefeated = (finalBoss && finalBoss->IsDefeated());

	// ★ 変更：ラスボスが出現演出を開始（kStandby解除）した瞬間のみ光の演出を開始
	if (finalBoss && !isBossLightTriggered_) {
		if (finalBoss->GetState() != FinalBoss::State::kStandby) {
			isBossLightTriggered_ = true;
		}
	}
	// 出現演出が始まるまでは赤いスポットライトを消す（朝のライティングは常に有効）
	// ★ ラスボスを倒したら、プレイヤーと雑魚敵たちの赤いスポットライトもなめらかに消す
	lightManager_->SetLightsEnabled(isBossLightTriggered_ && !bossDefeated);

	// ラスボスの出現演出に合わせて、朝のライティングから夜のライティングへ段々切り替える
	float night = 0.0f;
	bool isFrenzy = false;
	if (finalBoss) {
		// ★ 変更: ラスボスが倒された場合は夜から朝のライティングへ切り替える
		if (finalBoss->IsDefeated()) {
			night = 0.0f;
		} else {
			night = finalBoss->GetSpawnProgress();
			isFrenzy = (finalBoss->GetState() == FinalBoss::State::kFrenzyInit || finalBoss->GetState() == FinalBoss::State::kFrenzy);
		}
	}
	lightManager_->SetNightFactor(night);
	lightManager_->SetBossFrenzy(isFrenzy);

	// プレイヤーのスポットライト
	lightManager_->SetPlayerSpot(hasPlayer, playerPos);

	// プレイヤーに一番近い普通敵・盾敵のスポットライト
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

	// ラスボスの発光（スポットライトの代わりに、モデル本来の色を保ったまま本体を光らせる。出現演出が始まってから）
	// ★ 撃破アニメーションが終わるまでは光らせたままにする（終わったらなめらかに消える）
	lightManager_->SetBossGlow(finalBoss && !finalBoss->IsDefeatAnimationFinished() && finalBoss->GetState() != FinalBoss::State::kStandby);
	if (finalBoss) {
		lightManager_->SetCrystalPointLight(finalBoss->IsCrystal22LightActive(), finalBoss->GetCrystal22Position());

		// ★ 流星群攻撃中に降下する finalBossCrystal_22 へ黄色のポイントライトを適用
		std::vector<Vector3> meteorPositions = finalBoss->GetActiveMeteorPositions();
		lightManager_->SetMeteorPointLights(finalBoss->IsMeteorShowerActive(), meteorPositions);
	}
}

// ★ 処刑演出の開始
void GameScene::StartExecution(BossEnemy* boss) {
	executionBoss_ = boss;
	executionPhase_ = ExecutionPhase::kZoomIn;
	executionTimer_ = 0.0f;
	executionIdleTimer_ = 0.0f;
	executionHitCount_ = 0;
	executionInputBuffered_ = false;

	const Vector3 bossPos = boss->GetWorldTransform().translation_;
	const Vector3 playerPos = player_->GetWorldTransform().translation_;

	// プレイヤーがいた側に立ったまま、中ボスの横に固定する（中ボスはプレイヤーと反対側へ吹き飛ぶ）
	executionPushDirX_ = (playerPos.x < bossPos.x) ? 1.0f : -1.0f;
	const float kPlayerOffsetX = 3.2f; // 中ボスの横の立ち位置（ハンマーが届く距離）
	Vector3 executionPos = {bossPos.x - executionPushDirX_ * kPlayerOffsetX, bossPos.y, bossPos.z};
	player_->BeginExecution(executionPos, executionPushDirX_ > 0.0f ? LRDirection::kRight : LRDirection::kLeft);

	boss->BeginExecution();
	cameraController_->StartExecution(bossPos);
}

// ★ 処刑演出の更新
void GameScene::UpdateExecution() {
	const float dt = 1.0f / 60.0f;
	executionTimer_ += dt;

	// 演出に関係するものだけを更新する（他の敵・アイテムなどは止まる）
	skydome->Update();

	if (player_) {
		player_->Update();
	}

	// ★ 追加: 雲のふわふわ揺れアニメーションとプレイヤー連動の更新
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

	// 連撃が当たった時の演出（エフェクト・SE・中ボスののけぞり・カメラのズームイン + シェイク + 傾き）
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

	// 定数：連撃の回数など
	const int32_t kHitMax = 6;            // この回数だけ連撃が当たったら最後の一撃へ
	const float kIdleLimit = 3.0f;        // Eキーが押されなくても、この時間が経つと自動で 1 発出す
	const float kFinaleChargeTime = 1.0f; // 最後の一撃の「ため」の時間
	const float kLaunchTime = 2.4f;       // 打ち上げの時間

	switch (executionPhase_) {
	case ExecutionPhase::kZoomIn:
		// 中ボスへズームイン
		if (executionTimer_ >= 0.9f) {
			executionPhase_ = ExecutionPhase::kHammering;
			executionTimer_ = 0.0f;
			executionIdleTimer_ = 0.0f;
		}
		break;

	case ExecutionPhase::kHammering: {
		// Eキー連打で連撃（スイング中に押された分は 1 回だけ先行入力として保持する）
		if (KamataEngine::Input::GetInstance()->TriggerKey(DIK_E)) {
			executionInputBuffered_ = true;
			executionIdleTimer_ = 0.0f;
		}
		executionIdleTimer_ += dt;
		if (executionIdleTimer_ >= kIdleLimit) {
			executionInputBuffered_ = true;
			executionIdleTimer_ = 0.0f;
		}

		// スイングが当たったタイミング
		if (player_->ConsumeExecutionHit()) {
			++executionHitCount_;
			onHit(1.0f, executionPushDirX_ * 0.5f);
		}

		if (!player_->IsExecutionSwinging()) {
			if (executionHitCount_ >= kHitMax) {
				// 連撃は終わり。最後の一撃のため
				executionPhase_ = ExecutionPhase::kFinaleCharge;
				executionTimer_ = 0.0f;
				executionInputBuffered_ = false;
				cameraController_->SetExecutionZoom(5.5f); // さらに寄る
			} else if (executionInputBuffered_) {
				executionInputBuffered_ = false;
				player_->StartExecutionSwing(static_cast<uint32_t>(executionHitCount_ % 4)); // 連撃の 1 ~ 4 段目を順番に使う
			}
		}
		break;
	}

	case ExecutionPhase::kFinaleCharge: {
		// 最後の一撃の「ため」：中ボスが小刻みに震える
		if (static_cast<int>(executionTimer_ / 0.25f) != static_cast<int>((executionTimer_ - dt) / 0.25f)) {
			executionBoss_->OnExecutionHit(0.0f);
		}
		if (executionTimer_ >= kFinaleChargeTime) {
			executionPhase_ = ExecutionPhase::kFinaleSwing;
			executionTimer_ = 0.0f;
			player_->StartExecutionSwing(4); // 連撃の最終段（一番大きなハンマーの振り）で思い切り叩く
		}
		break;
	}

	case ExecutionPhase::kFinaleSwing: {
		// 当たった瞬間に中ボスをホームランで打ち上げる（万一当たりが通知されなくても、一定時間で打ち上げる）
		bool isHit = player_->ConsumeExecutionHit();
		if (isHit || executionTimer_ >= 1.5f) {
			onHit(2.5f, executionPushDirX_);
			// 大きなヒットエフェクトを重ねる
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
		// 飛んでいく中ボスをカメラが追いかけ、だんだん引いていく
		if (executionBoss_) {
			cameraController_->SetExecutionFocus(executionBoss_->GetWorldTransform().translation_);
		}
		cameraController_->SetExecutionZoom(5.5f + executionTimer_ * 4.0f);

		if (executionTimer_ >= kLaunchTime) {
			// 演出終了：中ボスを消して、プレイヤーの操作とカメラを元に戻す
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

	// カメラとHPの更新
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
	// ★ 処刑演出中は、時間が止まったように演出に関係するものだけを更新する
	if (executionPhase_ != ExecutionPhase::kNone) {
		UpdateExecution();
		return;
	}

	skydome->Update();

	if (player_) {
		player_->Update();
	}

	// ★ 追加: 雲のふわふわ揺れアニメーションとプレイヤー連動の更新
	UpdateClouds();

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
			if (FinalBoss* finalBoss = dynamic_cast<FinalBoss*>(enemy)) {
				finalBoss->SetPlayer(player_.get());
				// ★ ラスボス戦でのみ、中ボスのHPバーの可視性をラスボスの状態と連動させる
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
			// ★ 追加: プレイヤーの攻撃判定が終わったら、ラスボス本体の多重ヒット防止フラグをリセット
			for (BaseEnemy* enemy : enemies_) {
				if (FinalBoss* finalBoss = dynamic_cast<FinalBoss*>(enemy)) {
					finalBoss->ResetBodyHitConsumed();
				}
			}
		}
		if (attackAABB.has_value()) {
			for (BaseEnemy* enemy : enemies_) {
				if (FinalBoss* finalBoss = dynamic_cast<FinalBoss*>(enemy)) {
					const Vector3& crystalPos = finalBoss->GetCrystal22Position();
					const auto& attack = attackAABB.value();
					// ★ 変更: 当たり判定のマージンを1.2fに広げて攻撃が確実に当たるように調整
					const bool crystalHit = crystalPos.x >= attack.min.x - 1.2f && crystalPos.x <= attack.max.x + 1.2f && crystalPos.y >= attack.min.y - 1.2f && crystalPos.y <= attack.max.y + 1.2f;
					if (finalBoss->IsCrystal22Landed() && crystalHit && !crystal22AttackConsumed_) {
						finalBoss->HitCrystal22();
						crystal22AttackConsumed_ = true;
						AudioManager::GetInstance()->PlaySE(SEType::kHummer);
					}
				}
				if (enemy && !enemy->IsDead() && IsCollision(attackAABB.value(), enemy->GetAABB())) {
					// ★ ハンマー連撃: 通常の敵・盾の敵は一撃で倒さず、連撃（コンボ）で倒す。連撃中は敵の行動が止まる
					//   打撃が当たるタイミング（IsHammerHitFrame）で 1 スイングにつき 1 回だけヒットし、ヒットのたびにカメラ演出が入る
					if (player_->GetBehavior() == Behavior::kHammerSkill && (dynamic_cast<Enemy*>(enemy) || dynamic_cast<ShieldEnemy*>(enemy))) {
						if (player_->IsHammerHitFrame()) {
							const float pushDirX = (player_->GetLRDirection() == LRDirection::kRight) ? 1.0f : -1.0f;
							const uint32_t swingId = player_->GetHammerSwingId();
							const Vector3 hitPos = enemy->GetWorldTransform().translation_;

							bool isRegistered = false;
							bool isFinisher = false;
							if (Enemy* normalEnemy = dynamic_cast<Enemy*>(enemy)) {
								isRegistered = normalEnemy->ReceiveHammerHit(swingId, pushDirX);
								isFinisher = normalEnemy->IsItemState();
							} else if (ShieldEnemy* shieldEnemy = dynamic_cast<ShieldEnemy*>(enemy)) {
								isRegistered = shieldEnemy->ReceiveHammerHit(swingId, pushDirX);
								isFinisher = shieldEnemy->IsItemState();
							}

							if (isRegistered) {
								AudioManager::GetInstance()->PlaySE(SEType::kHummer);

								HitEffect* newEffect = new HitEffect();
								newEffect->Initialize(hitPos);
								effects_.push_back(newEffect);

								// ★ 敵に当たったような感じのカメラ演出（フィニッシュは強め）
								if (cameraController_) {
									cameraController_->CombatHit(hitPos, isFinisher ? 1.6f : 1.0f);
								}
							}
						}
						continue;
					}

					BossEnemy* boss = dynamic_cast<BossEnemy*>(enemy);
					FinalBoss* finalBoss = dynamic_cast<FinalBoss*>(enemy);

					// ★ 追加: ラスボス本体にすでに攻撃がヒットしている場合は重複処理（SE・エフェクト生成）をスキップ
					if (finalBoss && finalBoss->IsBodyHitConsumed()) {
						continue;
					}

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
					} else if (boss || finalBoss) { // ★ 修正: FinalBoss本体ヒット時もヒットエフェクトを生成
						HitEffect* newEffect = new HitEffect();
						newEffect->Initialize(player_->GetWorldTransform().translation_);
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

	// ★ 中ボスを倒したら処刑演出を開始する（カメラ更新より前に開始して、同じフレームからズームインさせる）
	if (executionPhase_ == ExecutionPhase::kNone && player_ && !player_->IsDead() && cameraController_) {
		for (BaseEnemy* enemy : enemies_) {
			BossEnemy* boss = dynamic_cast<BossEnemy*>(enemy);
			if (boss && boss->IsDefeated() && !boss->IsDead()) {
				StartExecution(boss);
				break;
			}
		}
	}

	// ★ ラスボスの撃破アニメーションが終わったら、ゴールを表示してゴール追従カメラ演出を開始する
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

	// ★ 修正: 最背面である天球（Skydome）を一番最初に描画する
	if (skydome) {
		skydome->Draw();
	}

	// 背景ブロックおよび奥行き雲の描画
	if (backgroundBlocks_) {
		backgroundBlocks_->Draw(camera_);
	}

	// ブロックの分別描画（描画前処理は 1 回だけ。ステージが大きいので、カメラから横に遠いブロックは描画しない）
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

	// ★ ステージ上に配置された C0 雲タイルの描画
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
