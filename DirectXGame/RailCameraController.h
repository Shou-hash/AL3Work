#pragma once
#include "KamataEngine.h"

/// <summary>
/// レールカメラコントローラ
/// </summary>
class RailCameraController {
public:
	/// <summary>
	/// 初期化
	/// </summary>
	/// <param name="position">初期座標</param>
	/// <param name="rotation">初期角度[ラジアン]</param>
	void Initialize(const KamataEngine::Vector3& position, const KamataEngine::Vector3& rotation);

	/// <summary>
	/// 更新
	/// </summary>
	void Update();

	/// <summary>
	/// カメラの取得関数 (デバッグカメラを参考に)
	/// </summary>
	const KamataEngine::Camera& GetCamera() const { return camera_; }

private:
	// 行列の乗算ヘルパー
	KamataEngine::Matrix4x4 Multiply(const KamataEngine::Matrix4x4& m1, const KamataEngine::Matrix4x4& m2);

	// アフィン変換行列の作成ヘルパー
	KamataEngine::Matrix4x4 MakeAffineMatrix(const KamataEngine::Vector3& scale, const KamataEngine::Vector3& rot, const KamataEngine::Vector3& translate);

	// 逆行列の計算ヘルパー
	KamataEngine::Matrix4x4 Inverse(const KamataEngine::Matrix4x4& m);

private:
	// ワールド変換データ
	KamataEngine::WorldTransform worldTransform_;
	// カメラ
	KamataEngine::Camera camera_;
};