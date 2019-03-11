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

static void extractColor(ToveRGBA &rgba, uint32_t c) {
	rgba.r = (c & 0xff) / 255.0;
	rgba.g = ((c >> 8) & 0xff) / 255.0;
	rgba.b = ((c >> 16) & 0xff) / 255.0;
	rgba.a = ((c >> 24) & 0xff) / 255.0;
}

static void extractColor(ToveRGBA &rgba, uint32_t color, float opacity) {
	uint32_t c;
	if (opacity < 1.0f) {
		c = nsvg::applyOpacity(color, opacity);
	} else {
		c = color;
	}
	extractColor(rgba, c);
}

static uint32_t lerpColor(uint32_t c1, uint32_t c2, float p_t) {
	const int32_t t = int32_t(p_t * 0x100);
	const int32_t s = 0x100 - t;
	return
		(((c1 & 0xff) * s + (c2 & 0xff) * t) >> 8) |
		(((((c1 >> 8) & 0xff) * s + ((c2 >> 8) & 0xff) * t) >> 8) << 8) |
		(((((c1 >> 16) & 0xff) * s + ((c2 >> 16) & 0xff) * t) >> 8) << 16) |
		(((((c1 >> 24) & 0xff) * s + ((c2 >> 24) & 0xff) * t) >> 8) << 24);
}


void AbstractPaint::changed() {
	broadcastChange(CHANGED_COLORS);
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

void Color::getRGBA(ToveRGBA &rgba, float opacity) const {
	extractColor(rgba, color, opacity);
}

bool Color::animate(const PaintRef &a, const PaintRef &b, float t) {
	if (a->getType() == NSVG_PAINT_COLOR &&
		b->getType() == NSVG_PAINT_COLOR) {
		
		color = lerpColor(
			static_cast<Color*>(a.get())->color,
			static_cast<Color*>(b.get())->color,
			t
		);
	
		changed();
		return true;
	} else {
		return false;
	}
}

#if TOVE_DEBUG
std::ostream &Color::dump(std::ostream &os) {
	os << tove::debug::color(color);
	return os;
}
#endif


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

void AbstractGradient::setNumColorStops(int numStops) {
	const size_t size = getRecordSize(numStops);
	nsvg = static_cast<NSVGgradient*>(realloc(nsvg, nextpow2(size)));
	if (!nsvg) {
		throw std::bad_alloc();
	}
	nsvg->nstops = numStops;

}

void AbstractGradient::addColorStop(float offset, float r, float g, float b, float a) {
	setNumColorStops(nsvg->nstops + 1);
	setColorStop(nsvg->nstops - 1, offset, r, g, b, a);
	sorted = false;
}

float AbstractGradient::getColorStop(int i, ToveRGBA &rgba, float opacity) {
	if (i >= 0 && i < nsvg->nstops) {
		extractColor(rgba, nsvg->stops[i].color, opacity);
		return nsvg->stops[i].offset;
	} else {
		return 0.0f;
	}
}

bool AbstractGradient::animate(const PaintRef &a, const PaintRef &b, float t) {
	const TovePaintType type = getType();
	if (a->getType() != type || b->getType() != type) {
		return false; // cannot animate
	}

	const auto *gradA = static_cast<AbstractGradient*>(a.get());
	const auto *gradB = static_cast<AbstractGradient*>(b.get());

	const auto *nsvgA = gradA->nsvg;
	const auto *nsvgB = gradB->nsvg;

	const int nstops = gradA->nsvg->nstops;
	if (nstops != gradB->nsvg->nstops) {
		return false; // cannot animate
	}

	if (nsvg->nstops != nstops) {
		setNumColorStops(nstops);
	}

	const float s = 1.0f - t;

	float *xform = nsvg->xform;
	bool xformIsAnimated = false;
	for (int i = 0; i < 6; i++) {
		const float xformA = nsvgA->xform[i];
		const float xformB = nsvgB->xform[i];
		if (xformA != xformB) {
			xform[i] = xformA * s + xformB * t;
			xformIsAnimated = true;
		} else {
			xform[i] = xformA;
		}
	}
	if (xformIsAnimated) {
		nsvg::xformInverse(xformInverse, xform);
	} else if (gradA != this) {
		std::memcpy(xformInverse, gradA->xformInverse, sizeof(xformInverse));
	}
	nsvg->fx = nsvgA->fx * s + nsvgB->fx * t;
	nsvg->fy = nsvgA->fy * s + nsvgB->fy * t;

	sorted = true;
	for (int i = 0; i < nstops; i++) {
		const auto &stopA = nsvgA->stops[i];
		const auto &stopB = nsvgB->stops[i];
		
		float offset = stopA.offset * s + stopB.offset * t;
		nsvg->stops[i].offset = offset;

		if (i > 0 && offset < nsvg->stops[i - 1].offset) {
			sorted = false;
		}

		nsvg->stops[i].color = lerpColor(stopA.color, stopB.color, t);
	}

	changed();
	return true;
}

#if TOVE_DEBUG
std::ostream &AbstractGradient::dump(std::ostream &os) {
	os << "xform:" << std::endl;
	os << tove::debug::xform(nsvg->xform);

	os << "xform inverse:" << std::endl;
	os << tove::debug::xform(xformInverse);

	tove::debug::save_ios_fmt fmt(os);
	os << std::fixed << std::setprecision(2);

	os << "fx/fy: " <<
		nsvg->fx << ", " <<
		nsvg->fy << ", " <<
		(int)nsvg->spread << std::endl;

	os << "color stops:" << std::endl;

	{
		tove::debug::indent_output indent(os);

		for (int i = 0; i < nsvg->nstops; i++) {
			const auto &stop = nsvg->stops[i];
			os << "[" << i << ": " << stop.offset << "] " <<
				tove::debug::color(stop.color) << std::endl;
		}
	}

	return os;
}
#endif


LinearGradient::LinearGradient(const NSVGgradient *gradient) :
	AbstractGradient(gradient) {
	
	nsvg->spread = 0;
	nsvg->fx = 0;
	nsvg->fy = 0;
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

#if TOVE_DEBUG
std::ostream &LinearGradient::dump(std::ostream &os) {
	os << "linear gradient" << std::endl;
	return AbstractGradient::dump(os);
}
#endif


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

#if TOVE_DEBUG
std::ostream &RadialGradient::dump(std::ostream &os) {
	os << "radial gradient" << std::endl;
	return AbstractGradient::dump(os);
}
#endif


PaintRef NSVGpaintToPaint(const NSVGpaint &paint) {
	switch (paint.type) {
		case NSVG_PAINT_NONE: {
			return PaintRef();
		} break;
		case NSVG_PAINT_COLOR: {
			return tove_make_shared<Color>(
				(paint.color & 0xff) / 255.0,
				((paint.color >> 8) & 0xff) / 255.0,
				((paint.color >> 16) & 0xff) / 255.0,
				((paint.color >> 24) & 0xff) / 255.0);
		} break;
		case NSVG_PAINT_LINEAR_GRADIENT: {
			return tove_make_shared<LinearGradient>(paint.gradient);
		} break;
		case NSVG_PAINT_RADIAL_GRADIENT: {
			return tove_make_shared<RadialGradient>(paint.gradient);
		} break;
	}
	tove::report::err("invalid nsvg paint type.");
	return PaintRef();
}

END_TOVE_NAMESPACE
