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

#define NANOSVG_IMPLEMENTATION
#define NANOSVG_ALL_COLOR_KEYWORDS
#include "../thirdparty/nanosvg.h"

#define NANOSVGRAST_IMPLEMENTATION
#include "../thirdparty/nanosvgrast.h"

namespace nsvg {

thread_local NSVGparser *_parser = nullptr;
thread_local NSVGrasterizer *rasterizer = nullptr;

static NSVGparser *getNSVGparser() {
	if (!_parser) {
		_parser = nsvg__createParser();
		if (!_parser) {
			throw std::bad_alloc();
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
		throw std::bad_alloc();
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

uint8_t *rasterize(NSVGimage *image, float tx, float ty, float scale,
	int width, int height, const ToveTesselationQuality *quality) {

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

	if (quality && quality->adaptive.valid) {
		rasterizer->tessTol = quality->adaptive.angleTolerance;
		rasterizer->distTol = quality->adaptive.distanceTolerance;
	} else {
		rasterizer->tessTol = defaultTessTol;
		rasterizer->distTol = defaultDistTol;
	}

	uint8_t *pixels = static_cast<uint8_t*>(malloc(width * height * 4));
	if (pixels) {
		nsvgRasterize(rasterizer, image, tx, ty, scale, pixels, width, height, width * 4);
	}

	return pixels;
}

Transform::Transform() {
	identity = false;
	scaleLineWidth = false;

	nsvg__xformIdentity(matrix);
}

Transform::Transform(
	float tx, float ty,
	float r,
	float sx, float sy,
	float ox, float oy,
	float kx, float ky) {

	identity = false;
	scaleLineWidth = false;

	// adapted from love2d:
	const float c = cosf(r);
	const float s = sinf(r);
	// matrix multiplication carried out on paper:
	// |1    x| |c -s  | |sx     | | 1 ky  | |1   -ox|
	// |  1  y| |s  c  | |   sy  | |kx  1  | |  1 -oy|
	// |     1| |     1| |      1| |      1| |     1 |
	//   move    rotate    scale     skew      origin
	matrix[0] = c * sx - ky * s * sy; // = a
	matrix[1] = s * sx + ky * c * sy; // = b
	matrix[2] = kx * c * sx - s * sy; // = c
	matrix[3] = kx * s * sx + c * sy; // = d
	matrix[4] = tx - ox * matrix[0] - oy * matrix[2];
	matrix[5] = ty - ox * matrix[1] - oy * matrix[3];

	nsvg__xformInverse(inverse, matrix);
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

} // namespace nsvg
