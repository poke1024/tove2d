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

#include "flatten.h"
#include <cmath>
#include "mesh.h"
#include "../utils.h"
#include "turtle.h"
#include "../path.h"
#include "../subpath.h"

BEGIN_TOVE_NAMESPACE

int toveMaxFlattenSubdivisions = 6;

inline double squareDistance(double x1, double y1, double x2, double y2) {
	double dx = x2 - x1;
	double dy = y2 - y1;
	return dx * dx + dy * dy;
}

ClipperParameters DefaultCurveFlattener::configure(float scale) {
	const float e = 1.0f / (resolution * scale);

	// note that clipper scale here also impacts the minimal possible
	// line width.
#if TOVE_TARGET == TOVE_TARGET_GODOT
	// for Godot, we want a high, static clipper scale. reason is that
	// individuals paths can be moved around in a floating point scale
	// _after_ the Clipper step happened.
	const float clipperScale = 65536.0f;
#else
	const float clipperScale = std::max(2.0f, 2.0f / e);
#endif

	const float eps = e * clipperScale;
	tolerance = eps * eps;
	return ClipperParameters{clipperScale, eps};
}

void DefaultCurveFlattener::flatten(
	float x1, float y1, float x2, float y2,
	float x3, float y3, float x4, float y4,
	ClipperPath &points, int level) const {

	if (level > recursionLimit) {
		points.push_back(ClipperPoint(x4, y4));
		return;
	}

	float ax = 3.0 * x2 - 2.0 * x1 - x4;
	float ay = 3.0 * y2 - 2.0 * y1 - y4;
	float bx = 3.0 * x3 - x1 - 2.0 * x4;
	float by = 3.0 * y3 - y1 - 2.0 * y4;
	float error = std::max(ax * ax, bx * bx) + std::max(ay * ay, by * by);

	if (error <= tolerance) {
		points.push_back(ClipperPoint(x4, y4));
		return;
	}

	float x12 = (x1 + x2) / 2;
	float y12 = (y1 + y2) / 2;
	float x23 = (x2 + x3) / 2;
	float y23 = (y2 + y3) / 2;
	float x34 = (x3 + x4) / 2;
	float y34 = (y3 + y4) / 2;
	float x123 = (x12 + x23) / 2;
	float y123 = (y12 + y23) / 2;
	float x234 = (x23 + x34) / 2;
	float y234 = (y23 + y34) / 2;
	float x1234 = (x123 + x234) / 2;
	float y1234 = (y123 + y234) / 2;

	flatten(x1, y1, x12, y12, x123, y123, x1234, y1234, points, level + 1);
	flatten(x1234, y1234, x234, y234, x34, y34, x4, y4, points, level + 1);
}

void AntiGrainFlattener::initialize(
	const ToveTesselationSettings &quality) {

	assert(quality.stopCriterion == TOVE_ANTIGRAIN);

	distanceTolerance = quality.antigrain.distanceTolerance;
	colinearityEpsilon = quality.antigrain.colinearityEpsilon;
	angleEpsilon = quality.antigrain.angleEpsilon;
	angleTolerance = quality.antigrain.angleTolerance;
	cuspLimit = quality.antigrain.cuspLimit;
	recursionLimit = std::min(
		toveMaxFlattenSubdivisions, quality.recursionLimit);

	distanceToleranceSquare = distanceTolerance * distanceTolerance;
}

void AntiGrainFlattener::flatten(
	float x1, float y1, float x2, float y2,
	float x3, float y3, float x4, float y4,
	ClipperPath &points, int level) const {

	// the following function is taken from Maxim Shemanarev's AntiGrain:
	// https://github.com/pelson/antigrain/blob/master/agg-2.4/src/agg_curves.cpp

	if (level > recursionLimit) {
		points.push_back(ClipperPoint(x4, y4));
		return;
	}

	// Calculate all the mid-points of the line segments
	//----------------------
	double x12   = (x1 + x2) / 2;
	double y12   = (y1 + y2) / 2;
	double x23   = (x2 + x3) / 2;
	double y23   = (y2 + y3) / 2;
	double x34   = (x3 + x4) / 2;
	double y34   = (y3 + y4) / 2;
	double x123  = (x12 + x23) / 2;
	double y123  = (y12 + y23) / 2;
	double x234  = (x23 + x34) / 2;
	double y234  = (y23 + y34) / 2;
	double x1234 = (x123 + x234) / 2;
	double y1234 = (y123 + y234) / 2;


	// Try to approximate the full cubic curve by a single straight line
	//------------------
	double dx = x4-x1;
	double dy = y4-y1;

	double d2 = fabs(((x2 - x4) * dy - (y2 - y4) * dx));
	double d3 = fabs(((x3 - x4) * dy - (y3 - y4) * dx));
	double da1, da2, k;

	switch((int(d2 > colinearityEpsilon) << 1) +
			int(d3 > colinearityEpsilon))
	{
	case 0:
		// All collinear OR p1==p4
		//----------------------
		k = dx*dx + dy*dy;
		if(k == 0)
		{
			d2 = squareDistance(x1, y1, x2, y2);
			d3 = squareDistance(x4, y4, x3, y3);
		}
		else
		{
			k   = 1 / k;
			da1 = x2 - x1;
			da2 = y2 - y1;
			d2  = k * (da1*dx + da2*dy);
			da1 = x3 - x1;
			da2 = y3 - y1;
			d3  = k * (da1*dx + da2*dy);
			if(d2 > 0 && d2 < 1 && d3 > 0 && d3 < 1)
			{
				// Simple collinear case, 1---2---3---4
				// We can leave just two endpoints
				return;
			}
				 if(d2 <= 0) d2 = squareDistance(x2, y2, x1, y1);
			else if(d2 >= 1) d2 = squareDistance(x2, y2, x4, y4);
			else             d2 = squareDistance(x2, y2, x1 + d2*dx, y1 + d2*dy);

				 if(d3 <= 0) d3 = squareDistance(x3, y3, x1, y1);
			else if(d3 >= 1) d3 = squareDistance(x3, y3, x4, y4);
			else             d3 = squareDistance(x3, y3, x1 + d3*dx, y1 + d3*dy);
		}
		if(d2 > d3)
		{
			if(d2 < distanceToleranceSquare)
			{
				points.push_back(ClipperPoint(x2, y2));
				return;
			}
		}
		else
		{
			if(d3 < distanceToleranceSquare)
			{
				points.push_back(ClipperPoint(x3, y3));
				return;
			}
		}
		break;

	case 1:
		// p1,p2,p4 are collinear, p3 is significant
		//----------------------
		if(d3 * d3 <= distanceToleranceSquare * (dx*dx + dy*dy))
		{
			if(angleTolerance < angleEpsilon)
			{
				points.push_back(ClipperPoint(x23, y23));
				return;
			}

			// Angle Condition
			//----------------------
			da1 = fabs(atan2(y4 - y3, x4 - x3) - atan2(y3 - y2, x3 - x2));
			if(da1 >= M_PI) da1 = 2*M_PI - da1;

			if(da1 < angleTolerance)
			{
				points.push_back(ClipperPoint(x2, y2));
				points.push_back(ClipperPoint(x3, y3));
				return;
			}

			if(cuspLimit != 0.0)
			{
				if(da1 > cuspLimit)
				{
					points.push_back(ClipperPoint(x3, y3));
					return;
				}
			}
		}
		break;

	case 2:
		// p1,p3,p4 are collinear, p2 is significant
		//----------------------
		if(d2 * d2 <= distanceToleranceSquare * (dx*dx + dy*dy))
		{
			if(angleTolerance < angleEpsilon)
			{
				points.push_back(ClipperPoint(x23, y23));
				return;
			}

			// Angle Condition
			//----------------------
			da1 = fabs(atan2(y3 - y2, x3 - x2) - atan2(y2 - y1, x2 - x1));
			if(da1 >= M_PI) da1 = 2*M_PI - da1;

			if(da1 < angleTolerance)
			{
				points.push_back(ClipperPoint(x2, y2));
				points.push_back(ClipperPoint(x3, y3));
				return;
			}

			if(cuspLimit != 0.0)
			{
				if(da1 > cuspLimit)
				{
					points.push_back(ClipperPoint(x2, y2));
					return;
				}
			}
		}
		break;

	case 3:
		// Regular case
		//-----------------
		if((d2 + d3)*(d2 + d3) <= distanceToleranceSquare * (dx*dx + dy*dy))
		{
			// If the curvature doesn't exceed the distance_tolerance value
			// we tend to finish subdivisions.
			//----------------------
			if(angleTolerance < angleEpsilon)
			{
				points.push_back(ClipperPoint(x23, y23));
				return;
			}

			// Angle & Cusp Condition
			//----------------------
			k   = atan2(y3 - y2, x3 - x2);
			da1 = fabs(k - atan2(y2 - y1, x2 - x1));
			da2 = fabs(atan2(y4 - y3, x4 - x3) - k);
			if(da1 >= M_PI) da1 = 2*M_PI - da1;
			if(da2 >= M_PI) da2 = 2*M_PI - da2;

			if(da1 + da2 < angleTolerance)
			{
				// Finally we can stop the recursion
				//----------------------
				points.push_back(ClipperPoint(x23, y23));
				return;
			}

			if(cuspLimit != 0.0)
			{
				if(da1 > cuspLimit)
				{
					points.push_back(ClipperPoint(x2, y2));
					return;
				}

				if(da2 > cuspLimit)
				{
					points.push_back(ClipperPoint(x3, y3));
					return;
				}
			}
		}
		break;
	}

	flatten(x1, y1, x12, y12, x123, y123, x1234, y1234, points, level + 1);
	flatten(x1234, y1234, x234, y234, x34, y34, x4, y4, points, level + 1);
}

ClipperPaths AbstractAdaptiveFlattener::computeDashes(
	const NSVGshape *shape, const ClipperPaths &lines) const {

	const int dashCount = shape->strokeDashCount;
	if (dashCount <= 0) {
		return lines;
	}

	ClipperPaths dashes;
	const float *dashArray = shape->strokeDashArray;

	float dashLength = 0.0;
	for (int i = 0; i < dashCount; i++) {
		dashLength += dashArray[i];
	}

	for (int i = 0; i < lines.size(); i++) {
		const ClipperPath &path = lines[i];
		if (path.size() < 2) {
			continue;
		}

		Turtle turtle(path, shape->strokeDashOffset, dashes);
		int dashIndex = 0;

		while (turtle.push(dashArray[dashIndex])) {
			turtle.toggle();
			dashIndex = (dashIndex + 1) % dashCount;
		}
	}

	return dashes;
}

inline ClipperLib::JoinType joinType(int t) {
	switch (t) {
		case NSVG_JOIN_MITER:
		default:
			return ClipperLib::jtMiter;
		case NSVG_JOIN_ROUND:
			return ClipperLib::jtRound;
		case NSVG_JOIN_BEVEL:
			return ClipperLib::jtSquare;
	}
}

inline ClipperLib::EndType endType(int t, bool closed) {
	if (closed) {
		return ClipperLib::etClosedLine;
	}
	switch (t) {
		case NSVG_CAP_BUTT:
			return ClipperLib::etOpenButt;
		case NSVG_CAP_ROUND:
		default:
			return ClipperLib::etOpenRound;
		case NSVG_CAP_SQUARE:
			return ClipperLib::etOpenSquare;
	}
}

void AbstractAdaptiveFlattener::flatten(
	const PathRef &path,
	Tesselation &tesselation) const {

	const int n = path->getNumSubpaths();
	bool closed = true;
	for (int i = 0; i < n; i++) {
		const auto subpath = path->getSubpath(i);
		tesselation.fill.push_back(flatten(subpath));
		closed = closed && subpath->isClosed();
	}

	NSVGshape * const shape = &path->nsvg;
	const ClipperLib::PolyFillType fillType = path->getClipperFillType();

	const bool hasStroke = shape->stroke.type != NSVG_PAINT_NONE &&
		shape->strokeWidth > 0.0f;

	ClipperPaths lines;
	if (hasStroke) {
		lines = computeDashes(shape, tesselation.fill);
	}

	ClipperLib::SimplifyPolygons(tesselation.fill, fillType);

	if (hasStroke) {
		float lineOffset = shape->strokeWidth * clipper.scale * 0.5f;
		if (lineOffset < 1.0f) {
			// scaled offsets < 1 will generate artefacts as the ClipperLib's
			// underlying integer resolution cannot handle them.
			lineOffset = 0.0f;
			tove::report::warn("ignoring line width < 2. please use setResolution().");
		}

		ClipperLib::ClipperOffset offset(
			shape->miterLimit, clipper.arcTolerance);
		offset.AddPaths(lines,
			joinType(shape->strokeLineJoin),
			endType(shape->strokeLineCap,
				closed && shape->strokeDashCount == 0));
		offset.Execute(tesselation.stroke, lineOffset);

		ClipperPaths stroke;
		ClipperLib::ClosedPathsFromPolyTree(tesselation.stroke, stroke);

		ClipperLib::Clipper clipper;
		clipper.AddPaths(tesselation.fill, ClipperLib::ptSubject, true);
		clipper.AddPaths(stroke, ClipperLib::ptClip, true);
		clipper.Execute(ClipperLib::ctDifference, tesselation.fill);
	}
}



int RigidFlattener::flatten(
	const Vertices &vertices,
	int index, int level,
	float x1, float y1, float x2, float y2,
	float x3, float y3, float x4, float y4) const
{
	if (level >= _depth) {
		auto &v = vertices[index++];
		if (_offset != 0.0) {
			float dx = x4 - x1;
			float dy = y4 - y1;
			float s = _offset / sqrt(dx * dx + dy * dy);
			v.x = x4 - s * dy;
			v.y = y4 + s * dx;
		} else {
			v.x = x4;
			v.y = y4;
		}
		return index;
	}

	float x12,y12,x23,y23,x34,y34,x123,y123,x234,y234,x1234,y1234;

	x12 = (x1+x2)*0.5f;
	y12 = (y1+y2)*0.5f;
	x23 = (x2+x3)*0.5f;
	y23 = (y2+y3)*0.5f;
	x34 = (x3+x4)*0.5f;
	y34 = (y3+y4)*0.5f;
	x123 = (x12+x23)*0.5f;
	y123 = (y12+y23)*0.5f;

	x234 = (x23+x34)*0.5f;
	y234 = (y23+y34)*0.5f;
	x1234 = (x123+x234)*0.5f;
	y1234 = (y123+y234)*0.5f;

	index = flatten(vertices, index, level+1,
		x1,y1, x12,y12, x123,y123, x1234,y1234);
	index = flatten(vertices, index, level+1,
		x1234,y1234, x234,y234, x34,y34, x4,y4);
	return index;
}


int RigidFlattener::size(const SubpathRef &subpath) const {
	const NSVGpath *path = &subpath->nsvg;
	const int npts = path->npts;
	const int n = ncurves(npts);
	const int verticesPerCurve = (1 << _depth);
	return 1 + n * verticesPerCurve;
}

int RigidFlattener::flatten(
	const SubpathRef &subpath, const MeshRef &mesh, int index) const {

	const NSVGpath *path = &subpath->nsvg;
	const int npts = path->npts;

	if (npts < 3) {
		return 0;
	}

	const int n = ncurves(npts);

	const int verticesPerCurve = (1 << _depth);
	const int numVertices = 1 + n * verticesPerCurve;
	const auto vertices = mesh->vertices(index, numVertices);

	vertices[0].x = path->pts[0];
	vertices[0].y = path->pts[1];

	int v = 1;
	int k = 0;
	for (int i = 0; i < n; i++) {
		const float *p = &path->pts[k * 2];
		k += 3;

		const int v0 = v;
		v = flatten(vertices, v, 0,
			p[0], p[1], p[2], p[3], p[4], p[5], p[6], p[7]);
		assert(v - v0 == verticesPerCurve);
	}

	assert(v == numVertices);
	return numVertices;
}

END_TOVE_NAMESPACE
