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

#include "nsvg.h"
#include "utils.h"

BEGIN_TOVE_NAMESPACE

#define NANOSVG_IMPLEMENTATION
#define NANOSVG_ALL_COLOR_KEYWORDS
#include "../thirdparty/nanosvg.h"

#define NANOSVGRAST_IMPLEMENTATION
#include "../thirdparty/nanosvgrast.h"

namespace nsvg {

thread_local NSVGparser *_parser = nullptr;
thread_local NSVGrasterizer *rasterizer = nullptr;

NSVGrasterizer *getRasterizer(const ToveTesselationQuality *quality) {
	static float defaultTessTol;
	static float defaultDistTol;

	if (!rasterizer) {
		rasterizer = nsvgCreateRasterizer();
		if (!rasterizer) {
			return nullptr;
		}
		// skipping nsvgDeleteRasterizer(rasterizer)

		defaultTessTol = rasterizer->tessTol;
		defaultDistTol = rasterizer->distTol;
	}

	if (quality && quality->stopCriterion == TOVE_MAX_ERROR) {
		rasterizer->tessTol = quality->maximumError;
		rasterizer->distTol = quality->maximumError;
	} else {
		rasterizer->tessTol = defaultTessTol;
		rasterizer->distTol = defaultDistTol;
	}

	rasterizer->nedges = 0;
	rasterizer->npoints = 0;
	rasterizer->npoints2 = 0;

	return rasterizer;
}

static NSVGparser *getNSVGparser() {
	if (!_parser) {
		_parser = nsvg__createParser();
		if (!_parser) {
			TOVE_BAD_ALLOC();
			return nullptr;
		}
	} else {
		nsvg__resetPath(_parser);
	}
	return _parser;
}

uint32_t makeColor(float r, float g, float b, float a) {
	return nsvg__RGBA(
		clamp(r, 0, 1) * 255.0,
		clamp(g, 0, 1) * 255.0,
		clamp(b, 0, 1) * 255.0,
		clamp(a, 0, 1) * 255.0);
}

uint32_t applyOpacity(uint32_t color, float opacity) {
	return nsvg__applyOpacity(color, opacity);
}

int maxDashes() {
	return NSVG_MAX_DASHES;
}

void curveBounds(float *bounds, float *curve) {
	nsvg__curveBounds(bounds, curve);
}

void xformInverse(float *a, float *b) {
	nsvg__xformInverse(a, b);
}

void xformIdentity(float *m) {
	nsvg__xformIdentity(m);
}

NSVGimage *parsePath(const char *d) {
	NSVGparser *parser = getNSVGparser();

	const char *attr[3] = {"d", d, nullptr};
	nsvg__parsePath(parser, attr);

	NSVGimage *image = parser->image;

	parser->image = static_cast<NSVGimage*>(malloc(sizeof(NSVGimage)));
	if (!parser->image) {
		TOVE_BAD_ALLOC();
		return nullptr;
	}
	memset(parser->image, 0, sizeof(NSVGimage));

	return image;
}

float *pathArcTo(float *cpx, float *cpy, float *args, int &npts) {
	NSVGparser *parser = getNSVGparser();
	nsvg__pathArcTo(parser, cpx, cpy, args, 0);
	npts = parser->npts;
	return parser->pts;
}

void CachedPaint::init(const NSVGpaint &paint, float opacity) {
	NSVGcachedPaint cache;
	nsvg__initPaint(&cache, const_cast<NSVGpaint*>(&paint), opacity);

	type = cache.type;
	spread = cache.spread;
	for (int i = 0; i < 6; i++) {
		xform[i] = cache.xform[i];
	}

	const int n = numColors;
	uint8_t *storage = reinterpret_cast<uint8_t*>(colors);
	const int row = rowBytes;

	for (int i = 0; i < n; i++) {
		*reinterpret_cast<uint32_t*>(storage) = cache.colors[i];
		storage += row;
	}
}

bool shapeStrokeBounds(float *bounds, const NSVGshape *shape,
	float scale, const ToveTesselationQuality *quality) {

	// computing the shape stroke bounds is not trivial as miters
	// might take varying amounts of space.

	NSVGrasterizer *rasterizer = getRasterizer(quality);
	nsvg__flattenShapeStroke(
		rasterizer, const_cast<NSVGshape*>(shape), scale);

	const int n = rasterizer->nedges;
	if (n < 1) {
		return false;
	} else {
		const NSVGedge *edges = rasterizer->edges;

		float bx0 = edges->x0, by0 = edges->y0;
		float bx1 = bx0, by1 = by0;

 		for (int i = 0; i < n; i++) {
			const float x0 = edges->x0;
			const float y0 = edges->y0;
			const float x1 = edges->x1;
			const float y1 = edges->y1;
			edges++;

			bx0 = std::min(bx0, x0);
			bx0 = std::min(bx0, x1);

			by0 = std::min(by0, y0);
			by0 = std::min(by0, y1);

			bx1 = std::max(bx1, x0);
			bx1 = std::max(bx1, x1);

			by1 = std::max(by1, y0);
			by1 = std::max(by1, y1);
		}

		bounds[0] = bx0;
		bounds[1] = by0;
		bounds[2] = bx1;
		bounds[3] = by1;
		return true;
	}
}

void rasterize(NSVGimage *image, float tx, float ty, float scale,
	uint8_t* pixels, int width, int height, int stride,
	const ToveTesselationQuality *quality) {

	NSVGrasterizer *rasterizer = getRasterizer(quality);

	nsvgRasterize(rasterizer, image, tx, ty, scale,
			pixels, width, height, stride);
}

Transform::Transform() {
	identity = true;
	scaleLineWidth = false;

	nsvg__xformIdentity(matrix);
}

Transform::Transform(
	float a, float b, float c,
	float d, float e, float f) {

	identity = false;
	scaleLineWidth = false;

	matrix[0] = a;
	matrix[1] = d;
	matrix[2] = b;
	matrix[3] = e;
	matrix[4] = c;
	matrix[5] = f;
}

void Transform::multiply(const Transform &t) {
	nsvg__xformMultiply(matrix, const_cast<float*>(&t.matrix[0]));
}

void Transform::transformGradient(NSVGgradient* grad) const {
	nsvg__xformMultiply(grad->xform, const_cast<float*>(&matrix[0]));
}

void Transform::transformPoints(float *pts, const float *srcpts, int npts) const {
	if (identity) {
		memcpy(pts, srcpts, 2 * sizeof(float) * npts);
	} else {
		const float t0 = matrix[0];
		const float t1 = matrix[1];
		const float t2 = matrix[2];
		const float t3 = matrix[3];
		const float t4 = matrix[4];
		const float t5 = matrix[5];

		for (int i = 0; i < npts; i++) {
			const float x = srcpts[2 * i + 0];
			const float y = srcpts[2 * i + 1];
			pts[2 * i + 0] = x * t0 + y * t2 + t4;
			pts[2 * i + 1] = x * t1 + y * t3 + t5;
		}
	}
}

float Transform::getScale() const {
	float x = matrix[0] + matrix[2];
	float y = matrix[1] + matrix[3];
	return std::sqrt(x * x + y * y);
}

NSVGlineJoin nsvgLineJoin(ToveLineJoin join) {
	switch (join) {
		case TOVE_LINEJOIN_MITER:
			return NSVG_JOIN_MITER;
		case TOVE_LINEJOIN_ROUND:
			return NSVG_JOIN_ROUND;
		case TOVE_LINEJOIN_BEVEL:
			return NSVG_JOIN_BEVEL;
	}
	return NSVG_JOIN_MITER;
}

ToveLineJoin toveLineJoin(NSVGlineJoin join) {
	switch (join) {
		case NSVG_JOIN_MITER:
			return TOVE_LINEJOIN_MITER;
		case NSVG_JOIN_ROUND:
			return TOVE_LINEJOIN_ROUND;
		case NSVG_JOIN_BEVEL:
			return TOVE_LINEJOIN_BEVEL;
	}
	return TOVE_LINEJOIN_MITER;
}
} // namespace nsvg

END_TOVE_NAMESPACE
