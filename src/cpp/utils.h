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

inline float clamp(float x, float v0, float v1) {
	return std::max(std::min(x, v1), v0);
}

inline float lerp(float a, float b, float t) {
	return a + (b - a) * t;
}

inline void coeffs(float P0, float P1, float P2, float P3, float *Z) {
	Z[0] = -P0 + 3 * P1  + -3 * P2 + P3;
    Z[1] = 3 * P0 - 6 * P1 + 3 * P2;
    Z[2] = -3 * P0 + 3 * P1;
    Z[3] = P0;
}

inline int mod4(int n) {
    return (n / 4) + (n % 4 ? 1 : 0);
}

inline int ncurves(int npts) {
	int n = npts / 3;
	if (n > 0 && (n - 1) * 3 + 4 > npts) {
		return n - 1;
	} else {
		return n;
	}
}

#endif // __TOVE_UTILS
