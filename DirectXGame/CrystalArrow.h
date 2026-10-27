#pragma once
#include "KamataEngine.h"

/// <summary>
/// 攻撃できるクリスタルを指す矢印（プレイヤーの周りに表示する）
/// Arrow.obj（モデルの最初の向きは右）を、プレイヤーからクリスタルへ向けて回転させ、プレイヤーの周りの円周上に置く。
/// 現れるときはポンッと拡大し、指している間は Scale が脈打ち、クリスタルが近いときや使えないときは縮んで消える。
/// </summary>
class CrystalArrow {
public:
	void Initialize(KamataEngine::Model* model, KamataEngine::Camera* camera);

	// active: 攻撃できるクリスタルがあるか / playerCenter: プレイヤーの体の中心 / targetPos: クリスタルの位置
	void Update(bool active, const KamataEngine::Vector3& playerCenter, const KamataEngine::Vector3& targetPos);

	void Draw();

private:
	KamataEngine::Model* model_ = nullptr;
	KamataEngine::Camera* camera_ = nullptr;
	KamataEngine::WorldTransform worldTransform_;

	float timer_ = 0.0f;         // 脈打ち用のタイマー
	float appear_ = 0.0f;        // 出現の進行度（0.0 = 非表示、1.0 = 表示）
	bool wasVisible_ = false;    // 前のフレームで表示対象だったか
	bool isDrawEnabled_ = false; // 描画するか
};
