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
#include "utils.h"

int find_unequal_backward(const float *vertices, int start, int n) {
	start = start % n;
	const float x = vertices[2 * start + 0];
	const float y = vertices[2 * start + 1];
	for (int i = 1; i < n; i++) {
		const int j = (start + n - i) % n;
		const float *p = &vertices[2 * j];
		if (unequal(x, y, p[0], p[1])) {
			return j;
		}
	}
	return (start + n - 1) % n;
}

int find_unequal_forward(const float *vertices, int start, int n) {
	start = start % n;
	const float x = vertices[2 * start + 0];
	const float y = vertices[2 * start + 1];
	for (int i = 1; i < n; i++) {
		const int j = (start + i) % n;
		const float *p = &vertices[2 * j];
		if (unequal(x, y, p[0], p[1])) {
			return j;
		}
	}
	return (start + 1) % n;
}

int find_unequal_forward(const float *vertices, const TPPLPoly &poly, int start, int n) {
	start = start % n;
	const int s = poly[start].id * 2;
	const float x = vertices[s + 0];
	const float y = vertices[s + 1];
	for (int i = 1; i < n; i++) {
		const int j = (start + i) % n;
		const float *p = &vertices[poly[j].id * 2];
		if (unequal(x, y, p[0], p[1])) {
			return j;
		}
	}
	return (start + 1) % n;
}

int find_unequal_forward(const float *vertices, const uint16_t *indices, int start, int n) {
	start = start % n;
	const int s = indices[start] * 2;
	const float x = vertices[s + 0];
	const float y = vertices[s + 1];
	for (int i = 1; i < n; i++) {
		const int j = (start + i) % n;
		const float *p = &vertices[indices[j] * 2];
		if (unequal(x, y, p[0], p[1])) {
			return j;
		}
	}
	return (start + 1) % n;
}
