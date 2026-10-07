#pragma once
#include "KamataEngine.h"
#include <vector>

class MapChipField;

class BackgroundBlocks {
public:
	struct LayerObject {
		KamataEngine::WorldTransform transform;
		KamataEngine::Model* model = nullptr;
		KamataEngine::Vector3 basePosition; // 初期配置座標（パララックス計算用）
		float parallaxFactor = 1.0f;        // 視差係数（0.0=固定、1.0=手前と同速）
	};

	BackgroundBlocks() = default;
	~BackgroundBlocks();

	/// <summary>
	/// 初期化：モデル読み込み & 多層背景オブジェクトの自然不規則生成
	/// </summary>
	void Initialize(MapChipField* mapChipField);

	/// <summary>
	/// 更新処理：カメラ移動に伴う視差（パララックス）移動と行列計算
	/// </summary>
	void Update(const KamataEngine::Camera& camera);

	/// <summary>
	/// 描画処理
	/// </summary>
	void Draw(const KamataEngine::Camera& camera);

private:
	// 使用するモデルポインタ
	KamataEngine::Model* modelBlock_ = nullptr;
	KamataEngine::Model* modelBlockAbove_ = nullptr;

	// 背景レイヤーオブジェクトのリスト
	std::vector<LayerObject*> backgroundObjects_;
};