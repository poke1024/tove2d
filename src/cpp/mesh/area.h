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

#ifndef __TOVE_MESH_AREA
#define __TOVE_MESH_AREA 1

#include <vector>
#include "utils.h"

BEGIN_TOVE_NAMESPACE

class VanishingTriangles {
	template<int W, typename V>
	inline static bool check(const V &vertices, const ToveVertexIndex *indices) {
		bool good = true;
	
		#pragma clang loop vectorize(enable) interleave(enable)
		for (int i = 0; i < W; i++) {
			const vec2 &a = vertices[indices[i * 3 + 0]];
			const vec2 &b = vertices[indices[i * 3 + 1]];
			const vec2 &c = vertices[indices[i * 3 + 2]];

			good &= std::abs(cross(
				a.x, a.y,
				b.x, b.y,
				c.x, c.y)) < 1e-2f;
		}
	
		return good;
	}

	std::vector<ToveVertexIndex> indices;

public:
	void add(
		ToveVertexIndex a,
		ToveVertexIndex b,
		ToveVertexIndex c) {
		
		const ToveVertexIndex i[] = {a, b, c};
		indices.insert(indices.end(), i, i + 3);
	}

	template<typename V>
	bool check(const V &vertices) const {
		const int n = indices.size();
		int i = 0;
		while (i < n) {
			bool good;
			if (n - i >= 4 * 3) {
				good = check<4>(vertices, indices.data() + i);
				i += 4 * 3;
			} else {
				good = check<1>(vertices, indices.data() + i);
				i += 1 * 3;
			}
			if (!good) {
				return false;
			}
		}
		return true;
	}

	void reserve(int n) {
		indices.reserve(n * 3);
	}

	void clear() {
		indices.clear();
	}
};

// fast, SIMD-able computation of areas
// (clang does a good job of SIMD-ing this)

class NonVanishingAreas {
	const float eps;
	uint8_t * const result;

public:
	inline NonVanishingAreas(
		uint8_t *result, float eps) :
		result(result), eps(eps) {
	}

	inline void operator()(int i, float area) const {
		result[i] = std::abs(area) > eps;
	}

	inline bool done() const {
		return false;
	}
};

class IsConvex {
	bool _convex;

public:
	inline IsConvex() : _convex(true) {
	}

	inline void operator()(int i, float area) {
		_convex &= area < 1e-2f;
	}

	inline operator bool() const {
		return _convex;
	}

	inline bool done() const {
		return !_convex;
	}
};

// SIMD-able.
template<typename R>
static void computeFromAreas(const vec2 * const v, const int n, R &r) {
	float ax = v[0].x, ay = v[0].y;
	float bx = v[1].x, by = v[1].y;

	float ABx = ax - bx;
	float ABy = ay - by;

	#pragma clang loop vectorize(enable) interleave(enable)
	for (int i = 0; i < n; i += 8) {
		for (int j = i; j < std::min(n, i + 8); j++) {
			const float cx = v[j + 2].x;
			const float cy = v[j + 2].y;

			const float BCx = bx - cx;
			const float BCy = by - cy;

			const float area = ABx * -BCy - ABy * -BCx;

			r(j, area);

			ABx = BCx;
			ABy = BCy;

			bx = cx;
			by = cy;
		}

		if (r.done()) {
			break;
		}
	}
}

END_TOVE_NAMESPACE

#endif // __TOVE_MESH_AREA
