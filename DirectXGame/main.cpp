#include "AudioManager.h"
#include "EndScene.h"
#include "GameScene.h"
#include "GlobalVariables.h"
#include "Kamataengine.h"
#include "SelectScene.h"
#include "StageManager.h"
#include "TitleScene.h"
#include <Windows.h>
#include <algorithm>
#include <fstream>
#ifdef _DEBUG
#include <imgui.h>
#endif
#include <sstream>

enum class Scene {
	kUnknown = 0,
	kTitle,
	kSelect,
	kEnd,
	kGame,
};

// グローバル変数（または静的変数）の管理
Scene scene = Scene::kUnknown;
TitleScene* titleScene = nullptr;
SelectScene* selectScene = nullptr;
EndScene* endScene = nullptr;
GameScene* gameScene = nullptr;
StageManager* stageManager = nullptr;
bool isExitRequested = false;

// シーン切り替え関数
void ChangeScene(Scene newScene) { scene = newScene; }

void LoadDebugSettings() {
	std::ifstream file("DebugSettings.ini");
	if (!file.is_open()) {
		return;
	}

	std::string line;
	while (std::getline(file, line)) {
		if (line.empty() || line[0] == '#' || line[0] == ';') {
			continue;
		}

		std::replace(line.begin(), line.end(), '=', ' ');

		std::stringstream lineStream(line);
		std::string key, value;
		if (lineStream >> key >> value) {
			if (key == "InitialStage") {
				stageManager->SetCurrentStageIndexByName(value);
			}
		}
	}
}

// シーンごとの更新処理と遷移管理を行う関数
void UpdateScene() {
	GlobalVariables::GetInstance()->Update();

	switch (scene) {
	case Scene::kTitle:
		titleScene->Update();

		// ★ 軽量化: 待ち時間のあいだに、ゲームで使うモデルを 1 フレームにつき 1 個ずつ読み込んでおく
		GameScene::PreloadModelsStep();

		if (titleScene->IsFinished()) {
			ChangeScene(Scene::kSelect);
			selectScene->Initialize(stageManager);
		}
		break;

	case Scene::kSelect:
		selectScene->Update();

		// ★ 軽量化: セレクト画面でも続きを読み込む
		GameScene::PreloadModelsStep();

		// ★ 追加: ポーズからの画面遷移要求の処理
		if (selectScene->IsTitleRequested()) {
			ChangeScene(Scene::kTitle);
			titleScene->Initialize();
		} else if (selectScene->IsExitRequested()) {
			isExitRequested = true;
		} else if (selectScene->IsFinished()) {
			ChangeScene(Scene::kGame);
			gameScene->Initialize(stageManager);
		}
		break;

	case Scene::kGame:
		gameScene->Update();

		// ★ 追加: ポーズからの画面遷移要求の処理
		if (gameScene->IsTitleRequested()) {
			ChangeScene(Scene::kTitle);
			titleScene->Initialize();

			delete gameScene;
			gameScene = nullptr;
			gameScene = new GameScene();
		} else if (gameScene->IsSelectRequested()) {
			ChangeScene(Scene::kSelect);
			selectScene->Initialize(stageManager);

			delete gameScene;
			gameScene = nullptr;
			gameScene = new GameScene();
		} else if (gameScene->IsExitRequested()) {
			isExitRequested = true;
		} else if (gameScene->isFinished()) {
			ChangeScene(Scene::kEnd);
			endScene->Initialize(stageManager);
		} else if (gameScene->IsReloadRequested()) {
			delete gameScene;
			gameScene = nullptr;
			gameScene = new GameScene();
			gameScene->Initialize(stageManager);
		}
		break;

	case Scene::kEnd:
		endScene->Update();

		if (endScene->IsFinished()) {
			EndScene::MenuType selected = endScene->GetSelectedMenu();
			if (selected == EndScene::MenuType::Return) {
				ChangeScene(Scene::kSelect);
				selectScene->Initialize(stageManager);

				delete gameScene;
				gameScene = nullptr;
				gameScene = new GameScene();
			} else if (selected == EndScene::MenuType::Retry) {
				delete gameScene;
				gameScene = nullptr;
				gameScene = new GameScene();
				ChangeScene(Scene::kGame);
				gameScene->Initialize(stageManager);
			} else if (selected == EndScene::MenuType::Title) {
				ChangeScene(Scene::kTitle);
				titleScene->Initialize();

				delete gameScene;
				gameScene = nullptr;
				gameScene = new GameScene();
			} else if (selected == EndScene::MenuType::Exit) {
				isExitRequested = true;
			}
		}
		break;
	}
}

// 描画処理をまとめた関数
void DrawScene() {
	switch (scene) {
	case Scene::kTitle:
		titleScene->Draw();
		break;

	case Scene::kSelect:
		selectScene->Draw();
		break;

	case Scene::kGame:
		gameScene->Draw();
		break;

	case Scene::kEnd:
		endScene->Draw();
		break;
	}
}

using namespace KamataEngine;

// Windowsアプリでのエントリーポイント(main関数)
int WINAPI WinMain(_In_ HINSTANCE, _In_opt_ HINSTANCE, _In_ LPSTR, _In_ int) {

	// 初期化
	Initialize(L"LE2C_12_ショウ_ズーウェン_ダイス・ルミナ統合の玉座");
	DirectXCommon* dxCommon = DirectXCommon::GetInstance();

	// ★ オーディオマネージャーの初期化（BGMの読み込み）
	AudioManager::GetInstance()->Initialize();

	// ★1. まず最初に StageManager を生成＆CSV読み込みする
	stageManager = new StageManager();
	stageManager->LoadStageDatas();

	// ★2. シーンのインスタンス生成
	titleScene = new TitleScene();
	selectScene = new SelectScene();
	endScene = new EndScene();
	gameScene = new GameScene();

#ifdef _DEBUG
	// デバッグ設定ファイル読み込み
	LoadDebugSettings();
#endif

#ifdef _DEBUG
	ImGuiManager* imguiManager = ImGuiManager::GetInstance();
#endif

	// 最初のシーン（タイトル）の初期化と設定
	ChangeScene(Scene::kTitle);
	titleScene->Initialize();

	// メインループ
	while (true) {
		if (KamataEngine::Update() || isExitRequested) {
			break;
		}

#ifdef _DEBUG
		imguiManager->Begin();
#endif
		UpdateScene();

#ifdef _DEBUG
		imguiManager->End();
#endif

		dxCommon->PreDraw();
		DrawScene();

#ifdef _DEBUG
		imguiManager->Draw();
#endif
		dxCommon->PostDraw();
	}

	// ★ オーディオマネージャーの終了処理
	AudioManager::GetInstance()->Finalize();

	// エンジンの終了処理
	Finalize();

	// 解放処理
	delete titleScene;
	titleScene = nullptr;
	delete selectScene;
	selectScene = nullptr;
	delete endScene;
	endScene = nullptr;
	delete gameScene;
	gameScene = nullptr;
	delete stageManager;
	stageManager = nullptr;

	return 0;
}