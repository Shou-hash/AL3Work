#pragma once
#include "KamataEngine.h"
#include <vector>

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

	/// <summary>
	/// CatmullRom補間
	/// </summary>
	/// <param name="p0">点0の座標</param>
	/// <param name="p1">点1の座標</param>
	/// <param name="p2">点2の座標</param>
	/// <param name="p3">点3の座標</param>
	/// <param name="t">点1を0.0f、点2を1.0fとした割合指定</param>
	/// <returns>点1と点2の間で指定された座標</returns>
	KamataEngine::Vector3 CatmullRomInterpolation(const KamataEngine::Vector3& p0, const KamataEngine::Vector3& p1, const KamataEngine::Vector3& p2, const KamataEngine::Vector3& p3, float t);

	/// <summary>
	/// CatmullRomスプライン曲線上の座標を得る
	/// </summary>
	/// <param name="points">制御点の集合</param>
	/// <param name="t">スプラインの全区間の中での割合指定[0,1]</param>
	/// <returns>座標</returns>
	KamataEngine::Vector3 CatmullRomPosition(const std::vector<KamataEngine::Vector3>& points, float t);

private:
	// ワールド変換データ
	KamataEngine::WorldTransform worldTransform_;
	// カメラ
	KamataEngine::Camera camera_;

	// スプライン曲線制御点（通過点）
	std::vector<KamataEngine::Vector3> controlPoints_;

	// レール上の進行度 (0.0f 〜 1.0f)
	float trackT_ = 0.0f;
};