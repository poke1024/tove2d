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

#ifndef __TOVE_MESH_FLATTEN
#define __TOVE_MESH_FLATTEN 1

#include "../common.h"
#include "mesh.h"

struct Tesselation {
	ClipperLib::Paths fill;
	ClipperLib::PolyTree stroke;
};

class AdaptiveFlattener {
private:
	float colinearityEpsilon;
	float distanceTolerance;
	float angleEpsilon;
	float angleTolerance;
	float cuspLimit;
	int recursionLimit;

	double distanceToleranceSquare;

	void flatten(
		float x1, float y1, float x2, float y2,
		float x3, float y3, float x4, float y4,
		ClipperPath &points) const;

	void recursive(
		float x1, float y1, float x2, float y2,
		float x3, float y3, float x4, float y4,
		ClipperPath &points, int level) const;

	ClipperPath flatten(const SubpathRef &subpath) const;

	inline double squareDistance(double x1, double y1, double x2, double y2) const {
        double dx = x2-x1;
        double dy = y2-y1;
        return dx * dx + dy * dy;
    }

	ClipperPaths computeDashes(const NSVGshape *shape, const ClipperPaths &lines) const;

public:
	const float scale;

	AdaptiveFlattener(float scale, const ToveTesselationQuality *quality);

	void operator()(const PathRef &path, Tesselation &tesselation) const;
};

class FixedFlattener {
private:
	const int _depth;
	const float _offset;

	int flatten(
		const Vertices &vertices,
		int index, int level,
		float x1, float y1, float x2, float y2,
		float x3, float y3, float x4, float y4) const;

public:
	FixedFlattener(int depth, float offset) : _depth(depth), _offset(offset) {
	}

	int size(const SubpathRef &subpath) const;
	int flatten(const SubpathRef &subpath, const MeshRef &mesh, int index) const;
};

#endif // __TOVE_MESH_FLATTEN
