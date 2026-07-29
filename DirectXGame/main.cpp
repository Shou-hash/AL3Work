#include "GameScene.h"
#include "GlobalVariables.h" // ★ GlobalVariablesのヘッダーを追加
#include "Kamataengine.h"
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
	kGame,
};

// グローバル変数（または静的変数）の管理
Scene scene = Scene::kUnknown;
TitleScene* titleScene = nullptr;
GameScene* gameScene = nullptr;
StageManager* stageManager = nullptr;

// シーン切り替え関数
void ChangeScene(Scene newScene) { scene = newScene; }

void LoadDebugSettings() {
	std::ifstream file("DebugSettings.ini");
	if (!file.is_open()) {
		return;
	}

	std::string line;
	while (std::getline(file, line)) {
		// 空行やコメント行のスキップ
		if (line.empty() || line[0] == '#' || line[0] == ';') {
			continue;
		}

		// '=' をスペースに置換して stream で読みやすくする
		std::replace(line.begin(), line.end(), '=', ' ');

		std::stringstream lineStream(line);
		std::string key, value;
		if (lineStream >> key >> value) {
			// ステージ設定
			if (key == "InitialStage") {
				stageManager->SetCurrentStageIndexByName(value);
			}
		}
	}
}

// シーンごとの更新処理と遷移管理を行う関数
void UpdateScene() {

	// ★ グローバル変数の更新（ImGui描画など）の呼び出しを追加
	GlobalVariables::GetInstance()->Update();

	switch (scene) {
	case Scene::kTitle:
		titleScene->Update();

		if (titleScene->IsFinished()) {
			// タイトルシーンが終了したらゲームシーンへ切り替え
			ChangeScene(Scene::kGame);
			// ゲームシーンを最初から遊べるように初期化
			gameScene->Initialize(stageManager); // ★ 引数を追加
		}
		break;

	case Scene::kGame:
		gameScene->Update();

		if (gameScene->isFinished()) {
			// ゲームシーンが終了（デス演出が完了）したらタイトルシーンへ切り替え
			ChangeScene(Scene::kTitle);
			titleScene->Initialize();

			// ゲームシーンもインスタンスごとリロード（解放して再生成）しておく
			delete gameScene;
			gameScene = nullptr;
			gameScene = new GameScene();
			// 次回ゲーム開始時（タイトルから遷移時）に Initialize() される
		}
		// リロード要求（ボタン押し）があった場合の処理
		else if (gameScene->IsReloadRequested()) {
			// シーンリロード
			delete gameScene;
			gameScene = nullptr;
			gameScene = new GameScene();
			gameScene->Initialize(stageManager); // ★ 引数を追加
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

	case Scene::kGame:
		gameScene->Draw();
		break;
	}
}

using namespace KamataEngine;

// Windowsアプリでのエントリーポイント(main関数)
int WINAPI WinMain(_In_ HINSTANCE, _In_opt_ HINSTANCE, _In_ LPSTR, _In_ int) {

	// 初期化
	Initialize(L"LE2C_12_ショウ_ズーウェン_AL3");
	DirectXCommon* dxCommon = DirectXCommon::GetInstance();

	// ★1. まず最初に StageManager を生成＆CSV読み込みする
	stageManager = new StageManager();
	stageManager->LoadStageDatas();

	// ★2. シーンのインスタンス生成（この時点では Initialize は Title だけでもOKですが、生成はここで行う）
	titleScene = new TitleScene();
	titleScene->Initialize();

#ifdef _DEBUG
	// デバッグ設定ファイル読み込み
	LoadDebugSettings();

	// ゲームシーンの初期化
	scene = Scene::kGame;
	gameScene = new GameScene();
	gameScene->Initialize(stageManager);
#else
	// リリース時の初期化
	titleScene = new TitleScene();
	titleScene->Initialize();
	gameScene = new GameScene();
	ChangeScene(Scene::kTitle);
#endif

#ifdef _DEBUG
	ImGuiManager* imguiManager = ImGuiManager::GetInstance();
#endif

	// 最初のシーンを設定
	ChangeScene(Scene::kTitle);

	// メインループ
	while (true) {
		if (KamataEngine::Update()) {
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

	// エンジンの終了処理
	Finalize();

	// 解放処理
	delete titleScene;
	titleScene = nullptr;
	delete gameScene;
	gameScene = nullptr;
	delete stageManager;
	stageManager = nullptr;

	return 0;
}