#pragma once
#include <KamataEngine.h>

namespace MyMath {

// ベクトルの正規化
KamataEngine::Vector3 Normalize(const KamataEngine::Vector3& v);

// ベクトルの長さを求める
float Length(const KamataEngine::Vector3& v);

// 内積を求める
float Dot(const KamataEngine::Vector3& v1, const KamataEngine::Vector3& v2);

// 線形補間（Lerp）
float Lerp(float start, float end, float t);

// 2ベクトルを球面線形補間する Slerp 関数
KamataEngine::Vector3 Slerp(const KamataEngine::Vector3& v1, const KamataEngine::Vector3& v2, float t);

} // namespace MyMath