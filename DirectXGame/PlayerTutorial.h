#pragma once
#include "KamataEngine.h"
#include <array>

class LightManager;

class PlayerTutorial {
public:
	PlayerTutorial() = default;
	~PlayerTutorial();

	/// <summary>
	/// 初期化：ステージ0判定と矢印モデルの読み込み
	/// </summary>
	/// <param name="stageIndex">現在のステージインデックス</param>
	/// <param name="playerPos">プレイヤーの初期位置</param>
	void Initialize(int stageIndex, const KamataEngine::Vector3& playerPos);

	/// <summary>
	/// 更新処理：移動操作キーの入力検知と全方向達成判定、位置追従
	/// </summary>
	/// <param name="playerPos">プレイヤーの現在位置</param>
	void Update(const KamataEngine::Vector3& playerPos);

	/// <summary>
	/// 描画処理：ステージ0かつ未クリア時のみ操作矢印を描画
	/// </summary>
	/// <param name="camera">カメラ</param>
	void Draw(const KamataEngine::Camera& camera);

	/// <summary>
	/// ライトグループの適用
	/// </summary>
	void ApplyLightManager(const LightManager& lightManager);

private:
	// 4方向の定義
	static inline const int kNumArrows = 4;

	// 矢印用モデルポインタ
	KamataEngine::Model* modelArrow_ = nullptr;

	// 4方向のWorldTransform（0: UP, 1: DOWN, 2: LEFT, 3: RIGHT）
	std::array<KamataEngine::WorldTransform, kNumArrows> arrowTransforms_;

	// 4方向のキー入力完了フラグ
	std::array<bool, kNumArrows> keyPressed_ = {false, false, false, false};

	bool isStageZero_ = false; // ステージ0判定
	bool isActive_ = true;     // チュートリアル有効フラグ
};