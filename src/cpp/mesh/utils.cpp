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

int find_unequal_backward(
	const Vertices &vertices, int start, int n) {
	start = start % n;
	const float x = vertices[start].x;
	const float y = vertices[start].y;
	for (int i = 1; i < n; i++) {
		const int j = (start + n - i) % n;
		const auto &p = vertices[j];
		if (unequal(x, y, p.x, p.y)) {
			return j;
		}
	}
	return (start + n - 1) % n;
}

int find_unequal_forward(
	const Vertices &vertices, int start, int n) {
	start = start % n;
	const float x = vertices[start].x;
	const float y = vertices[start].y;
	for (int i = 1; i < n; i++) {
		const int j = (start + i) % n;
		const auto &p = vertices[j];
		if (unequal(x, y, p.x, p.y)) {
			return j;
		}
	}
	return (start + 1) % n;
}

int find_unequal_forward(
	const Vertices &vertices, const TPPLPoly &poly, int start, int n) {
	start = start % n;
	const auto &s = vertices[poly[start].id];
	const float x = s.x;
	const float y = s.y;
	for (int i = 1; i < n; i++) {
		const int j = (start + i) % n;
		const auto &p = vertices[poly[j].id];
		if (unequal(x, y, p.x, p.y)) {
			return j;
		}
	}
	return (start + 1) % n;
}

int find_unequal_forward(
	const Vertices &vertices, const uint16_t *indices, int start, int n) {
	start = start % n;
	const auto &s = vertices[indices[start]];
	const float x = s.x;
	const float y = s.y;
	for (int i = 1; i < n; i++) {
		const int j = (start + i) % n;
		const auto &p = vertices[indices[j]];
		if (unequal(x, y, p.x, p.y)) {
			return j;
		}
	}
	return (start + 1) % n;
}
