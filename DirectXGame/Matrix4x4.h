#pragma once
#include "Kamataengine.h"
#include <cmath>

// 行列の乗算ヘルパー（Multiplyがすでにあるのでこれを利用）
inline KamataEngine::Matrix4x4 Multiply(const KamataEngine::Matrix4x4& m1, const KamataEngine::Matrix4x4& m2) {
	KamataEngine::Matrix4x4 result{};
	for (int i = 0; i < 4; ++i) {
		for (int j = 0; j < 4; ++j) {
			result.m[i][j] = m1.m[i][0] * m2.m[0][j] + m1.m[i][1] * m2.m[1][j] + m1.m[i][2] * m2.m[2][j] + m1.m[i][3] * m2.m[3][j];
		}
	}
	return result;
}

inline KamataEngine::Matrix4x4 MakeAffineMatrix(const KamataEngine::Vector3& scale, const KamataEngine::Vector3& rot, const KamataEngine::Vector3& translate) {
	// --- 1. スケール行列 ---
	KamataEngine::Matrix4x4 matScale{};
	matScale.m[0][0] = scale.x;
	matScale.m[1][1] = scale.y;
	matScale.m[2][2] = scale.z;
	matScale.m[3][3] = 1.0f;

	// --- 2. 回転行列 (X, Y, Z個別に作成) ---
	float cx = std::cos(rot.x);
	float sx = std::sin(rot.x);
	float cy = std::cos(rot.y);
	float sy = std::sin(rot.y);
	float cz = std::cos(rot.z);
	float sz = std::sin(rot.z);

	KamataEngine::Matrix4x4 matRotX{}, matRotY{}, matRotZ{};
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

	// 回転を合成 (一般的に Z * X * Y または X * Y * Z など用途に合わせるが、ここではXYZ順で合成)
	KamataEngine::Matrix4x4 matRot = Multiply(matRotX, Multiply(matRotY, matRotZ));

	// --- 3. 平行移動行列 ---
	KamataEngine::Matrix4x4 matTrans{};
	matTrans.m[0][0] = 1.0f;
	matTrans.m[1][1] = 1.0f;
	matTrans.m[2][2] = 1.0f;
	matTrans.m[3][3] = 1.0f;
	matTrans.m[3][0] = translate.x;
	matTrans.m[3][1] = translate.y;
	matTrans.m[3][2] = translate.z;

	// --- 4. すべてを合成 (Scale -> Rotate -> Translate) ---
	return Multiply(matScale, Multiply(matRot, matTrans));
}