/*
 * TÖVE - Animated vector graphics for LÖVE.
 * https://github.com/poke1024/tove2d
 *
 * Copyright (c) 2018, Bernhard Liebl
 *
 * Distributed under the MIT license. See LICENSE file for details.
 *
 * All rights reserved.
 */

#ifndef __TOVE_UTILS
#define __TOVE_UTILS 1

#include "common.h"
#include <cmath>

BEGIN_TOVE_NAMESPACE

struct vec2 {
	float x;
	float y;

	inline vec2() {
	}

	inline vec2(float x, float y) : x(x), y(y) {
	}

	inline float magnitude() const {
		return std::sqrt(x * x + y * y);
	}

	inline vec2 normalized() const {
		const float m = magnitude();
		return vec2(x / m, y / m);
	}

	inline float dot(const vec2 &v) const {
		return x * v.x + y * v.y;
	}

	inline float angle(const vec2 &v) const {
		return std::atan2(x * v.y - y * v.x, x * v.x + y * v.y);
	}

	inline vec2 rotated(float angle) const {
		const float c = std::cos(angle);
		const float s = std::sin(angle);
		return vec2(c * x - s * y, s * x + c * y);
	}

	inline vec2 operator*(float s) const {
		return vec2(x * s, y * s);
	}
};

inline float clamp(float x, float v0, float v1) {
	return std::max(std::min(x, v1), v0);
}

inline float lerp(float a, float b, float t) {
	return a + (b - a) * t;
}

inline int div4(int n) {
    return (n / 4) + (n % 4 ? 1 : 0);
}

inline int ncurves(int npts) {
	int n = npts / 3;
	n -= int(n > 0 && (n - 1) * 3 + 4 > npts);
	return n;
}

template<typename T>
inline double dot4(const T *v, double x, double y, double z, double w) {
	return v[0] * x + v[1] * y + v[2] * z + v[3] * w;
}

template<typename T>
inline double dot3(const T *v, double x, double y, double z) {
	return v[0] * x + v[1] * y + v[2] * z;
}

template<typename T> int sgn(T val) {
    return (T(0) < val) - (val < T(0));
}

inline float wrapAngle(float phi) {
	return phi - 360 * std::floor(phi / 360.0);
}

END_TOVE_NAMESPACE

#endif // __TOVE_UTILS
