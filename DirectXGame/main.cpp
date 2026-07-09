#include "GameScene.h"
#include "Kamataengine.h"
#include "TitleScene.h"
#include <Windows.h>

enum class Scene {
	kUnknown = 0,
	kTitle,
	kGame,
};

// グローバル変数（または静的変数）の管理
Scene scene = Scene::kUnknown;
TitleScene* titleScene = nullptr;
GameScene* gameScene = nullptr;

// シーン切り替え関数
void ChangeScene(Scene newScene) { scene = newScene; }

// シーンごとの更新処理と遷移管理を行う関数
void UpdateScene() {
	switch (scene) {
	case Scene::kTitle:
		titleScene->Update();

		if (titleScene->IsFinished()) {
			// タイトルシーンが終了したらゲームシーンへ切り替え
			ChangeScene(Scene::kGame);
			// ゲームシーンを最初から遊べるように初期化
			gameScene->Initialize();
		}
		break;

	case Scene::kGame:
		gameScene->Update();

		if (gameScene->isFinished()) {
			// ゲームシーンが終了（デス演出が完了）したらタイトルシーンへ切り替え
			ChangeScene(Scene::kTitle);
			// タイトルシーンを再度遊べるように初期化
			titleScene->Initialize();
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

	// 各シーンのインスタンス生成と初期化
	gameScene = new GameScene();
	gameScene->Initialize();

	titleScene = new TitleScene();
	titleScene->Initialize();

	// 最初のシーンを設定
	ChangeScene(Scene::kTitle);

	// メインループ
	while (true) {
		// エンジンの更新
		if (KamataEngine::Update()) {
			break;
		}

		// シーンの更新・切り替え判定を関数化
		UpdateScene();

		// 描画処理
		dxCommon->PreDraw();

		// 描画処理を関数化
		DrawScene();

		dxCommon->PostDraw();
	}

	// エンジンの終了処理
	Finalize();

	// 開放処理
	delete titleScene;
	titleScene = nullptr;
	delete gameScene;
	gameScene = nullptr;

	return 0;
}