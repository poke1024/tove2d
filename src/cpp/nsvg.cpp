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

void scaleGradient(NSVGgradient* grad, float tx, float ty, float sx, float sy) {
	nsvg__scaleGradient(grad, tx, ty, sx, sy);
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

} // namespace nsvg
