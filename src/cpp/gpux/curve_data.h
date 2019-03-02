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

#ifndef __TOVE_CURVEDATA
#define __TOVE_CURVEDATA 1

#include "../common.h"
#include <cmath>

BEGIN_TOVE_NAMESPACE

struct CurveBounds {
	struct Roots {
		int count;
		float t[2];
		float positions[4];
	};

	// note: it's very important that the curve's bounds and
	// endpoints values are indeed numerically identical if they
	// are the same; otherwise we will end up with too many
	// values in the LUT (see class LookupTable).

	float bounds[4];
	Roots roots[2];
	float sroots[4];

	void update(const float *curve, float *bx, float *by);
};

template<typename T>
inline void bezierpc(T P0, T P1, T P2, T P3, T *Z) {
	Z[0] = -P0 + 3 * P1  + -3 * P2 + P3;
    Z[1] = 3 * P0 - 6 * P1 + 3 * P2;
    Z[2] = -3 * P0 + 3 * P1;
    Z[3] = P0;
}

class CurveData {
public:
	coeff bx[4];
	coeff by[4];
	CurveBounds bounds;

	inline void updatePCs(const float *pts) {
		bezierpc(pts[0], pts[2], pts[4], pts[6], bx);
		bezierpc(pts[1], pts[3], pts[5], pts[7], by);
	}

	inline void updateBounds(const float *pts) {
		bounds.update(pts, bx, by);
	}

	void storeRoots(gpu_float_t *out) const;
};

enum {
	IGNORE_FILL = 1,
	IGNORE_LINE = 2
};

struct EndPoints {
	float p1[2], p2[2];
};

struct ExCurveData {
	const CurveBounds *bounds;
	EndPoints endpoints;
	uint8_t ignore;
};

END_TOVE_NAMESPACE

#endif // __TOVE_CURVEDATA
