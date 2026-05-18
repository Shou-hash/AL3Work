#pragma once
#include <cmath>

inline float EaseOutBounce(float x) {
	const float n1 = 7.5625f;
	const float d1 = 2.75f;
	if (x < 1.0f / d1) {
		return n1 * x * x;
	} else if (x < 2.0f / d1) {
		x -= 1.5f / d1;
		return n1 * x * x + 0.75f;
	} else if (x < 2.5f / d1) {
		x -= 2.25f / d1;
		return n1 * x * x + 0.9375f;
	} else {
		x -= 2.625f / d1;
		return n1 * x * x + 0.984375f;
	}
}

inline float EaseInOutQuart(float t) {
	if (t < 0.5f) {
		return 8.0f * t * t * t * t;
	} else {
		float f = (t - 1.0f);
		return 1.0f - 8.0f * f * f * f * f;
	}
}

inline float EaseOutQuart(float t) { return 1.0f - powf(1.0f - t, 4); }

// --- 追加: EaseInBack ---
inline float EaseInBack(float x) {
	const float c1 = 1.70158f;
	const float c3 = c1 + 1.0f;
	return c3 * x * x * x - c1 * x * x;
}