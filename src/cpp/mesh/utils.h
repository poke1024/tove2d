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

inline float cross(float Ax, float Ay, float Bx, float By, float Cx, float Cy) {
    float BAx = Ax - Bx;
    float BAy = Ay - By;
    float BCx = Cx - Bx;
    float BCy = Cy - By;
    return (BAx * BCy - BAy * BCx);
}

inline bool unequal(float x1, float y1, float x2, float y2) {
	float dx = x1 - x2;
	float dy = y1 - y2;
	return dx * dx + dy * dy > 1e-5;
}

int find_unequal_backward(const float *vertices, int start, int n);
int find_unequal_forward(const float *vertices, int start, int n);
int find_unequal_forward(const float *vertices, const TPPLPoly &poly, int start, int n);
int find_unequal_forward(const float *vertices, const uint16_t *indices, int start, int n);

#endif // __TOVE_MESH_UTILS
