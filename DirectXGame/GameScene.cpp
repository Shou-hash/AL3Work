#include "GameScene.h"
#include "AudioManager.h"
#include "DeathParticles.h"
#include "Enemy.h"
#include "GlobalVariables.h"
#include "Goal.h"
#include "HitEffect.h"
#include "Item.h"
#include "Matrix4x4.h"
#include "ModelCache.h"
#include "Player.h"
#include "ShieldEnemy.h"
#include "StageManager.h"
#include <algorithm>
#include <cmath>
#include <numbers> // ★追加：π参照用
#ifdef _DEBUG
#include <imgui.h>
#endif

using namespace KamataEngine;

namespace {
// ステージのブロック・雲を描画する、カメラからの横方向の距離（これを超えたら描画しない）
constexpr float kStageDrawRangeX = 45.0f;

// ラスボスの処刑演出のパラメータ
constexpr float kFinalExecutionZoomFar = 22.0f;     // 最初に大きく引くカメラの距離
constexpr float kFinalExecutionZoomNear = 13.0f;    // 力が溜まりきったときのカメラの距離
constexpr float kFinalExecutionZoomShatter = 31.0f; // 砕け散るときに引いていくカメラの距離
constexpr float kFinalExecutionChargeTime = 2.0f;   // Eキー長押しで力が溜まりきるまでの時間（秒）
} // namespace

GameScene::~GameScene() {
	delete fade_;

	// ★ 軽量化: モデルは ModelCache で使い回しているため、ここでは解放しない

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
	delete mapChipField_;

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
}

// ★ 軽量化: モデルの事前読み込み（タイトル・セレクト画面の待ち時間に 1 フレームにつき 1 個ずつ読み込んでおく）
void GameScene::PreloadModelsStep() {
	static std::vector<std::pair<std::string, std::string>> list;
	static size_t index = 0;

	// ★ 軽量化: 呼び出された直後の数フレームは読み込まず、その後も 3 フレームに 1 個ずつ読み込む
	//   （タイトルの表示・フェードインと重なって、1 フレームに重い読み込みが集中してカクつかないようにする）
	constexpr uint32_t kPreloadStartDelayFrames = 45;
	constexpr uint32_t kPreloadIntervalFrames = 3;
	static uint32_t frameCounter = 0;
	if (index > 0 && index >= list.size()) {
		return;
	}
	++frameCounter;
	if (frameCounter < kPreloadStartDelayFrames || (frameCounter % kPreloadIntervalFrames) != 0) {
		return;
	}

	if (list.empty()) {
		const char* commonNames[] = {"block",       "blockFall",   "blockFallLeft", "blockFallRight", "blockRight", "blockLeft", "blockAbove",  "blockBelow", "cloud3",     "skydome",
		                             "player_head", "player_body", "player_left",   "player_right",   "hummer",     "enemy",     "shieldEnemy", "goal",       "Explanation"};
		for (const char* name : commonNames) {
			list.emplace_back(name, "");
		}
		list.emplace_back("particle", "death");
		list.emplace_back("particle", "hit");
		list.emplace_back("itemHP", "player");
		list.emplace_back("itemHP", "item");

		// 背景ブロック（BackgroundBlocks）用
		for (const char* name : {"block", "blockAbove", "blockBelow", "blockLeft", "blockRight"}) {
			list.emplace_back(name, "bgTile");
		}
		list.emplace_back("block", "bgFar");
		list.emplace_back("blockAbove", "bgMid");
		list.emplace_back("cloud3", "bgFar");

		// 中ボス・ラスボス用（登場するステージが少ないので最後に読み込む）
		for (const char* name : {"bossEnemy_body", "bossEnemy_head", "bossEnemy_left", "bossEnemy_right", "finalBoss"}) {
			list.emplace_back(name, "");
		}
		for (int i = 0; i < 7; ++i) {
			list.emplace_back("finalBossHand_" + std::to_string(i + 1), "");
		}
		for (int i = 0; i < 21; ++i) {
			list.emplace_back("finalBossCrystal_" + std::to_string(i + 1), "");
		}
		list.emplace_back("finalBossCrystal_22", "");
	}

	if (index >= list.size()) {
		return;
	}
	ModelCache::Load(list[index].first, list[index].second);
	++index;
}

void GameScene::Initialize(StageManager* stageDataManager) {

	// ★ ゲームシーンに入った時にBGMを再生
	AudioManager::GetInstance()->PlayBGM(BGMType::kGame);

	stageManager_ = stageDataManager;

	// ★ ライトマネージャの初期化
	lightManager_ = std::make_unique<LightManager>();
	lightManager_->Initialize();

	pendingPauseMenu_ = Pause::MenuType::Count; // ポーズ選択項目の初期化

	phase_ = Phase::kFadeIn;
	finished_ = false;
	reloadRequested_ = false; // フラグの初期化
	isSelectRequested_ = false;
	isTitleRequested_ = false;
	isExitRequested_ = false;

	isBossSpawned_ = false;
	isGoalReached_ = false;                            // ゴール到達フラグの初期化
	isBossDefeatedGoalPerfStarted_ = false;            // ボス撃破後ゴール演出フラグの初期化
	isBossLightTriggered_ = false;                     // ラスボス出現演出による光の演出フラグの初期化（出現演出が始まるまでは赤いスポットライトを消す）
	finalExecutionPhase_ = FinalExecutionPhase::kNone; // ラスボスの処刑演出フラグの初期化
	executionFinalBoss_ = nullptr;
	isFinalExecutionDone_ = false;

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

	// ★ 軽量化: このステージに登場する敵の種類を先に調べ、登場しない敵のモデルは読み込まない
	bool needEnemy = false;
	bool needShieldEnemy = false;
	bool needBossEnemy = false;
	bool needFinalBoss = false;
	for (uint32_t y = 0; y < mapChipField_->GetNumBlockVertical(); ++y) {
		for (uint32_t x = 0; x < mapChipField_->GetNumBlockHorizontal(); ++x) {
			if (mapChipField_->GetMapChipTypeByIndex(x, y) != MapChipType::kEnemy) {
				continue;
			}
			switch (mapChipField_->GetMapChipSubIDByIndex(x, y)) {
			case 0:
				needEnemy = true;
				break;
			case 1:
				needShieldEnemy = true;
				break;
			case 2:
				needBossEnemy = true;
				break;
			case 3:
				needFinalBoss = true;
				break;
			default:
				break;
			}
		}
	}

	// ブロックモデルを種類別に読み込み
	modelBlock_ = ModelCache::Load("block");
	modelBlockFall_ = ModelCache::Load("blockFall");
	modelBlockFallLeft_ = ModelCache::Load("blockFallLeft");
	modelBlockFallRight_ = ModelCache::Load("blockFallRight");
	modelBlockLeft_ = ModelCache::Load("blockRight");
	modelBlockRight_ = ModelCache::Load("blockLeft");
	modelBlockAbove_ = ModelCache::Load("blockAbove");
	modelBlockBelow_ = ModelCache::Load("blockBelow");

	// ★ 追加: 雲モデル (cloud3) の読み込み
	modelCloud_ = ModelCache::Load("cloud3");

	worldTransform_.Initialize();
	camera_.Initialize();
	debugCamera_ = new DebugCamera(1280, 720);
	modelSkydome_ = ModelCache::Load("skydome");

	modelPlayerHead_ = ModelCache::Load("player_head");
	modelPlayerBody_ = ModelCache::Load("player_body");
	modelPlayerLeft_ = ModelCache::Load("player_left");
	modelPlayerRight_ = ModelCache::Load("player_right");

	if (needBossEnemy) {
		modelBossBody_ = ModelCache::Load("bossEnemy_body");
		modelBossHead_ = ModelCache::Load("bossEnemy_head");
		modelBossLeft_ = ModelCache::Load("bossEnemy_left");
		modelBossRight_ = ModelCache::Load("bossEnemy_right");
	}

	// ★ 追加: 攻撃できるクリスタルを指す矢印モデル（ラスボスのステージだけ読み込む）
	modelArrow_ = needFinalBoss ? ModelCache::Load("Arrow2") : nullptr;

	if (needFinalBoss) {
		// ラスボス本体モデル
		modelFinalBossBody_ = ModelCache::Load("finalBoss");

		// ラスボス7本の手のモデル
		for (int i = 0; i < 7; ++i) {
			std::string modelName = "finalBossHand_" + std::to_string(i + 1);
			modelFinalBossHands_[i] = ModelCache::Load(modelName);
		}

		// ラスボス21個のクリスタルのモデル
		for (int i = 0; i < 21; ++i) {
			std::string modelName = "finalBossCrystal_" + std::to_string(i + 1);
			modelFinalBossCrystals_[i] = ModelCache::Load(modelName);
		}
		// ★ 攻撃専用の finalBossCrystal_22 モデルを別に読み込む
		modelFinalBossCrystal22_ = ModelCache::Load("finalBossCrystal_22");
	}

	modelHammer_ = ModelCache::Load("hummer");

	if (needEnemy) {
		modelEnemy_ = ModelCache::Load("enemy");
	}
	if (needShieldEnemy) {
		modelShieldEnemy_ = ModelCache::Load("shieldEnemy");
	}
	modelDeathParticles_ = ModelCache::Load("particle", "death");
	modelHitEffect_ = ModelCache::Load("particle", "hit");

	modelPlayerHp_ = ModelCache::Load("itemHP", "player");
	modelItemHp_ = ModelCache::Load("itemHP", "item");
	modelGoal_ = ModelCache::Load("goal");               // ゴール用モデルの生成
	modelExplanation_ = ModelCache::Load("Explanation"); // 解説ブロック用モデルの生成

	// ★ 全モデルに LightManager のライトグループを設定する
	{
		KamataEngine::Model* lightTargets[] = {
		    modelEnemy_,         modelShieldEnemy_,    modelSkydome_,   modelPlayerHead_, modelPlayerBody_, modelPlayerLeft_, modelPlayerRight_, modelBossHead_,    modelBossBody_, modelBossLeft_,
		    modelBossRight_,     modelDeathParticles_, modelHitEffect_, modelHammer_,     modelPlayerHp_,   modelItemHp_,     modelGoal_,        modelExplanation_, modelBlock_,    modelBlockFall_,
		    modelBlockFallLeft_, modelBlockFallRight_, modelBlockLeft_, modelBlockRight_, modelBlockAbove_, modelBlockBelow_, modelCloud_,       modelArrow_,
		};
		for (KamataEngine::Model* model : lightTargets) {
			if (model) {
				lightManager_->ApplyToModel(model, LightManager::GroupType::kWorld);
			}
		}

		// ラスボスは専用のライトグループ
		if (needFinalBoss) {
			lightManager_->ApplyToModel(modelFinalBossBody_, LightManager::GroupType::kBoss);
			for (KamataEngine::Model* model : modelFinalBossHands_) {
				lightManager_->ApplyToModel(model, LightManager::GroupType::kBoss);
			}

			for (KamataEngine::Model* model : modelFinalBossCrystals_) {
				lightManager_->ApplyToModel(model, LightManager::GroupType::kBoss);
			}
			lightManager_->ApplyToModel(modelFinalBossCrystal22_, LightManager::GroupType::kBoss);
		}
		if (backgroundBlocks_) {
			backgroundBlocks_->SetLightGroups(
			    lightManager_->GetLightGroup(LightManager::GroupType::kBackgroundNear), lightManager_->GetLightGroup(LightManager::GroupType::kBackgroundMid),
			    lightManager_->GetLightGroup(LightManager::GroupType::kBackgroundFar));
		}
	}

	HitEffect::SetModel(modelHitEffect_);
	HitEffect::SetCamera(&camera_);

	// ★ 追加: フォント演出専用クラスの初期化
	comboFont_ = std::make_unique<Font>();
	comboFont_->Initialize(&camera_);
	if (lightManager_) {
		comboFont_->ApplyLightManager(*lightManager_);
	}

	// ★ 追加: ゲーム画面モード(Return, Retry, Title, Exit)でポーズクラスを初期化
	pause_ = std::make_unique<Pause>();
	pause_->Initialize(&camera_);
	pause_->SetMode(Pause::Mode::Game);                         // ゲーム画面モードに設定
	pause_->SetScale({0.085f, 0.07f, 0.07f});                   // セレクトシーンと同じスケールに設定
	pause_->SetOffsets({0.0f, 8.0f, 3.5f}, {0.0f, 0.0f, 3.5f}); // セレクトシーンと同じ奥行きZ位置に設定

	// ★ 各操作説明モデルの個別の拡大倍率(XYZ)と相対オフセット(XYZ)を初期化
	pause_->SetMoveExplanationParam({8.5f, 10.5f, 10.5f}, {-1.5f, 0.65f, -0.05f});
	pause_->SetAttackExplanationParam({8.5f, 10.5f, 10.5f}, {-1.6f, 0.30f, -0.05f});
	pause_->SetDashExplanationParam({8.5f, 10.5f, 10.5f}, {-1.2f, -0.15f, -0.05f});

	if (lightManager_) {
		pause_->ApplyLightManager(*lightManager_);
	}

	skydome = std::make_unique<Skydome>();
	skydome->Initialize(modelSkydome_, &camera_);

	cameraController_ = std::make_unique<CameraController>();
	cameraController_->Initialize(&camera_);

	GenerateFieldObjects();

	// ★ 追加: 攻撃できるクリスタルを指す矢印の初期化（ラスボスのステージだけ）
	crystalArrow_.reset();
	if (modelArrow_) {
		crystalArrow_ = std::make_unique<CrystalArrow>();
		crystalArrow_->Initialize(modelArrow_, &camera_);
	}

	// ★ プレイヤーのモデルにもマップタイルなどと同じライティングを設定
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

	// ★ カメラの移動範囲の設定
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

	for (auto& line : blockDatas_) {
		for (auto* blockData : line) {
			if (!blockData || !blockData->transform) {
				continue;
			}
			blockData->transform->matWorld_ = MakeAffineMatrix(blockData->transform->scale_, blockData->transform->rotation_, blockData->transform->translation_);
			blockData->transform->TransferMatrix();
		}
	}
	for (WorldTransform* worldTransformExplanation : worldTransformExplanations_) {
		if (!worldTransformExplanation) {
			continue;
		}
		worldTransformExplanation->matWorld_ = MakeAffineMatrix(worldTransformExplanation->scale_, worldTransformExplanation->rotation_, worldTransformExplanation->translation_);
		worldTransformExplanation->TransferMatrix();
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
#ifdef _DEBUG
	Player::ApplyGlobalVariables();
	Enemy::ApplyGlobalVariables();
#endif

	// ★ 追加: ポーズクラスの更新（ESC押下判定と看板の降り/上がりアニメーション）
	if (pause_) {
		pause_->Update();

		// ★ ポーズメニュー項目確定時の画面遷移処理
		if (pause_->IsMenuSelected()) {
			Pause::MenuType selected = pause_->GetSelectedMenu();
			pause_->ResetMenuSelection();
			pause_->Close(); // 看板を閉じる

			// ★ 修正: 選択項目を保持してフェードアウトを開始
			pendingPauseMenu_ = selected;
			phase_ = Phase::kFadeOut;
			if (fade_) {
				fade_->Start(Fade::Status::FadeOut, 0.8f);
			}
		}
	}

	// ★ 追加: ポーズ中（看板展開中・完全停止中）はゲーム内の全挙動を一時停止（フェードアウト中は除く）
	if (pause_ && pause_->IsPaused() && phase_ != Phase::kFadeOut) {
		return;
	}

	// ★ 追加: ポーズ中（看板展開中・完全停止中）はゲーム内の全挙動を一時停止
	if (pause_ && pause_->IsPaused()) {
		return;
	}

	// ★ LightManager の更新（ライトグループへの反映は、フレームの最後の TransferBuffer で 1 回だけ行う）
	if (lightManager_) {
		lightManager_->Update(false);
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

	bool isCameraPerformance = cameraController_ && cameraController_->IsCameraPerformance();

	// ★ 修正: フォント演出専用クラスの更新（カメラ演出中フラグを渡す）
	if (comboFont_) {
		comboFont_->Update(isCameraPerformance);
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

	// ★ 追加: 矢印の更新（全てのフェーズで、更新後の位置を反映する）
	UpdateCrystalArrow();

#ifdef _DEBUG
	debugCamera_->Update();
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

	// ステージタイルの雲のワールド行列更新
	for (auto* cloudData : cloudDatas_) {
		if (!cloudData || !cloudData->transform) {
			continue;
		}
		if (std::fabs(cloudData->transform->translation_.x - camera_.translation_.x) > kStageDrawRangeX) {
			continue;
		}
		Matrix4x4 affineMatrix = MakeAffineMatrix(cloudData->transform->scale_, cloudData->transform->rotation_, cloudData->transform->translation_);
		cloudData->transform->matWorld_ = affineMatrix;
		cloudData->transform->TransferMatrix();
	}

	// ★ 光の演出（点灯・夜・追従スポットライト）を更新してからライトを転送する
	if (lightManager_) {
		UpdateLightEffects();
		lightManager_->TransferBuffer();
	}
}

// ★ 追加: 攻撃できるクリスタルを指す矢印の更新
void GameScene::UpdateCrystalArrow() {
	const bool hasPlayer = (player_ != nullptr && !player_->IsDead());
	const Vector3 playerPos = player_ ? player_->GetWorldTransform().translation_ : Vector3{0.0f, 0.0f, 0.0f};

	FinalBoss* finalBoss = nullptr;

	for (BaseEnemy* enemy : enemies_) {
		if (!enemy || enemy->IsDead()) {
			continue;
		}

		if (FinalBoss* fBoss = dynamic_cast<FinalBoss*>(enemy)) {
			finalBoss = fBoss;
		}
	}

	// ★ ラスボスの流星群が終わったあと、ステージに落ちてきた「攻撃できるクリスタル」（地面に着地して、まだ飛んでいないもの）
	const bool crystalActive = finalBoss && !finalBoss->IsDefeated() && finalBoss->IsCrystal22LightActive();
	const Vector3 crystalPos = finalBoss ? finalBoss->GetCrystal22Position() : Vector3{0.0f, 0.0f, 0.0f};

	// ★ 矢印: プレイヤーの体の中心から、攻撃できるクリスタルを指す
	if (crystalArrow_) {
		crystalArrow_->Update(crystalActive && hasPlayer, {playerPos.x, playerPos.y + 1.0f, playerPos.z}, crystalPos);
	}
}

// ★ 光の演出およびBGMの更新（中ボス・ラスボスの進行状態に応じたBGM切り替え）
void GameScene::UpdateLightEffects() {
	if (!lightManager_) {
		return;
	}

	FinalBoss* finalBoss = nullptr;
	BossEnemy* bossEnemy = nullptr; // ★ 追加: 中ボスの取得用ポインタ

	for (BaseEnemy* enemy : enemies_) {
		if (FinalBoss* fBoss = dynamic_cast<FinalBoss*>(enemy)) { // ★ 修正: 変数名を fBoss に変更
			finalBoss = fBoss;
		} else if (BossEnemy* bEnemy = dynamic_cast<BossEnemy*>(enemy)) { // ★ 修正: 変数名を bEnemy に変更
			bossEnemy = bEnemy;                                           // ★ 追加: 中ボスを保持
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

	// ★ 追加: 中ボス・ラスボスの進行状態に応じたBGMの切り替え制御
	if (finalBoss) {
		if (finalBoss->IsDefeated()) {
			// ★ ラスボス撃破（死亡）時: ステージ本来のBGMに戻す
			AudioManager::GetInstance()->PlayBGM(BGMType::kGame);
		} else if (finalBoss->GetState() != FinalBoss::State::kStandby) {
			if (finalBoss->IsAwakened()) {
				// ★ 覚醒後: FinalBoss2.mp3 を再生
				AudioManager::GetInstance()->PlayBGM(BGMType::kFinalBoss2);
			} else {
				// ★ 出現時: FinalBoss.mp3 を再生
				AudioManager::GetInstance()->PlayBGM(BGMType::kFinalBoss);
			}
		}
	} else if (bossEnemy) {
		if (bossEnemy->IsDefeated() || bossEnemy->IsDead()) {
			// ★ 追加: 中ボス撃破（死亡）時: ステージ本来のBGMに戻す
			AudioManager::GetInstance()->PlayBGM(BGMType::kGame);
		} else {
			// ★ 追加: 中ボス出現・戦闘時: BossEnemy.mp3 を再生
			AudioManager::GetInstance()->PlayBGM(BGMType::kBossEnemy);
		}
	}

	float night = 0.0f;
	bool isFrenzy = false;
	bool isAwakened = false;
	if (finalBoss) {
		if (finalBoss->IsDefeated() && !finalBoss->IsExecuting()) {
			night = 0.0f;
		} else {
			night = finalBoss->GetSpawnProgress();
			isFrenzy = (finalBoss->GetState() == FinalBoss::State::kFrenzyInit || finalBoss->GetState() == FinalBoss::State::kFrenzy || finalBoss->IsExecuting());
			isAwakened = finalBoss->IsAwakened(); // ★ 覚醒状態を取得
		}
	}
	lightManager_->SetNightFactor(night);
	lightManager_->SetBossFrenzy(isFrenzy);
	lightManager_->SetAwakened(isAwakened);                                                              // ★ 終末ライティング設定をライトマネージャへ伝達（なめらかに切り替わる）
	lightManager_->SetAwakenRoarGlow((finalBoss && isAwakened) ? finalBoss->GetAwakenRoarGlow() : 0.0f); // ★ 覚醒の咆哮の余韻

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

		// ★ 終末の裁きの光の柱が着弾している間だけ、赤いポイントライトを灯す（流星群の点光源を使い回すため、必ず流星群の設定のあとに呼ぶ）
		Vector3 doomLightPos{};
		if (finalBoss->GetDoomLightPosition(doomLightPos)) {
			lightManager_->SetDoomPointLight(finalBoss->GetDoomStrikeIntensity(), doomLightPos);
		}
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

	executionPushDirX_ = (playerPos.x < bossPos.x) ? 1.0f : -1.0f;
	const float kPlayerOffsetX = 3.2f;
	Vector3 executionPos = {bossPos.x - executionPushDirX_ * kPlayerOffsetX, bossPos.y, bossPos.z};
	player_->BeginExecution(executionPos, executionPushDirX_ > 0.0f ? LRDirection::kRight : LRDirection::kLeft);

	boss->BeginExecution();
	cameraController_->StartExecution(bossPos);
}

// ★ 処刑演出の更新
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

	bool isShowExecutionUI = (executionPhase_ == ExecutionPhase::kHammering || executionPhase_ == ExecutionPhase::kFinaleCharge || executionPhase_ == ExecutionPhase::kFinaleSwing);
	if (comboFont_) {
		comboFont_->SetExecutionUIVisible(isShowExecutionUI);
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

		if (comboFont_) {
			comboFont_->AddCombo();
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

// ★ ラスボスの処刑演出の開始
void GameScene::StartFinalExecution(FinalBoss* boss) {
	executionFinalBoss_ = boss;
	finalExecutionPhase_ = FinalExecutionPhase::kPullBack;
	finalExecutionTimer_ = 0.0f;
	finalExecutionIdleTimer_ = 0.0f;
	finalExecutionCharge_ = 0.0f;
	finalExecutionRumbleTimer_ = 0.0f;

	const Vector3 bossPos = boss->GetWorldTransform().translation_;
	const Vector3 playerPos = player_->GetWorldTransform().translation_;

	finalExecutionPushDirX_ = (bossPos.x >= playerPos.x) ? 1.0f : -1.0f;
	player_->BeginExecution(playerPos, finalExecutionPushDirX_ > 0.0f ? LRDirection::kRight : LRDirection::kLeft);

	boss->BeginExecution();

	const Vector3 focus = {(bossPos.x + playerPos.x) * 0.5f, (bossPos.y + playerPos.y) * 0.5f, 0.0f};
	cameraController_->StartExecution(focus);
	cameraController_->SetExecutionZoom(kFinalExecutionZoomFar);
}

// ★ ラスボスの処刑演出の更新
void GameScene::UpdateFinalExecution() {
	const float dt = 1.0f / 60.0f;
	finalExecutionTimer_ += dt;

	skydome->Update();

	if (player_) {
		player_->Update();
	}

	UpdateClouds();

	if (executionFinalBoss_) {
		executionFinalBoss_->Update();
	}

	if (comboFont_) {
		comboFont_->SetExecutionUIVisible(finalExecutionPhase_ == FinalExecutionPhase::kCharge);
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

	if (executionFinalBoss_ && player_ && cameraController_) {
		const Vector3 bossPos = executionFinalBoss_->GetWorldTransform().translation_;
		const Vector3 playerPos = player_->GetWorldTransform().translation_;
		cameraController_->SetExecutionFocus({(bossPos.x + playerPos.x) * 0.5f, (bossPos.y + playerPos.y) * 0.5f, 0.0f});
	}

	const float kPullBackTime = 1.2f;
	const float kIdleLimit = 2.5f;
	const float kShatterTime = 2.2f;
	const float kRumbleInterval = 0.25f;

	switch (finalExecutionPhase_) {
	case FinalExecutionPhase::kPullBack:
		if (finalExecutionTimer_ >= kPullBackTime) {
			finalExecutionPhase_ = FinalExecutionPhase::kCharge;
			finalExecutionTimer_ = 0.0f;
			finalExecutionIdleTimer_ = 0.0f;
			finalExecutionRumbleTimer_ = 0.0f;
		}
		break;

	case FinalExecutionPhase::kCharge: {
		const bool isHolding = KamataEngine::Input::GetInstance()->PushKey(DIK_E);
		if (isHolding) {
			finalExecutionCharge_ += dt / kFinalExecutionChargeTime;
			finalExecutionIdleTimer_ = 0.0f;
		} else {
			finalExecutionIdleTimer_ += dt;
			if (finalExecutionIdleTimer_ >= kIdleLimit) {
				finalExecutionCharge_ += dt / (kFinalExecutionChargeTime * 2.0f);
			} else {
				finalExecutionCharge_ -= dt * 0.35f;
			}
		}
		finalExecutionCharge_ = std::clamp(finalExecutionCharge_, 0.0f, 1.0f);

		if (executionFinalBoss_) {
			executionFinalBoss_->SetExecutionCharge(finalExecutionCharge_);
		}

		cameraController_->SetExecutionZoom(kFinalExecutionZoomFar + (kFinalExecutionZoomNear - kFinalExecutionZoomFar) * finalExecutionCharge_);
		finalExecutionRumbleTimer_ += dt;
		if (finalExecutionRumbleTimer_ >= kRumbleInterval && finalExecutionCharge_ > 0.1f) {
			finalExecutionRumbleTimer_ = 0.0f;
			cameraController_->ExecutionHit(0.2f + 0.8f * finalExecutionCharge_);
		}

		if (finalExecutionCharge_ >= 1.0f) {
			finalExecutionPhase_ = FinalExecutionPhase::kStrike;
			finalExecutionTimer_ = 0.0f;
			player_->StartExecutionSwing(4);
		}
		break;
	}

	case FinalExecutionPhase::kStrike: {
		const bool isHit = player_->ConsumeExecutionHit();
		if (isHit || finalExecutionTimer_ >= 1.5f) {
			if (comboFont_) {
				comboFont_->AddCombo();
			}

			const Vector3 effectBase = executionFinalBoss_->GetWorldTransform().translation_;
			for (int i = 0; i < 4; ++i) {
				HitEffect* bigEffect = new HitEffect();
				Vector3 effectPos = effectBase;
				effectPos.x += (static_cast<float>(i) - 1.5f) * 1.2f;
				effectPos.y += (i % 2 == 0) ? 0.8f : -0.4f;
				bigEffect->Initialize(effectPos);
				effects_.push_back(bigEffect);
			}

			AudioManager::GetInstance()->PlaySE(SEType::kHummer);
			executionFinalBoss_->ExecutionStrike(finalExecutionPushDirX_);
			cameraController_->ExecutionHit(3.5f);

			finalExecutionPhase_ = FinalExecutionPhase::kShatter;
			finalExecutionTimer_ = 0.0f;
		}
		break;
	}

	case FinalExecutionPhase::kShatter: {
		const float t = std::clamp(finalExecutionTimer_ / 1.8f, 0.0f, 1.0f);
		cameraController_->SetExecutionZoom(kFinalExecutionZoomNear + (kFinalExecutionZoomShatter - kFinalExecutionZoomNear) * t);

		if (static_cast<int>(finalExecutionTimer_ / 0.5f) != static_cast<int>((finalExecutionTimer_ - dt) / 0.5f) && finalExecutionTimer_ < 1.2f) {
			cameraController_->ExecutionHit(1.2f);
		}

		if (finalExecutionTimer_ >= kShatterTime) {
			if (executionFinalBoss_) {
				executionFinalBoss_->EndExecution();
				executionFinalBoss_ = nullptr;
			}
			player_->EndExecution();
			cameraController_->EndExecution();
			if (comboFont_) {
				comboFont_->SetExecutionUIVisible(false);
			}
			finalExecutionPhase_ = FinalExecutionPhase::kNone;
			isFinalExecutionDone_ = true;
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
			// ★ 修正: ポーズからの遷移要求に応じて各フラグを立てる
			if (pendingPauseMenu_ == Pause::MenuType::Return) {
				isSelectRequested_ = true;
			} else if (pendingPauseMenu_ == Pause::MenuType::Retry) {
				reloadRequested_ = true;
			} else if (pendingPauseMenu_ == Pause::MenuType::Title) {
				isTitleRequested_ = true;
			} else if (pendingPauseMenu_ == Pause::MenuType::Exit) {
				isExitRequested_ = true;
			} else {
				finished_ = true;
			}
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

	if (finalExecutionPhase_ != FinalExecutionPhase::kNone) {
		UpdateFinalExecution();
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

	bool isCameraPerformance = cameraController_ && cameraController_->IsCameraPerformance();

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

			bool isFinalBoss = (dynamic_cast<FinalBoss*>(enemy) != nullptr);
			if (!isCameraPerformance || isFinalBoss) {
				enemy->Update();
			}

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

	if (!isCameraPerformance && player_) {
		auto attackAABB = player_->GetAttackAABB();
		if (!attackAABB.has_value()) {
			crystal22AttackConsumed_ = false;
			bossHitConsumed_ = false;
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
					const bool crystalHit = crystalPos.x >= attack.min.x - 1.2f && crystalPos.x <= attack.max.x + 1.2f && crystalPos.y >= attack.min.y - 1.2f && crystalPos.y <= attack.max.y + 1.2f;
					if (finalBoss->IsCrystal22Landed() && crystalHit && !crystal22AttackConsumed_) {
						finalBoss->HitCrystal22();
						crystal22AttackConsumed_ = true;
						AudioManager::GetInstance()->PlaySE(SEType::kHummer);

						if (comboFont_) {
							comboFont_->AddCombo();
						}
					}
				}

				if (enemy && !enemy->IsDead() && IsCollision(attackAABB.value(), enemy->GetAABB())) {
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

								if (comboFont_) {
									comboFont_->AddCombo();
								}

								HitEffect* newEffect = new HitEffect();
								newEffect->Initialize(hitPos);
								effects_.push_back(newEffect);

								if (cameraController_) {
									cameraController_->CombatHit(hitPos, isFinisher ? 1.6f : 1.0f);
								}
							}
						}
						continue;
					}

					BossEnemy* boss = dynamic_cast<BossEnemy*>(enemy);
					FinalBoss* finalBoss = dynamic_cast<FinalBoss*>(enemy);

					if (boss && bossHitConsumed_) {
						continue;
					}

					if (finalBoss && finalBoss->IsBodyHitConsumed()) {
						continue;
					}

					bool isHitSuccess = true;
					if (boss) {
						if (boss->IsAttacking() || boss->GetHp() <= 0) {
							isHitSuccess = false;
						}
					}

					if (isHitSuccess) {
						if (player_->GetBehavior() == Behavior::kHammerSkill) {
							AudioManager::GetInstance()->PlaySE(SEType::kHummer);
						} else if (player_->GetBehavior() == Behavior::kAttack) {
							AudioManager::GetInstance()->PlaySE(SEType::kDash);
						}

						if (comboFont_) {
							comboFont_->AddCombo();
						}

						if (boss) {
							bossHitConsumed_ = true;
						}
					}

					enemy->OnCollision(player_.get());

					if (dynamic_cast<Enemy*>(enemy)) {
						HitEffect* newEffect = new HitEffect();
						newEffect->Initialize(enemy->GetWorldTransform().translation_);
						effects_.push_back(newEffect);
						enemy->OnDead();
					} else if (boss || finalBoss) {
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

	if (finalExecutionPhase_ == FinalExecutionPhase::kNone && !isFinalExecutionDone_ && player_ && !player_->IsDead() && cameraController_) {
		for (BaseEnemy* enemy : enemies_) {
			FinalBoss* finalBoss = dynamic_cast<FinalBoss*>(enemy);
			if (finalBoss && finalBoss->IsDefeated() && !finalBoss->IsDefeatAnimationFinished() && !finalBoss->IsDead()) {
				StartFinalExecution(finalBoss);
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

	if (!isCameraPerformance && player_) {
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
	if (pause_) {
		pause_->Update(); // ★ 追加: 看板の戻りアニメーション更新
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

	if (backgroundBlocks_) {
		backgroundBlocks_->Draw(camera_);
	}

	if (comboFont_) {
		Model::PreDraw();
		comboFont_->Draw();
		Model::PostDraw();
	}

	Model::PreDraw();
	{
		const int32_t minCol = static_cast<int32_t>(std::ceil(camera_.translation_.x - kStageDrawRangeX));
		const int32_t maxCol = static_cast<int32_t>(std::floor(camera_.translation_.x + kStageDrawRangeX));
		for (auto& line : blockDatas_) {
			const int32_t colStart = (std::max)(minCol, 0);
			const int32_t colEnd = (std::min)(maxCol, static_cast<int32_t>(line.size()) - 1);
			for (int32_t col = colStart; col <= colEnd; ++col) {
				BlockData* blockData = line[col];
				if (!blockData || !blockData->transform || !blockData->model) {
					continue;
				}
				if (std::fabs(blockData->transform->translation_.x - camera_.translation_.x) > kStageDrawRangeX) {
					continue;
				}
				blockData->model->Draw(*blockData->transform, camera_);
			}
		}
	}

	for (auto* cloudData : cloudDatas_) {
		if (!cloudData || !cloudData->transform || !cloudData->model) {
			continue;
		}
		if (std::fabs(cloudData->transform->translation_.x - camera_.translation_.x) > kStageDrawRangeX) {
			continue;
		}
		cloudData->model->Draw(*cloudData->transform, camera_);
	}

	for (WorldTransform* worldTransformExplanation : worldTransformExplanations_) {
		if (!worldTransformExplanation) {
			continue;
		}
		modelExplanation_->Draw(*worldTransformExplanation, camera_);
	}
	Model::PostDraw();

	if (comboFont_) {
		Model::PreDraw();
		comboFont_->DrawExecutionUI();
		Model::PostDraw();
	}

	if (goal_) {
		Model::PreDraw();
		goal_->Draw();
		Model::PostDraw();
	}

	if (!effects_.empty()) {
		Model::PreDraw();
		for (BaseEffect* effect : effects_) {
			if (effect) {
				effect->Draw();
			}
		}
		Model::PostDraw();
	}

	if (!items_.empty()) {
		Model::PreDraw();
		for (Item* item : items_) {
			if (item) {
				item->Draw();
			}
		}
		Model::PostDraw();
	}

	if (player_) {
		Model::PreDraw();
		player_->Draw();
		Model::PostDraw();
	}

	// ★ 追加: 攻撃できるクリスタルを指す矢印の描画（プレイヤーの周り）
	if (crystalArrow_) {
		Model::PreDraw();
		crystalArrow_->Draw();
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

	if (comboFont_) {
		Model::PreDraw();
		comboFont_->DrawPauseUI();
		Model::PostDraw();
	}

	// ★ ポーズ看板モデル（PictureFrame）の描画
	if (pause_) {
		Model::PreDraw();
		pause_->Draw();
		Model::PostDraw();
	}

	if (fade_) {
		Model::PreDraw();
		fade_->Draw();
		Model::PostDraw();
	}
}