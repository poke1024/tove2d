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

#include <cmath>

enum {
	IGNORE_FILL = 1,
	IGNORE_LINE = 2
};

class ExtendedCurveData {
private:
	struct Root {
		float t;
		int d; // dimension
	};

public:
	struct EndPoints {
		float p1[2], p2[2];
	};

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
	EndPoints endpoints;
	Roots roots[2];
	uint8_t ignore;

	void copy(__fp16 *out);
	void set(const float *curve, const float *bx, const float *by);
};

#endif // __TOVE_CURVEDATA
