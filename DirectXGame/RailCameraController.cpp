#include "RailCameraController.h"
#include <algorithm>
#include <cassert>
#include <cmath>
#include <imgui.h>

using namespace KamataEngine;

// クランプ関数の定義（スクショ資料内の指定範囲にクランプ用）
namespace {
float 指定範囲にクランプ(float value, float min, float max) { return (std::max)(min, (std::min)(value, max)); }
} // namespace

void RailCameraController::Initialize(const Vector3& position, const Vector3& rotation) {
	// 引数でワールド座標を受け取ってワールドトランスフォームに設定
	worldTransform_.translation_ = position;
	// 引数で回転角[ラジアン]を受け取ってワールドトランスフォームに設定
	worldTransform_.rotation_ = rotation;

	// スケールの初期化 (1.0fにしておかないと行列の計算結果が0になってしまいます)
	worldTransform_.scale_ = {1.0f, 1.0f, 1.0f};

	// カメラのfarZを適切な値（天球の直径以上）に変更する
	// ※値が小さすぎると天球に穴が空き、大きすぎるとZファイティングの原因になるため、
	// プロジェクトの天球サイズに合わせて調整してください（ここでは仮に 2000.0f とします）
	camera_.farZ = 2000.0f;

	// カメラの初期化
	camera_.Initialize();

	// スプライン曲線制御点（通過点）
	controlPoints_ = {
	    {0.0f,  0.0f,  0.0f},
        {10.0f, 10.0f, 0.0f},
        {10.0f, 15.0f, 0.0f},
        {20.0f, 15.0f, 0.0f},
        {20.0f, 0.0f,  0.0f},
        {30.0f, 0.0f,  0.0f},
	};

	// 進行度の初期化
	trackT_ = 0.0f;
}

void RailCameraController::Update() {
	// レールカメラの自動進行、またはImGuiでのデバッグ操作
	ImGui::Begin("Rail Camera");
	ImGui::SliderFloat("Track T", &trackT_, 0.0f, 1.0f);
	ImGui::End();

	// スプライン曲線から現在のカメラ座標を計算
	worldTransform_.translation_ = CatmullRomPosition(controlPoints_, trackT_);

	// 線分で描画する用の頂点リスト (スクショ資料の再現・検証用)
	std::vector<Vector3> pointsDrawing;
	// 線分の数
	const size_t segmentCount = 100;
	// 線分の数+1個分の頂点座標を計算
	for (size_t i = 0; i < segmentCount + 1; ++i) {
		float t = 1.0f / segmentCount * i;
		Vector3 pos = CatmullRomPosition(controlPoints_, t);
		// 描画用頂点リストに追加
		pointsDrawing.push_back(pos);
	}

	// カメラの座標を画面表示する処理
	ImGui::Begin("Camera");
	// スライダーでカメラのtranslationを表示
	ImGui::DragFloat3("Translation", &worldTransform_.translation_.x, 0.1f);
	// スライダーでカメラのrotationを表示
	ImGui::DragFloat3("Rotation", &worldTransform_.rotation_.x, 0.01f);
	ImGui::End();

	// 【行列の再計算と転送】
	// エンジンの関数に依存せず、内部のヘルパー関数で直接アフィン行列を計算して代入します。
	worldTransform_.matWorld_ = MakeAffineMatrix(worldTransform_.scale_, worldTransform_.rotation_, worldTransform_.translation_);

	// カメラオブジェクトのワールド行列からビュー行列を計算する
	// ① カメラオブジェクトのワールド行列の逆行列をビュー行列とする
	camera_.matView = Inverse(worldTransform_.matWorld_);

	// カメラの定数バッファ等へ転送
	camera_.TransferMatrix();
}

// 4x4行列の乗算
Matrix4x4 RailCameraController::Multiply(const Matrix4x4& m1, const Matrix4x4& m2) {
	Matrix4x4 result{};
	for (int i = 0; i < 4; ++i) {
		for (int j = 0; j < 4; ++j) {
			result.m[i][j] = m1.m[i][0] * m2.m[0][j] + m1.m[i][1] * m2.m[1][j] + m1.m[i][2] * m2.m[2][j] + m1.m[i][3] * m2.m[3][j];
		}
	}
	return result;
}

// アフィン行列の作成 (X->Y->Z順回転)
Matrix4x4 RailCameraController::MakeAffineMatrix(const Vector3& scale, const Vector3& rot, const Vector3& translate) {
	Matrix4x4 matScale{};
	matScale.m[0][0] = scale.x;
	matScale.m[1][1] = scale.y;
	matScale.m[2][2] = scale.z;
	matScale.m[3][3] = 1.0f;

	float cx = std::cos(rot.x);
	float sx = std::sin(rot.x);
	float cy = std::cos(rot.y);
	float sy = std::sin(rot.y);
	float cz = std::cos(rot.z);
	float sz = std::sin(rot.z);

	Matrix4x4 matRotX{}, matRotY{}, matRotZ{};
	matRotX.m[0][0] = 1.0f;
	matRotX.m[3][3] = 1.0f;
	matRotX.m[1][1] = cx;
	matRotX.m[1][2] = sx;
	matRotX.m[2][1] = -sx;
	matRotX.m[2][2] = cx;

	matRotY.m[1][1] = 1.0f;
	matRotY.m[3][3] = 1.0f;
	matRotY.m[0][0] = cy;
	matRotY.m[0][2] = -sy;
	matRotY.m[2][0] = sy;
	matRotY.m[2][2] = cy;

	matRotZ.m[2][2] = 1.0f;
	matRotZ.m[3][3] = 1.0f;
	matRotZ.m[0][0] = cz;
	matRotZ.m[0][1] = sz;
	matRotZ.m[1][0] = -sz;
	matRotZ.m[1][1] = cz;

	Matrix4x4 matRot = Multiply(matRotX, Multiply(matRotY, matRotZ));

	Matrix4x4 matTrans{};
	matTrans.m[0][0] = 1.0f;
	matTrans.m[1][1] = 1.0f;
	matTrans.m[2][2] = 1.0f;
	matTrans.m[3][3] = 1.0f;
	matTrans.m[3][0] = translate.x;
	matTrans.m[3][1] = translate.y;
	matTrans.m[3][2] = translate.z;

	return Multiply(matScale, Multiply(matRot, matTrans));
}

// 4x4行列の逆行列計算
Matrix4x4 RailCameraController::Inverse(const Matrix4x4& m) {
	Matrix4x4 inv{};
	float det =
	    m.m[0][0] *
	        (m.m[1][1] * (m.m[2][2] * m.m[3][3] - m.m[2][3] * m.m[3][2]) - m.m[1][2] * (m.m[2][1] * m.m[3][3] - m.m[2][3] * m.m[3][1]) + m.m[1][3] * (m.m[2][1] * m.m[3][2] - m.m[2][2] * m.m[3][1])) -
	    m.m[0][1] *
	        (m.m[1][0] * (m.m[2][2] * m.m[3][3] - m.m[2][3] * m.m[3][2]) - m.m[1][2] * (m.m[2][0] * m.m[3][3] - m.m[2][3] * m.m[3][2]) + m.m[1][3] * (m.m[2][0] * m.m[3][2] - m.m[2][2] * m.m[3][0])) +
	    m.m[0][2] *
	        (m.m[1][0] * (m.m[2][1] * m.m[3][3] - m.m[2][3] * m.m[3][1]) - m.m[1][1] * (m.m[2][0] * m.m[3][3] - m.m[2][3] * m.m[3][0]) + m.m[1][3] * (m.m[2][0] * m.m[3][1] - m.m[2][1] * m.m[3][0])) -
	    m.m[0][3] *
	        (m.m[1][0] * (m.m[2][1] * m.m[3][2] - m.m[2][2] * m.m[3][1]) - m.m[1][1] * (m.m[2][0] * m.m[3][2] - m.m[2][2] * m.m[3][0]) + m.m[1][2] * (m.m[2][0] * m.m[3][1] - m.m[2][1] * m.m[3][0]));

	if (det != 0.0f) {
		float invDet = 1.0f / det;
		inv.m[0][0] =
		    (m.m[1][1] * (m.m[2][2] * m.m[3][3] - m.m[2][3] * m.m[3][2]) - m.m[1][2] * (m.m[2][1] * m.m[3][3] - m.m[2][3] * m.m[3][1]) + m.m[1][3] * (m.m[2][1] * m.m[3][2] - m.m[2][2] * m.m[3][1])) *
		    invDet;
		inv.m[0][1] =
		    -(m.m[0][1] * (m.m[2][2] * m.m[3][3] - m.m[2][3] * m.m[3][2]) - m.m[0][2] * (m.m[2][1] * m.m[3][3] - m.m[2][3] * m.m[3][1]) + m.m[0][3] * (m.m[2][1] * m.m[3][2] - m.m[2][2] * m.m[3][1])) *
		    invDet;
		inv.m[0][2] =
		    (m.m[0][1] * (m.m[1][2] * m.m[3][3] - m.m[1][3] * m.m[3][2]) - m.m[0][2] * (m.m[1][1] * m.m[3][3] - m.m[1][3] * m.m[3][1]) + m.m[0][3] * (m.m[1][1] * m.m[3][2] - m.m[1][2] * m.m[3][1])) *
		    invDet;
		inv.m[0][3] =
		    -(m.m[0][1] * (m.m[1][2] * m.m[2][3] - m.m[1][3] * m.m[2][2]) - m.m[0][2] * (m.m[1][1] * m.m[2][3] - m.m[1][3] * m.m[2][1]) + m.m[0][3] * (m.m[1][1] * m.m[2][2] - m.m[1][2] * m.m[2][1])) *
		    invDet;

		inv.m[1][0] =
		    -(m.m[1][0] * (m.m[2][2] * m.m[3][3] - m.m[2][3] * m.m[3][2]) - m.m[1][2] * (m.m[2][0] * m.m[3][3] - m.m[2][3] * m.m[3][0]) + m.m[1][3] * (m.m[2][0] * m.m[3][2] - m.m[2][2] * m.m[3][0])) *
		    invDet;
		inv.m[1][1] =
		    (m.m[0][0] * (m.m[2][2] * m.m[3][3] - m.m[2][3] * m.m[3][2]) - m.m[0][2] * (m.m[2][0] * m.m[3][3] - m.m[2][3] * m.m[3][0]) + m.m[0][3] * (m.m[2][0] * m.m[3][2] - m.m[2][2] * m.m[3][0])) *
		    invDet;
		inv.m[1][2] =
		    -(m.m[0][0] * (m.m[1][2] * m.m[3][3] - m.m[1][3] * m.m[3][2]) - m.m[0][2] * (m.m[1][0] * m.m[3][3] - m.m[1][3] * m.m[3][0]) + m.m[0][3] * (m.m[1][0] * m.m[3][2] - m.m[1][2] * m.m[3][0])) *
		    invDet;
		inv.m[1][3] =
		    (m.m[0][0] * (m.m[1][2] * m.m[2][3] - m.m[1][3] * m.m[2][2]) - m.m[0][2] * (m.m[1][0] * m.m[2][3] - m.m[1][3] * m.m[2][0]) + m.m[0][3] * (m.m[1][0] * m.m[2][2] - m.m[1][2] * m.m[2][0])) *
		    invDet;

		inv.m[2][0] =
		    (m.m[1][0] * (m.m[2][1] * m.m[3][3] - m.m[2][3] * m.m[3][1]) - m.m[1][1] * (m.m[2][0] * m.m[3][3] - m.m[2][3] * m.m[3][0]) + m.m[1][3] * (m.m[2][0] * m.m[3][1] - m.m[2][1] * m.m[3][0])) *
		    invDet;
		inv.m[2][1] =
		    -(m.m[0][0] * (m.m[2][1] * m.m[3][3] - m.m[2][3] * m.m[3][1]) - m.m[0][1] * (m.m[2][0] * m.m[3][3] - m.m[2][3] * m.m[3][0]) + m.m[0][3] * (m.m[2][0] * m.m[3][1] - m.m[2][1] * m.m[3][0])) *
		    invDet;
		inv.m[2][2] =
		    (m.m[0][0] * (m.m[1][1] * m.m[3][3] - m.m[1][3] * m.m[3][1]) - m.m[0][1] * (m.m[1][0] * m.m[3][3] - m.m[1][3] * m.m[3][0]) + m.m[0][3] * (m.m[1][0] * m.m[3][1] - m.m[1][1] * m.m[3][0])) *
		    invDet;
		inv.m[2][3] =
		    -(m.m[0][0] * (m.m[1][1] * m.m[2][3] - m.m[1][3] * m.m[2][1]) - m.m[0][1] * (m.m[1][0] * m.m[2][3] - m.m[1][3] * m.m[2][0]) + m.m[0][3] * (m.m[1][0] * m.m[2][1] - m.m[1][1] * m.m[2][0])) *
		    invDet;

		inv.m[3][0] =
		    -(m.m[1][0] * (m.m[2][1] * m.m[3][2] - m.m[2][2] * m.m[3][1]) - m.m[1][1] * (m.m[2][0] * m.m[3][2] - m.m[2][2] * m.m[3][0]) + m.m[1][2] * (m.m[2][0] * m.m[3][1] - m.m[2][1] * m.m[3][0])) *
		    invDet;
		inv.m[3][1] =
		    (m.m[0][0] * (m.m[2][1] * m.m[3][2] - m.m[2][2] * m.m[3][1]) - m.m[0][1] * (m.m[2][0] * m.m[3][2] - m.m[2][2] * m.m[3][0]) + m.m[0][2] * (m.m[2][0] * m.m[3][1] - m.m[2][1] * m.m[3][0])) *
		    invDet;
		inv.m[3][2] =
		    -(m.m[0][0] * (m.m[1][1] * m.m[3][2] - m.m[1][2] * m.m[3][1]) - m.m[0][1] * (m.m[1][0] * m.m[3][2] - m.m[1][2] * m.m[3][0]) + m.m[0][2] * (m.m[1][0] * m.m[3][1] - m.m[1][1] * m.m[3][0])) *
		    invDet;
		inv.m[3][3] =
		    (m.m[0][0] * (m.m[1][1] * m.m[2][2] - m.m[1][2] * m.m[2][1]) - m.m[0][1] * (m.m[1][0] * m.m[2][2] - m.m[1][2] * m.m[2][0]) + m.m[0][2] * (m.m[1][0] * m.m[2][1] - m.m[1][1] * m.m[2][0])) *
		    invDet;
	}
	return inv;
}

Vector3 RailCameraController::CatmullRomInterpolation(const Vector3& p0, const Vector3& p1, const Vector3& p2, const Vector3& p3, float t) {
	const float s = 0.5f; // 数式に出てくる 1/2 のこと。[cite: 4]

	float t2 = t * t;  // t の2乗[cite: 4]
	float t3 = t2 * t; // t の3乗[cite: 4]

	// 各要素ごとに計算を行うように変更
	Vector3 result{};

	// X要素の計算
	float e3_x = -p0.x + 3.0f * p1.x - 3.0f * p2.x + p3.x;
	float e2_x = 2.0f * p0.x - 5.0f * p1.x + 4.0f * p2.x - p3.x;
	float e1_x = -p0.x + p2.x;
	float e0_x = 2.0f * p1.x;
	result.x = s * (e3_x * t3 + e2_x * t2 + e1_x * t + e0_x);

	// Y要素の計算
	float e3_y = -p0.y + 3.0f * p1.y - 3.0f * p2.y + p3.y;
	float e2_y = 2.0f * p0.y - 5.0f * p1.y + 4.0f * p2.y - p3.y;
	float e1_y = -p0.y + p2.y;
	float e0_y = 2.0f * p1.y;
	result.y = s * (e3_y * t3 + e2_y * t2 + e1_y * t + e0_y);

	// Z要素の計算
	float e3_z = -p0.z + 3.0f * p1.z - 3.0f * p2.z + p3.z;
	float e2_z = 2.0f * p0.z - 5.0f * p1.z + 4.0f * p2.z - p3.z;
	float e1_z = -p0.z + p2.z;
	float e0_z = 2.0f * p1.z;
	result.z = s * (e3_z * t3 + e2_z * t2 + e1_z * t + e0_z);

	return result;
}

Vector3 RailCameraController::CatmullRomPosition(const std::vector<Vector3>& points, float t) {
	assert(points.size() >= 4 && "制御点は4点以上必要です");

	// 区間数は制御点の数-1
	size_t division = points.size() - 1;
	// 1区間の長さ（全体を1.0とした割合）
	float areaWidth = 1.0f / division;

	// 区間内の始点を0.0f、終点を1.0fとしたときの現在位置
	float t_2 = std::fmod(t, areaWidth) * division;
	// 下限(0.0f)上限(1.0f)の範囲に収める
	t_2 = 指定範囲にクランプ(t_2, 0.0f, 1.0f);

	// 区間番号
	size_t index = static_cast<size_t>(t / areaWidth);
	// 区間番号が上限を超えないように収める
	if (index >= division) {
		index = division - 1;
	}

	// 4点分のインデックス
	size_t index0 = index - 1;
	size_t index1 = index;
	size_t index2 = index + 1;
	size_t index3 = index + 2;

	// 最初の区間のp0はp1を重複使用する
	if (index == 0) {
		index0 = index1;
	}

	// 最後の区間のp3はp2を重複使用する
	if (index3 >= points.size()) {
		index3 = index2;
	}

	// 4点の座標
	const Vector3& p0 = points[index0];
	const Vector3& p1 = points[index1];
	const Vector3& p2 = points[index2];
	const Vector3& p3 = points[index3];

	// 4点を指定してCatmull-Rom補間
	return CatmullRomInterpolation(p0, p1, p2, p3, t_2);
}