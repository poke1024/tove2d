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
#include "../subpath.h"
#include "mesh.h"

BEGIN_TOVE_NAMESPACE

extern int toveMaxFlattenSubdivisions;

struct Tesselation {
	ClipperLib::Paths fill;
	ClipperLib::PolyTree stroke;
};

class AntiGrainFlattener {
private:
	float colinearityEpsilon;
	float distanceTolerance;
	float angleEpsilon;
	float angleTolerance;
	float cuspLimit;
	int recursionLimit;
	float distanceToleranceSquare;
	float clipperScale;

public:
	void initialize(
		const ToveTesselationSettings &quality);

	float configure(float extent);

	inline void flatten(
		float x1, float y1, float x2, float y2,
		float x3, float y3, float x4, float y4,
		ClipperPath &points, int level) const;
};

struct ClipperParameters {
	float scale;
	float arcTolerance;
};

class DefaultCurveFlattener {
private:
	const float resolution;
	const int recursionLimit;
	float tolerance;

public:
	inline DefaultCurveFlattener(const DefaultCurveFlattener& r) :
		resolution(r.resolution),
		recursionLimit(r.recursionLimit),
		tolerance(0.0f) {
	}

	DefaultCurveFlattener(
		float resolution,
		int recursionLimit) :
		resolution(resolution),
		recursionLimit(std::min(toveMaxFlattenSubdivisions, recursionLimit)),
		tolerance(0.0f) {
	}

	ClipperParameters configure(float extent);

	void flatten(
		float x1, float y1, float x2, float y2,
		float x3, float y3, float x4, float y4,
		ClipperPath &points, int level) const;
};

class AbstractAdaptiveFlattener {
private:
	void flatten(
		float x1, float y1, float x2, float y2,
		float x3, float y3, float x4, float y4,
		ClipperPath &points) const;

	virtual ClipperPath flatten(const SubpathRef &subpath) const = 0;

	ClipperPaths computeDashes(
		const NSVGshape *shape, const ClipperPaths &lines) const;

protected:
	ClipperParameters clipper;

public:
	virtual void configure(float extent) = 0;

	inline float getClipperScale() const {
		return clipper.scale;
	}

	void flatten(
		const PathRef &path,
		Tesselation &tesselation) const;

	virtual ~AbstractAdaptiveFlattener() {
	}
};

template<typename CurveFlattener>
class AdaptiveFlattener : public AbstractAdaptiveFlattener {
private:
	CurveFlattener curveFlattener;

protected:
	inline void flatten(
		float x1, float y1, float x2, float y2,
		float x3, float y3, float x4, float y4,
		ClipperPath &points) const {

		points.push_back(ClipperPoint(x1, y1));
		curveFlattener.flatten(x1, y1, x2, y2, x3, y3, x4, y4, points, 0);
		points.push_back(ClipperPoint(x4, y4));
	}

	ClipperPath flatten(const SubpathRef &subpath) const {
		NSVGpath *path = &subpath->nsvg;
		ClipperPath result;

		if (path->npts < 1) {
			return result;
		}

		const float scale = clipper.scale;

		const ClipperPoint p0 = ClipperPoint(
			path->pts[0] * scale, path->pts[1] * scale);
		result.push_back(p0);

		for (int i = 0; i * 2 + 7 < path->npts * 2; i += 3) {
			const float *p = &path->pts[i * 2];

			flatten(
				p[0] * scale, p[1] * scale,
				p[2] * scale, p[3] * scale,
				p[4] * scale, p[5] * scale,
				p[6] * scale, p[7] * scale,
				result);
		}

		if (path->closed) {
			result.push_back(p0);
		}

		return result;
	}
	
public:
	virtual void configure(float extent) {
		clipper = curveFlattener.configure(extent);
	}

	AdaptiveFlattener(const CurveFlattener &curveFlattener) :
		curveFlattener(curveFlattener) {
	}
};

class RigidFlattener {
private:
	const int _depth;
	const float _offset;

	int flatten(
		const Vertices &vertices,
		int index, int level,
		float x1, float y1, float x2, float y2,
		float x3, float y3, float x4, float y4) const;

public:
	int size(const SubpathRef &subpath) const;
	int flatten(const SubpathRef &subpath, const MeshRef &mesh, int index) const;

	inline RigidFlattener(int subdivisions, float offset) :
		_depth(std::min(toveMaxFlattenSubdivisions, subdivisions)),
		_offset(offset) {
	}
};

END_TOVE_NAMESPACE

#endif // __TOVE_MESH_FLATTEN
