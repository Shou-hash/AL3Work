#pragma once
#include "Kamataengine.h"
#include <cmath>

inline KamataEngine::Matrix4x4 MakeAffineMatrix(const KamataEngine::Vector3& scale, const KamataEngine::Vector3& rot, const KamataEngine::Vector3& translate) {

	KamataEngine::Matrix4x4 result{};

	// 各軸の回転のサイン・コサインを計算
	float cx = std::cos(rot.x);
	float sx = std::sin(rot.x);
	float cy = std::cos(rot.y);
	float sy = std::sin(rot.y);
	float cz = std::cos(rot.z);
	float sz = std::sin(rot.z);

	// 行列の計算 (Scale * Rotate * Translate)

	// 1行目 (X軸)
	result.m[0][0] = scale.x * (cy * cz + sx * sy * sz);
	result.m[0][1] = scale.x * (sx * sy * cz - cy * sz);
	result.m[0][2] = scale.x * (cx * sy);
	result.m[0][3] = 0.0f;

	// 2行目 (Y軸)
	result.m[1][0] = scale.y * (cx * sz);
	result.m[1][1] = scale.y * (cx * cz);
	result.m[1][2] = scale.y * (-sx);
	result.m[1][3] = 0.0f;

	// 3行目 (Z軸)
	result.m[2][0] = scale.z * (sy * cz - cy * sx * sz);
	result.m[2][1] = scale.z * (-sy * sz - cy * sx * cz);
	result.m[2][2] = scale.z * (cx * cy);
	result.m[2][3] = 0.0f;

	// 4行目 (平行移動)
	result.m[3][0] = translate.x;
	result.m[3][1] = translate.y;
	result.m[3][2] = translate.z;
	result.m[3][3] = 1.0f;

	return result;
}

inline KamataEngine::Matrix4x4 Multiply(const KamataEngine::Matrix4x4& m1, const KamataEngine::Matrix4x4& m2) {
	KamataEngine::Matrix4x4 result{};
	for (int i = 0; i < 4; ++i) {
		for (int j = 0; j < 4; ++j) {
			result.m[i][j] = m1.m[i][0] * m2.m[0][j] + m1.m[i][1] * m2.m[1][j] + m1.m[i][2] * m2.m[2][j] + m1.m[i][3] * m2.m[3][j];
		}
	}
	return result;
}