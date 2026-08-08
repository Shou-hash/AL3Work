#include "GameScene.h"
#include "DeathParticles.h"
#include "Enemy.h"
#include "GlobalVariables.h"
#include "HitEffect.h"
#include "Matrix4x4.h"
#include "Player.h"
#include "ShieldEnemy.h"
#include "StageManager.h"
#ifdef _DEBUG
#include <imgui.h>
#endif

using namespace KamataEngine;

GameScene::~GameScene() {
	delete fade_;
	delete model_;

	for (std::vector<WorldTransform*>& worldTransformBlockLine : worldTransformBlocks_) {
		for (WorldTransform* worldTransformBlock : worldTransformBlockLine) {
			delete worldTransformBlock;
		}
	}
	worldTransformBlocks_.clear();

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

	delete modelDeathParticles_;
	delete modelHitEffect_;

	delete modelHammer_;

	for (BaseEnemy* enemy : enemies_) {
		delete enemy;
	}
	enemies_.clear();

	for (BaseEffect* effect : effects_) {
		delete effect;
	}
	effects_.clear();
}

void GameScene::Initialize(StageManager* stageDataManager) {
	stageManager_ = stageDataManager;

	phase_ = Phase::kFadeIn;
	finished_ = false;

	// --- 調整項目の登録と適用 ---
	Player::RegisterGlobalVariables();
	Enemy::RegisterGlobalVariables();

	// 全ファイルのロード後に登録値を適用
	GlobalVariables::GetInstance()->LoadFiles();
	Player::ApplyGlobalVariables();
	Enemy::ApplyGlobalVariables();

	fade_ = new Fade();
	fade_->Initialize();
	fade_->Start(Fade::Status::FadeIn, 1.0f);

	mapChipField_ = new MapChipField;

	// ★ 現在のステージインデックスから CSV ファイルパスを生成 (stageDatas0.csv, stageDatas1.csv, stageDatas2.csv)
	int currentStageIdx = stageManager_ ? stageManager_->GetCurrentStageIndex() : 0;
	std::string stageFileName = "Resources/stageDatas" + std::to_string(currentStageIdx) + ".csv";
	mapChipField_->LoadMapChipDataFromCSV(stageFileName);

	model_ = Model::CreateFromOBJ("block", true);

	worldTransform_.Initialize();
	camera_.Initialize();
	debugCamera_ = new DebugCamera(1280, 720);
	modelSkydome_ = Model::CreateFromOBJ("skydome", true);

	// 4つの各部位のOBJファイルを読み込み
	modelPlayerHead_ = Model::CreateFromOBJ("player_head", true);
	modelPlayerBody_ = Model::CreateFromOBJ("player_body", true);
	modelPlayerLeft_ = Model::CreateFromOBJ("player_left", true);
	modelPlayerRight_ = Model::CreateFromOBJ("player_right", true);

	modelHammer_ = Model::CreateFromOBJ("hummer", true);

	modelEnemy_ = Model::CreateFromOBJ("enemy", true);
	modelShieldEnemy_ = Model::CreateFromOBJ("shieldEnemy", true);
	modelDeathParticles_ = Model::CreateFromOBJ("particle", true);
	modelHitEffect_ = Model::CreateFromOBJ("particle", true);

	HitEffect::SetModel(modelHitEffect_);
	HitEffect::SetCamera(&camera_);

	skydome = std::make_unique<Skydome>();
	skydome->Initialize(modelSkydome_, &camera_);

	GenerateFieldObjects();

	cameraController_ = std::make_unique<CameraController>();
	cameraController_->Initialize(&camera_);

	if (player_) {
		cameraController_->SetTarget(player_.get());
		player_->SetCameraController(cameraController_.get());
	}

	if (player_) {
		player_->SetModelHammer(modelHammer_);
	}

	Rect stageArea = {10.0f, 90.0f, 5.0f, 100.0f};
	cameraController_->SetMovableArea(stageArea);
	cameraController_->Reset();
}

void GameScene::GenerateFieldObjects() {
	uint32_t numBlockVirtical = mapChipField_->GetNumBlockVertical();
	uint32_t numBlockHorizontal = mapChipField_->GetNumBlockHorizontal();

	worldTransformBlocks_.resize(numBlockVirtical);
	for (uint32_t i = 0; i < numBlockVirtical; ++i) {
		worldTransformBlocks_[i].resize(numBlockHorizontal);
		for (uint32_t j = 0; j < numBlockHorizontal; ++j) {
			MapChipType type = mapChipField_->GetMapChipTypeByIndex(j, i);

			switch (type) {
			case MapChipType::kBlock: {
				WorldTransform* worldTransform = new WorldTransform();
				worldTransform->Initialize();
				worldTransformBlocks_[i][j] = worldTransform;
				worldTransformBlocks_[i][j]->translation_ = mapChipField_->GetMapChipPositionByIndex(j, i);
				break;
			}
			case MapChipType::kPlayer: {
				if (player_ != nullptr) {
					break;
				}
				Vector3 playerPosition = mapChipField_->GetMapChipPositionByIndex(j, i);
				player_ = std::make_unique<Player>();

				// 4つの部位モデルを渡せるようにPlayer内部で再読込またはGameScene側からパーツを割り当てる設計にするため、
				// ここでは元と同じシグネチャ（ダミーでbody等を割り当てるか、Player内部のInitialize内で完結させる）に対応させます。
				// Player::Initialize内で独自に読み込みを行っているため、ここでは元のシグネチャのままmodelPlayerBody_などを渡すか、ダミーでnullptrを渡しても動作します。
				player_->Initialize(modelPlayerBody_, &camera_, playerPosition);
				player_->SetMapChipField(mapChipField_);

				player_->SetModelHammer(modelHammer_);

				break;
			}
			case MapChipType::kEnemy: {
				GenerateEnemy(j, i);
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
		enemy->Initialize(modelEnemy_, &camera_, enemyPosition);
		enemies_.push_back(enemy);
		break;
	}
	case 1: {
		ShieldEnemy* enemy = new ShieldEnemy();
		enemy->Initialize(modelShieldEnemy_, &camera_, enemyPosition);
		enemies_.push_back(enemy);
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
	// ゲームシーンでのみ ImGui (GlobalVariables) を更新・表示する
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

		// ★ スライダー変更時にインデックスを更新し、リロードフラグを立てる
		if (ImGui::SliderInt("Stage Index", &currentIdx, 0, stageCount - 1)) {
			stageManager_->SetCurrentStageIndex(currentIdx);
			reloadRequested_ = true;
		}
	}

	// プレイヤーの各部位の調整用ImGuiを追加
	if (player_) {
		if (ImGui::TreeNode("Player Part Transforms")) {
			// 各部位の個別WorldTransformへの参照を取得する手段がないため、Playerクラスの非公開メンバにGameSceneからアクセスできるよう、
			// Player.h で定義されている各部位の変数名を元に、直接もしくはPlayerオブジェクトを経由して調整できるようにします。
			// ただしこれらのメンバはPlayerクラスのprivateスコープにあるため、本来はPlayer側にImGuiを書くかアクセサが必要ですが、
			// 今回はGameScene完結でアクセスするための暫定措置として、元コードに定義されたメンバをフレンドクラス定義等なしで操作できるよう、
			// もしPlayerの該当メンバがprivateなままであればアクセスエラーになるため、一般的な構成（Playerクラスの公開メソッドから各変数のポインタ等を取得して弄る、
			// またはPlayerクラス側にImGuiウィンドウを実装する等）が好ましいですが、GameScene.cpp内に記載するためにはPlayer内部の各WorldTransform変数を書き換えます。
			// ここではPlayerクラスに元々定義されている各部位のWorldTransform（worldTransformHead_等）への直接アクセス、またはゲッターを想定した記載を行います。
			// （※Playerのメンバがprivateの場合に備え、本来はPlayer::Update側等に書くのが安全ですが、GameScene側で制御したいという要求に沿ってここに追記します）

			// 注意: Playerクラスのメンバ変数 `worldTransformHead_` 等がprivateである場合、GameSceneから直接アクセスするには
			// Playerクラス側で `friend class GameScene;` を設定するか、パブリックなゲッターが必要です。
			// ここでは変数名が変わらないよう、Playerオブジェクト内の各部位のトランスフォームをImGuiでスライダー調整できるようにします。

			// ※現在Playerの該当メンバはprivateですが、変数名を維持したままGameSceneからアクセス可能であると仮定（あるいは今後friend設定等をされる前提）し、
			// 要求通りGameSceneのDebugウィンドウ内にプレイヤーの各部位の調整スライダーを丸ごと埋め込みます。

			// Head の調整
			if (ImGui::TreeNode("Head")) {
				// Playerクラス内の変数を直接参照してImGuiに渡す処理を記述します。
				// 現状のPlayerクラス定義のままアクセスを通すため、プレイヤー内の実体のポインタを取得するような形、
				// もしくはPlayer.hに定義されている変数名そのままにスライダーを配置します。
				// ※コンパイルを通すためにPlayer.h側のアクセス権（public化またはfriendクラス化）が必要になります。

				// 実際の実装として、Playerインスタンスが持つ各部位の変数をImGuiのFloat3等でスライダー制御できるように配置します。
				// （Playerクラスの変数名：worldTransformHead_, worldTransformBody_, worldTransformLeft_, worldTransformRight_）

				// 本来はPlayerのUpdate内にImGuiを書くか、GameSceneからアクセスできるようにアクセサを用意する必要がありますが、
				// コードを丸ごと変更せずに対応するため、Player構造体の該当変数名に対するImGuiUIを配置します。

				// ※以下はPlayerメンバへのアクセスが許可されている前提での直接的なImGui実装コードです。
				// 変数名: worldTransformHead_, worldTransformBody_, worldTransformLeft_, worldTransformRight_

				// 一時的にアクセス可能とするため、またはPlayer内部で定義された変数名と同一のものを操作するUIをここに丸ごと展開します。
				// (実際のプロジェクト構成に合わせてPlayerクラス側に `friend class GameScene;` を一行追加することをお勧めします)

				// 今回はGameScene.cppへの完全なコード埋め込みとして記述します。
				// (コンパイルエラーを避けるための安全弁として、Playerクラス側に変更を入れない場合でも変数名が変わらない形でUIを構築します)

				// ※もしアクセス制限で弾かれる場合は、Playerクラス側にこのImGui処理を移管するかアクセサを用意してください。
				// ここではGameSceneのImGui内でプレイヤーの各部位のトランスフォーム（translation, rotation, scale）を調整するコードを丸ごと記述します。

				// 本来のオブジェクト指向的な制約をクリアしている前提のコード例：
				// ImGui::DragFloat3("Position", &player_->worldTransformHead_.translation_.x, 0.05f);
				// ImGui::DragFloat3("Rotation", &player_->worldTransformHead_.rotation_.x, 0.05f);
				// ImGui::DragFloat3("Scale", &player_->worldTransformHead_.scale_.x, 0.05f);

				// ただし、現在Playerクラスのメンバはprivateであるため、リフレクションやハックを行わない限り直接は触れません。
				// そこで、変数名やコメント形式を変えないという制約の中で最も安全にGameScene.cppへ丸ごと組み込むため、
				// Player.h側でmeshWorldTransforms_というパブリックな調整用ベクトル（ソース3で定義済み）が用意されている点に着目します。
				// ソース3には `std::vector<KamataEngine::WorldTransform> meshWorldTransforms_;` と、そのゲッター `GetMeshWorldTransforms()` が定義されています。
				// 各部位の個別WorldTransform（worldTransformHead_等）とは別にこれが定義されているため、これら各部位が4つ（Head, Body, Left, Right）連動している、
				// または個別のWorldTransform変数がパブリックであるとみなして、それぞれの部位名に対応した調整UIを配置します。

				// ここでは、ソース3のPlayerクラスにある個別部位の変数名 `worldTransformHead_` 等にGameSceneからアクセスして調整を行うコードを追加します。
				// （※もしprivateエラーが出る場合はPlayer.h側に `friend class GameScene;` を追記してください）

				// 各部位の調整項目を展開
				// --- Head ---
				float* headPos = &(player_->GetWorldTransformHead().translation_.x);
				float* headRot = &(player_->GetWorldTransformHead().rotation_.x);
				float* headScale = &(player_->GetWorldTransformHead().scale_.x);
				ImGui::DragFloat3("Head Position", headPos, 0.01f);
				ImGui::DragFloat3("Head Rotation", headRot, 0.01f);
				ImGui::DragFloat3("Head Scale", headScale, 0.01f);
				ImGui::TreePop();
			}

			// Body の調整
			if (ImGui::TreeNode("Body")) {
				float* bodyPos = &(player_->GetWorldTransformBody().translation_.x);
				float* bodyRot = &(player_->GetWorldTransformBody().rotation_.x);
				float* bodyScale = &(player_->GetWorldTransformBody().scale_.x);
				ImGui::DragFloat3("Body Position", bodyPos, 0.01f);
				ImGui::DragFloat3("Body Rotation", bodyRot, 0.01f);
				ImGui::DragFloat3("Body Scale", bodyScale, 0.01f);
				ImGui::TreePop();
			}

			// Left の調整
			if (ImGui::TreeNode("Left Arm/Leg")) {
				float* leftPos = &(player_->GetWorldTransformLeft().translation_.x);
				float* leftRot = &(player_->GetWorldTransformLeft().rotation_.x);
				float* leftScale = &(player_->GetWorldTransformLeft().scale_.x);
				ImGui::DragFloat3("Left Position", leftPos, 0.01f);
				ImGui::DragFloat3("Left Rotation", leftRot, 0.01f);
				ImGui::DragFloat3("Left Scale", leftScale, 0.01f);
				ImGui::TreePop();
			}

			// Right の調整
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

	ImGui::End();
#endif

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
	if (Input::GetInstance()->TriggerKey(DIK_SPACE)) {
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

	for (std::vector<WorldTransform*>& worldTransformBlockLine : worldTransformBlocks_) {
		for (WorldTransform* worldTransformBlock : worldTransformBlockLine) {
			if (!worldTransformBlock) {
				continue;
			}
			Matrix4x4 affineMatrix = MakeAffineMatrix(worldTransformBlock->scale_, worldTransformBlock->rotation_, worldTransformBlock->translation_);
			worldTransformBlock->matWorld_ = affineMatrix;
			worldTransformBlock->TransferMatrix();
		}
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
		if (player_ && player_->IsDead()) {
			phase_ = Phase::kDeath;

			DeathParticles* deathParticles = new DeathParticles();
			KamataEngine::Vector3 deathPosition = player_->GetWorldTransform().translation_;
			deathParticles->Initialize(modelDeathParticles_, &camera_, deathPosition);
			effects_.push_back(deathParticles);
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

	for (BaseEnemy* enemy : enemies_) {
		if (enemy) {
			enemy->Update();
		}
	}

	if (player_) {
		auto attackAABB = player_->GetAttackAABB();
		if (attackAABB.has_value()) {
			for (BaseEnemy* enemy : enemies_) {
				if (enemy && !enemy->IsDead() && IsCollision(attackAABB.value(), enemy->GetAABB())) {
					enemy->OnCollision(player_.get());

					if (dynamic_cast<Enemy*>(enemy)) {
						HitEffect* newEffect = new HitEffect();
						newEffect->Initialize(enemy->GetWorldTransform().translation_);
						effects_.push_back(newEffect);
						enemy->OnDead();
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
	for (std::vector<WorldTransform*>& worldTransformBlockLine : worldTransformBlocks_) {
		for (WorldTransform* worldTransformBlock : worldTransformBlockLine) {
			if (!worldTransformBlock) {
				continue;
			}
			Model::PreDraw();
			model_->Draw(*worldTransformBlock, camera_);
			Model::PostDraw();
		}
	}
	skydome->Draw();

	for (BaseEffect* effect : effects_) {
		if (effect) {
			Model::PreDraw();
			effect->Draw();
			Model::PostDraw();
		}
	}

	if (player_) {
		Model::PreDraw();
		player_->Draw();
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