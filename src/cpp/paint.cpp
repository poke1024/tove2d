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
#include <algorithm>

BEGIN_TOVE_NAMESPACE

void copyColor(ToveRGBA &rgba, uint32_t color, float opacity) {
	uint32_t c;
	if (opacity < 1.0f) {
		c = nsvg::applyOpacity(color, opacity);
	} else {
		c = color;
	}
	rgba.r = (c & 0xff) / 255.0;
	rgba.g = ((c >> 8) & 0xff) / 255.0;
	rgba.b = ((c >> 16) & 0xff) / 255.0;
	rgba.a = ((c >> 24) & 0xff) / 255.0;
}


void AbstractPaint::changed() {
	if (claimer) {
		claimer->colorChanged(this);
	}
}


void Color::getGradientParameters(ToveGradientParameters &p) {
}

void Color::cloneTo(PaintRef &target, const nsvg::Transform &transform) {
	if (target && target->getType() == PAINT_SOLID) {
		Color *t = static_cast<Color*>(target.get());
		if (t->color != color) {
			t->color = color;
			t->changed();
		}
	} else {
		target = clone();
	}
}

void Color::store(NSVGpaint &paint) {
	paint.type = NSVG_PAINT_COLOR;
	paint.color = color;
}


void AbstractGradient::sort() const {
	std::sort(nsvg->stops, nsvg->stops + nsvg->nstops,
		[] (const NSVGgradientStop &a, const NSVGgradientStop &b) {
			return a.offset < b.offset;
		});
	sorted = true;
}

NSVGgradient *AbstractGradient::getInverseNSVGgradient() {
	const size_t size = getRecordSize(nsvg->nstops);
	nsvgInverse = static_cast<NSVGgradient*>(realloc(nsvgInverse, size));
	if (!nsvgInverse) {
		TOVE_BAD_ALLOC();
		return nullptr;
	}
	std::memcpy(nsvgInverse, nsvg, size);
	std::memcpy(nsvgInverse->xform, xformInverse, 6 * sizeof(float));
	return nsvgInverse;
}

AbstractGradient::AbstractGradient(int nstops) :
	nsvgInverse(nullptr) {

	const size_t size = getRecordSize(nstops);
	nsvg = static_cast<NSVGgradient*>(malloc(size));
	if (!nsvg) {
		TOVE_BAD_ALLOC();
		return;
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
		TOVE_BAD_ALLOC();
		return;
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
		TOVE_BAD_ALLOC();
		return;
	}
	std::memcpy(nsvg, gradient.nsvg, size);
	std::memcpy(xformInverse, gradient.xformInverse, 6 * sizeof(float));
	sorted = gradient.sorted;
}

void AbstractGradient::set(const AbstractGradient *source) {
	const size_t size = getRecordSize(source->nsvg->nstops);

	if (nsvg->nstops == source->nsvg->nstops) {
		ensureSort();
		source->ensureSort();
		if (std::memcmp(nsvg, source->nsvg, size) == 0) {
			return;
		}
	}

	nsvg = static_cast<NSVGgradient*>(realloc(nsvg, nextpow2(size)));
	if (!nsvg) {
		TOVE_BAD_ALLOC();
		return;
	}
	std::memcpy(nsvg, source->nsvg, size);
	sorted = source->sorted;

	std::memcpy(xformInverse, source->xformInverse, 6 * sizeof(float));

	changed();
}

void AbstractGradient::transform(const nsvg::Transform &transform) {
	transform.transformGradient(nsvg);
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

void LinearGradient::getGradientParameters(ToveGradientParameters &p) {
	const float *xform = nsvg->xform;
	p.values[0] = xform[4];
	p.values[1] = xform[5];
	p.values[2] = xform[4] + xform[2];
	p.values[3] = xform[5] + xform[3];
}

void LinearGradient::cloneTo(PaintRef &target, const nsvg::Transform &transform) {
	if (target && target->getType() == PAINT_LINEAR_GRADIENT) {
		static_cast<LinearGradient*>(target.get())->set(this);
	} else {
		target = clone();
	}
	target->transform(transform);
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

void RadialGradient::getGradientParameters(ToveGradientParameters &p) {
	const float *xform = nsvg->xform;
	p.values[0] = xform[4];
	p.values[1] = xform[5];
	p.values[2] = nsvg->fx * xform[0];
	p.values[3] = nsvg->fy * xform[0];
	p.values[4] = xform[0];
}

void RadialGradient::cloneTo(PaintRef &target, const nsvg::Transform &transform) {
	if (target && target->getType() == PAINT_RADIAL_GRADIENT) {
		static_cast<RadialGradient*>(target.get())->set(this);
	} else {
		target = clone();
	}
	target->transform(transform);
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
	TOVE_WARN("Invalid nsvg paint type.");
	return PaintRef();
}

END_TOVE_NAMESPACE
