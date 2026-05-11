#include <Windows.h>
#include"Kamataengine.h"
#include"GameScene.h"

using namespace KamataEngine;

// Windowsアプリでのエントリーポイント(main関数)
int WINAPI WinMain(_In_ HINSTANCE, _In_opt_ HINSTANCE, _In_ LPSTR, _In_ int) {
	
	//初期化
	Initialize(L"LE2C_12_ショウ_ズーウェン_AL3");//#include <base\WinApp.h>の中でサイズ変える

	//KamataEngine::WinApp::GetInstance()->CreateGameWindow(L"LE2C_12_ショウ_ズーウェン_AL3", WS_OVERLAPPEDWINDOW, 1920, 1080);

	DirectXCommon* dxCommon = DirectXCommon::GetInstance();

	GameScene* gameScene = new GameScene();
	gameScene->Initialize();

	//メインループ
	while (true) 
	{
		//エンジンの更新
		if (KamataEngine::Update()) 
		{
			break;
		}

		// ゲームシーンの更新
		gameScene->Update();

		// 描画前処理
		dxCommon->PreDraw();

		// 描画処理
		gameScene->Draw();

		// 描画後処理
		dxCommon->PostDraw();
	}

	//エンジンの終了処理
	Finalize();

	//開放処理
	delete gameScene;
	gameScene = nullptr;

	return 0;
}
