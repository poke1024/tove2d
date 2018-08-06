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
