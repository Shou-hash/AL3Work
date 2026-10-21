#include "PlayerTutorial.h"
#include "LightManager.h"
#include "Matrix4x4.h"
#include <cmath>
#include <numbers>

using namespace KamataEngine;

PlayerTutorial::~PlayerTutorial() { delete modelArrow_; }

void PlayerTutorial::Initialize(int stageIndex, const Vector3& playerPos) {
	isStageZero_ = (stageIndex == 0);
	isActive_ = true;

	for (int i = 0; i < kNumArrows; ++i) {
		keyPressed_[i] = false;
	}

	// ステージ0以外では処理を行わない
	if (!isStageZero_) {
		return;
	}

	// 矢印モデルの読み込み ("Resources/Arrow")
	modelArrow_ = Model::CreateFromOBJ("Arrow", true);

	// Transform の初期化と回転角（Z軸 rotation）の設定
	for (int i = 0; i < kNumArrows; ++i) {
		arrowTransforms_[i].Initialize();
		arrowTransforms_[i].scale_ = {1.0f, 1.0f, 1.0f};
	}

	// ★ OBJファイル標準（右向き: Z=0）に対し、Z軸回転で4方向に対応（角度は維持）
	// 0: DOWN (下) -> Z軸 +90度 (+π/2)
	arrowTransforms_[0].rotation_ = {0.0f, 0.0f, std::numbers::pi_v<float> / 2.0f};
	// 1: UP (上) -> Z軸 -90度 (-π/2)
	arrowTransforms_[1].rotation_ = {0.0f, 0.0f, -std::numbers::pi_v<float> / 2.0f};
	// 2: RIGHT (右) -> Z軸 180度 (π)
	arrowTransforms_[2].rotation_ = {0.0f, 0.0f, std::numbers::pi_v<float>};
	// 3: LEFT (左) -> Z軸 0度
	arrowTransforms_[3].rotation_ = {0.0f, 0.0f, 0.0f};

	Update(playerPos);
}

void PlayerTutorial::Update(const Vector3& playerPos) {
	if (!isStageZero_ || !isActive_) {
		return;
	}

	// ★ 移動操作キー（UP / DOWN / LEFT / RIGHT）の押下判定（上下・左右を反転補正）
	Input* input = Input::GetInstance();
	if (input->PushKey(DIK_DOWN)) {
		keyPressed_[0] = true; // 下キーで 0 番（下矢印）を消去
	}
	if (input->PushKey(DIK_UP)) {
		keyPressed_[1] = true; // 上キーで 1 番（上矢印）を消去
	}
	if (input->PushKey(DIK_RIGHT)) {
		keyPressed_[2] = true; // 右キーで 2 番（右矢印）を消去
	}
	if (input->PushKey(DIK_LEFT)) {
		keyPressed_[3] = true; // 左キーで 3 番（左矢印）を消去
	}

	// ★ UP, RIGHT, LEFT, BOTTOM（DOWN）キーのすべてが押されたら非表示（消去）
	if (keyPressed_[0] && keyPressed_[1] && keyPressed_[2] && keyPressed_[3]) {
		isActive_ = false;
		return;
	}

	// ★ プレイヤー頭上への配置・追従（左右・下は同じ頭上高さ、上はさらに1段上に配置）
	constexpr float kBaseHeight = 2.2f; // プレイヤー頭上の基準高さ（左右・下の高さ）
	constexpr float kSideOffset = 2.2f; // 左右への離れ幅
	constexpr float kUpOffset = 4.0f;   // 上矢印の高さ（他の3つより1段上）

	const Vector3 offsets[kNumArrows] = {
	    {0.0f,         kBaseHeight, 0.0f}, // DOWN (下: 頭上の高さ)
	    {0.0f,         kUpOffset,   0.0f}, // UP (上: ほかの3つより1段上)
	    {kSideOffset,  kBaseHeight, 0.0f}, // RIGHT (右: 頭上の高さ)
	    {-kSideOffset, kBaseHeight, 0.0f}  // LEFT (左: 頭上の高さ)
	};

	for (int i = 0; i < kNumArrows; ++i) {
		// ★ Vector3 の各要素を個別に加算
		arrowTransforms_[i].translation_ = {playerPos.x + offsets[i].x, playerPos.y + offsets[i].y, playerPos.z + offsets[i].z};
		arrowTransforms_[i].matWorld_ = MakeAffineMatrix(arrowTransforms_[i].scale_, arrowTransforms_[i].rotation_, arrowTransforms_[i].translation_);
		arrowTransforms_[i].TransferMatrix();
	}
}

void PlayerTutorial::Draw(const Camera& camera) {
	if (!isStageZero_ || !isActive_ || !modelArrow_) {
		return;
	}

	for (int i = 0; i < kNumArrows; ++i) {
		// まだ入力されていない方向の矢印を描画
		if (!keyPressed_[i]) {
			modelArrow_->Draw(arrowTransforms_[i], camera);
		}
	}
}

void PlayerTutorial::ApplyLightManager(const LightManager& lightManager) {
	if (modelArrow_) {
		lightManager.ApplyToModel(modelArrow_, LightManager::GroupType::kWorld);
	}
}