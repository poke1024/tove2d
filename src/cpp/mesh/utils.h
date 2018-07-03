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

#include "utils.h"
#include "../../thirdparty/polypartition.h"

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

	vec2 &operator[](size_t i) const {
		return *reinterpret_cast<vec2*>(
            reinterpret_cast<uint8_t*>(vertices) + (i * stride)
        );
	}

	vec2* operator->() const {
		return reinterpret_cast<vec2*>(vertices);
	}

    inline uint8_t *attr() const {
        return reinterpret_cast<uint8_t*>(vertices) + sizeof(vec2);
    }

	Vertices& operator++() { // prefix
		vertices = reinterpret_cast<uint8_t*>(vertices) + stride;
		return *this;
	}

	Vertices operator++(int) { // postfix
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

int find_unequal_backward(
    const Vertices &vertices, int start, int n);
int find_unequal_forward(
    const Vertices &vertices, int start, int n);
int find_unequal_forward(
    const Vertices &vertices, const TPPLPoly &poly, int start, int n);
int find_unequal_forward(
    const Vertices &vertices, const uint16_t *indices, int start, int n);

#endif // __TOVE_MESH_UTILS
