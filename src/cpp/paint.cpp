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

#include "paint.h"
#include "path.h"


void AbstractPaint::changed() {
	if (claimer) {
		claimer->colorChanged(this);
	}
}

void AbstractGradient::sort() const {
	if (!sorted) {
		std::sort(nsvg->stops, nsvg->stops + nsvg->nstops,
			[] (const NSVGgradientStop &a, const NSVGgradientStop &b) {
				return a.offset < b.offset;
			});
		sorted = true;
	}
}

NSVGgradient *AbstractGradient::getInverseNSVGgradient() {
	const size_t size = getRecordSize(nsvg->nstops);
	nsvgInverse = static_cast<NSVGgradient*>(realloc(nsvgInverse, size));
	std::memcpy(nsvgInverse, nsvg, size);
	std::memcpy(nsvgInverse->xform, xformInverse, 6 * sizeof(float));
	return nsvgInverse;
}

AbstractGradient::AbstractGradient(int nstops) :
	nsvgInverse(nullptr) {

	const size_t size = getRecordSize(nstops);
	nsvg = static_cast<NSVGgradient*>(malloc(size));
	if (!nsvg) {
		throw std::bad_alloc();
	}
	nsvg->nstops = nstops;

	nsvg::xformIdentity(nsvg->xform);
	nsvg::xformIdentity(xformInverse);
	nsvg->spread = 0;
	nsvg->fx = 0;
	nsvg->fy = 0;
	sorted = true;
}

AbstractGradient::AbstractGradient(const NSVGgradient *gradient) :
	nsvgInverse(nullptr) {

	const size_t size = getRecordSize(gradient->nstops);
	nsvg = static_cast<NSVGgradient*>(malloc(size));
	if (!nsvg) {
		throw std::bad_alloc();
	}
	std::memcpy(nsvg, gradient, size);

	std::memcpy(xformInverse, gradient->xform, 6 * sizeof(float));
	nsvg::xformInverse(nsvg->xform, xformInverse);

	sorted = true;
}

AbstractGradient::AbstractGradient(const AbstractGradient &gradient) :
	nsvgInverse(nullptr) {

	const size_t size = getRecordSize(gradient.nsvg->nstops);
	nsvg = static_cast<NSVGgradient*>(malloc(size));
	if (!nsvg) {
		throw std::bad_alloc();
	}
	std::memcpy(nsvg, gradient.nsvg, size);
	std::memcpy(xformInverse, gradient.xformInverse, 6 * sizeof(float));
	sorted = gradient.sorted;
}

void AbstractGradient::transform(float sx, float sy, float tx, float ty) {
	nsvg::scaleGradient(nsvg, tx, ty, sx, sy);
	nsvg::xformInverse(xformInverse, nsvg->xform);
	changed();
}


LinearGradient::LinearGradient(float x1, float y1, float x2, float y2) :
	AbstractGradient(0) {

	float *xform = nsvg->xform;
	// adapted from NanoSVG:
	const float dx = x2 - x1;
	const float dy = y2 - y1;
	xform[0] = dy; xform[1] = -dx;
	xform[2] = dx; xform[3] = dy;
	xform[4] = x1; xform[5] = y1;
	nsvg::xformInverse(xformInverse, xform);
}

RadialGradient::RadialGradient(float cx, float cy, float fx, float fy, float r) :
	AbstractGradient(0) {

	float *xform = nsvg->xform;
	// adapted from NanoSVG:
	xform[0] = r; xform[1] = 0;
	xform[2] = 0; xform[3] = r;
	xform[4] = cx; xform[5] = cy;
	nsvg->fx = fx / r;
	nsvg->fy = fy / r;
	nsvg::xformInverse(xformInverse, xform);
}

PaintRef NSVGpaintToPaint(const NSVGpaint &paint) {
	switch (paint.type) {
		case NSVG_PAINT_NONE: {
			return PaintRef();
		} break;
		case NSVG_PAINT_COLOR: {
			return std::make_shared<Color>(
				(paint.color & 0xff) / 255.0,
				((paint.color >> 8) & 0xff) / 255.0,
				((paint.color >> 16) & 0xff) / 255.0,
				((paint.color >> 24) & 0xff) / 255.0);
		} break;
		case NSVG_PAINT_LINEAR_GRADIENT: {
			return std::make_shared<LinearGradient>(paint.gradient);
		} break;
		case NSVG_PAINT_RADIAL_GRADIENT: {
			return std::make_shared<RadialGradient>(paint.gradient);
		} break;
	}
	throw std::invalid_argument("invalid nsvg paint");
}
