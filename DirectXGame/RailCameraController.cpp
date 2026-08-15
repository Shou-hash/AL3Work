#include "RailCameraController.h"
#include <cmath>
#include <imgui.h>

using namespace KamataEngine;

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
}

void RailCameraController::Update() {
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