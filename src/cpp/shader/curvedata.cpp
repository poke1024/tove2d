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

#include "../common.h"
#include "curvedata.h"
#include <algorithm>

void ExtendedCurveData::copy(uint16_t *out) {
	float t[4];
	int count = 0;

	for (int i = 0; i < 2; i++) {
		const Roots &r = roots[i];
		for (int j = 0; j < r.count; j++) {
			t[count++] = r.t[j];
		}
	}

	if (count > 1) {
		std::sort(t, t + count);
	}

	for (int i = 0; i < count; i++) {
		store_fp16(out[i], t[i]);
	}
	for (int i = count; i < 4; i++) {
		store_fp16(out[i], 1.0f);
	}
}

void ExtendedCurveData::set(const float *curve, const float *bx, const float *by) {
	// adapted from nsvg__curveBounds.

	endpoints.p1[0] = curve[0];
	endpoints.p1[1] = curve[1];
	endpoints.p2[0] = curve[6];
	endpoints.p2[1] = curve[7];

	const float *v0 = &curve[0];
	const float *v1 = &curve[2];
	const float *v2 = &curve[4];
	const float *v3 = &curve[6];

	const float eps = 1e-12;

	for (int i = 0; i < 2; i++) {
		int count = 0;
		Roots &r = roots[i];

		float a = -3.0 * v0[i] + 9.0 * v1[i] - 9.0 * v2[i] + 3.0 * v3[i];
		float b = 6.0 * v0[i] - 12.0 * v1[i] + 6.0 * v2[i];
		float c = 3.0 * v1[i] - 3.0 * v0[i];

		if (fabs(a) < eps) {
			if (fabs(b) > eps) {
				double t = -c / b;
				if (t > 0.0 && t < 1.0) {
					r.t[count++] = t;
				}
			}
		} else {
			double b2ac = b*b - 4.0*c*a;
			if (b2ac > 0.0) {
				double t = (-b + sqrt(b2ac)) / (2.0 * a);
				if (t > 0.0 && t < 1.0) {
					r.t[count++] = t;
				}
				t = (-b - sqrt(b2ac)) / (2.0 * a);
				if (t > 0.0 && t < 1.0) {
					r.t[count++] = t;
				}
			}
		}

		for (int j = 0; j < count; j++) {
			float t = r.t[j];
			float t2 = t * t;
			float t3 = t2 * t;

			r.positions[2 * j + 0] = bx[0] * t3 + bx[1] * t2 + bx[2] * t + bx[3];
			r.positions[2 * j + 1] = by[0] * t3 + by[1] * t2 + by[2] * t + by[3];
		}

		r.count = count;
	}

	// Start the bounding box by end points
	bounds[0] = std::min(v0[0], v3[0]);
	bounds[1] = std::min(v0[1], v3[1]);
	bounds[2] = std::max(v0[0], v3[0]);
	bounds[3] = std::max(v0[1], v3[1]);

	for (int i = 0; i < 2; i++) {
		const Roots &r = roots[i];
		for (int j = 0; j < r.count; j++) {
			const float v = r.positions[2 * j + i];
			bounds[0 + i] = std::min(bounds[0 + i], v);
			bounds[2 + i] = std::max(bounds[2 + i], v);
		}
	}
}
