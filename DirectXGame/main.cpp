#include "GameScene.h"
#include "Kamataengine.h"
#include "TitleScene.h"
#include <Windows.h>

enum class Scene {

	kUnknown = 0,

	kTitle,
	kGame,
};

Scene scene = Scene::kUnknown;

void ChangeScene(Scene newScene) { scene = newScene; }

using namespace KamataEngine;

// Windowsアプリでのエントリーポイント(main関数)
int WINAPI WinMain(_In_ HINSTANCE, _In_opt_ HINSTANCE, _In_ LPSTR, _In_ int) {

	// 初期化
	Initialize(L"LE2C_12_ショウ_ズーウェン_AL3");
	DirectXCommon* dxCommon = DirectXCommon::GetInstance();

	GameScene* gameScene = nullptr;
	gameScene = new GameScene();
	gameScene->Initialize();

	TitleScene* titleScene = nullptr;

	scene = Scene::kTitle;
	titleScene = new TitleScene();
	titleScene->Initialize();

	// メインループ
	while (true) {
		// エンジンの更新
		if (KamataEngine::Update()) {
			break;
		}

		// シーンの切り替え
		switch (scene) {
		case Scene::kTitle:
			titleScene->Update();

			if (titleScene->IsFinished()) {
				// タイトルシーンが終了したらゲームシーンへ切り替え
				scene = Scene::kGame;
				// ゲームシーンを最初から遊べるように初期化
				gameScene->Initialize();
			}
			break;

		case Scene::kGame:
			gameScene->Update();

			if (gameScene->isFinished()) {
				// ゲームシーンが終了（デス演出が完了）したらタイトルシーンへ切り替え
				scene = Scene::kTitle;
				// タイトルシーンを再度遊べるように初期化
				titleScene->Initialize();
			}
			break;
		}

		// 描画処理
		dxCommon->PreDraw();

		switch (scene) {
		case Scene::kTitle:
			titleScene->Draw();
			break;

		case Scene::kGame:
			gameScene->Draw();
			break;
		}

		dxCommon->PostDraw();
	}

	//エンジンの終了処理
	Finalize();

	//開放処理
	
	delete titleScene;
	titleScene = nullptr;
	delete gameScene;
	gameScene = nullptr;

	return 0;
}
