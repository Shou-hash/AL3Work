#pragma once
#include "KamataEngine.h"
#include <random>
#include <string>
#include <vector>

class MapChipField;

class BackgroundBlocks {
public:
	struct LayerObject {
		KamataEngine::WorldTransform transform;
		KamataEngine::Model* model = nullptr;
		KamataEngine::Vector3 basePosition;                       // 初期配置座標（パララックス計算用）
		float parallaxFactor = 1.0f;                              // 視差係数（0.0=固定、1.0=手前と同速）
		float cullRange = 80.0f;                                  // カメラとのX方向の距離がこれを超えたら描画しない（描画負荷の軽減）
		bool isCloud = false;                                     // ★ 雲オブジェクト判定フラグ
		float moveSpeed = 0.0f;                                   // ★ 右から左への移動速度
		KamataEngine::Vector3 originalScale = {1.0f, 1.0f, 1.0f}; // ★ ランダム生成された元のスケール
	};

	BackgroundBlocks() = default;
	~BackgroundBlocks();

	/// <summary>
	/// 初期化：モデル読み込み & 自動生成による背景オブジェクトの生成
	/// CSVを使用せずプログラム制御で自然不規則に背景（中景・遠景・雲）を生成する
	/// </summary>
	void Initialize(MapChipField* mapChipField, const std::string& csvPath = "");

	/// <summary>
	/// 更新処理：カメラ移動に伴う視差（パララックス）移動、右から左への流れるアニメーション、行列計算
	/// </summary>
	void Update(const KamataEngine::Camera& camera);

	/// <summary>
	/// 描画処理
	/// </summary>
	void Draw(const KamataEngine::Camera& camera);

	/// <summary>
	/// ライトグループの設定：奥行きステージ構造・中景・遠景で別々のライト（明るさ・彩度）を使う
	/// </summary>
	void SetLightGroups(const KamataEngine::LightGroup* nearGroup, const KamataEngine::LightGroup* midGroup, const KamataEngine::LightGroup* farGroup);

private:
	// 使用するモデルポインタ
	KamataEngine::Model* modelBlock_ = nullptr;
	KamataEngine::Model* modelBlockAbove_ = nullptr;
	KamataEngine::Model* modelCloud_ = nullptr; // ★ 追加: 雲用モデルポインタ (cloud3)

	// ★ 奥行きステージ構造（S）用のタイルモデル（ステージのタイルと同じモデル。向きは GameScene と同じ判定で使い分ける）
	//   0: block（通常） 1: blockAbove（上が空白） 2: blockBelow（下が空白） 3: blockLeft（左が空白） 4: blockRight（右が空白）
	static inline const int kNumTileModels = 5;
	KamataEngine::Model* tileModels_[kNumTileModels] = {nullptr};

	// 背景レイヤーオブジェクトのリスト
	std::vector<LayerObject*> backgroundObjects_;

	// ★ 雲の無限ループ・再生成用パラメータ
	float stageWidth_ = 100.0f; // ステージ全体の横幅
	std::mt19937 rng_{1337};    // 乱数生成器
};