#include "MyMath.h"
#include <algorithm> // std::clamp 用
#include <cmath>

namespace MyMath {

// ベクトルの長さ（ノルム）
float Length(const KamataEngine::Vector3& v) { return std::sqrt(v.x * v.x + v.y * v.y + v.z * v.z); }

// ベクトルの正規化
KamataEngine::Vector3 Normalize(const KamataEngine::Vector3& v) {
	float len = Length(v);
	if (len != 0.0f) {
		return {v.x / len, v.y / len, v.z / len};
	}
	return {0.0f, 0.0f, 0.0f};
}

// 内積
float Dot(const KamataEngine::Vector3& v1, const KamataEngine::Vector3& v2) { return v1.x * v2.x + v1.y * v2.y + v1.z * v2.z; }

// スカラーの線形補間 (Lerp)
float Lerp(float start, float end, float t) { return start + (end - start) * t; }

// ----------------------------------------------------------------
// 球面線形補間 (Slerp)
// ----------------------------------------------------------------
KamataEngine::Vector3 Slerp(const KamataEngine::Vector3& v1, const KamataEngine::Vector3& v2, float t) {
	// 1. V1, V2 の正規化ベクトルを求める
	KamataEngine::Vector3 normV1 = Normalize(v1);
	KamataEngine::Vector3 normV2 = Normalize(v2);

	// 2. 内積を求める（これが cosθ となる）
	float dot = Dot(normV1, normV2);

	// 3. floatの計算誤差により 1.0f を超えるのを防ぐ（-1.0f ～ 1.0f にクランプ）
	dot = std::clamp(dot, -1.0f, 1.0f);

	// 4. アークコサインで θ の角度を求める
	float theta = std::acos(dot);

	// 5. θ の角度から sinθ を求める
	float sinTheta = std::sin(theta);

	// 6. sin(θ(1 - t)) と sin(θt) を求める
	float sinThetaFrom = std::sin((1.0f - t) * theta);
	float sinThetaTo = std::sin(t * theta);

	// 7. ゼロ除算の修正 ＆ 正規化補間ベクトルの計算
	KamataEngine::Vector3 normInterpolated;

	// sinTheta が 0 またはそれに近い微小な値のときは角度差がほぼ 0
	if (sinTheta < 1.0e-5f) {
		normInterpolated = normV1;
	} else {
		// 球面線形補間したベクトル（単位ベクトル）
		normInterpolated = {
		    (sinThetaFrom * normV1.x + sinThetaTo * normV2.x) / sinTheta, (sinThetaFrom * normV1.y + sinThetaTo * normV2.y) / sinTheta, (sinThetaFrom * normV1.z + sinThetaTo * normV2.z) / sinTheta};
	}

	// 8. ベクトルの長さは v1 と v2 の長さを線形補間（Lerp）
	float length1 = Length(v1);
	float length2 = Length(v2);
	float length = Lerp(length1, length2, t);

	// 9. 長さを反映して返す
	return {length * normInterpolated.x, length * normInterpolated.y, length * normInterpolated.z};
}

} // namespace MyMath