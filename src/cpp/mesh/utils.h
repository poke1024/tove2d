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

#ifndef __TOVE_MESH_UTILS
#define __TOVE_MESH_UTILS 1

#include "../common.h"
#include "../../thirdparty/polypartition.h"

BEGIN_TOVE_NAMESPACE

struct vec2 {
	float x;
	float y;
};

class Vertices {
private:
    void *vertices;
    const uint16_t stride;

public:
	inline Vertices(void *vertices, uint16_t stride, size_t i0 = 0) :
        vertices(reinterpret_cast<uint8_t*>(vertices) + (i0 * stride)),
        stride(stride) {
	}

	inline Vertices(const Vertices &v) :
        vertices(v.vertices),
        stride(v.stride) {
	}

	inline vec2 &operator[](size_t i) const {
		return *reinterpret_cast<vec2*>(
            reinterpret_cast<uint8_t*>(vertices) + (i * stride)
        );
	}

	inline vec2* operator->() const {
		return reinterpret_cast<vec2*>(vertices);
	}

    inline uint8_t *attr() const {
        return reinterpret_cast<uint8_t*>(vertices) + sizeof(vec2);
    }

	inline Vertices& operator++() { // prefix
		vertices = reinterpret_cast<uint8_t*>(vertices) + stride;
		return *this;
	}

	inline Vertices operator++(int) { // postfix
		Vertices saved(*this);
		operator++();
		return saved;
	}
};

inline float cross(
    float Ax, float Ay, float Bx, float By, float Cx, float Cy) {
    float BAx = Ax - Bx;
    float BAy = Ay - By;
    float BCx = Cx - Bx;
    float BCy = Cy - By;
    return (BAx * BCy - BAy * BCx);
}

inline bool unequal(
    float x1, float y1, float x2, float y2) {
	float dx = x1 - x2;
	float dy = y1 - y2;
	return dx * dx + dy * dy > 1e-5;
}

inline int find_unequal_backward(
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

inline int find_unequal_forward(
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

inline int find_unequal_forward(
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

inline int find_unequal_forward(
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

END_TOVE_NAMESPACE

#endif // __TOVE_MESH_UTILS
