#include "GameScene.h"
#include "Kamataengine.h"
#include <Windows.h>

using namespace KamataEngine;

// Windowsアプリでのエントリーポイント(main関数)
int WINAPI WinMain(_In_ HINSTANCE, _In_opt_ HINSTANCE, _In_ LPSTR, _In_ int) {

	// 初期化
	Initialize(L"LE2C_12_ショウ_ズーウェン_AL3");
	DirectXCommon* dxCommon = DirectXCommon::GetInstance();

	ImGuiManager* imguiManager = ImGuiManager::GetInstance();

	GameScene* gameScene = new GameScene();
	gameScene->Initialize();

	// メインループ
	while (true) 
	{

		// エンジンの更新
		if (KamataEngine::Update()) 
		{
			break;
		}
		// ImGui受付開始
		imguiManager->Begin();

		// ゲームシーンの更新
		gameScene->Update();

		// ImGui受付終了
		ImGuiManager::GetInstance()->End();

		// 描画前処理
		dxCommon->PreDraw();

		// 描画処理
		gameScene->Draw();

		// 軸表示の描画
		AxisIndicator::GetInstance()->Draw();

		// ImGuiの描画
		ImGuiManager::GetInstance()->Draw();

		// 描画後処理
		dxCommon->PostDraw();
	}

	// 開放処理
	delete gameScene;
	gameScene = nullptr;

	// エンジンの終了処理
	Finalize();

	return 0;
}