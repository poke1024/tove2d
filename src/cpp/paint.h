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

#ifndef __TOVE_PAINT
#define __TOVE_PAINT 1

#include "common.h"
#include "claimable.h"
#include "nsvg.h"

class AbstractPaint : public Claimable<Path> {
protected:
	void changed();

public:
	virtual ~AbstractPaint() {
	}

	virtual PaintRef clone() const = 0;
	virtual TovePaintType getType() const = 0;

	virtual bool isGradient() const = 0;
	virtual void store(NSVGpaint &paint) = 0;

	virtual void transform(float sx, float sy, float tx, float ty) = 0;

	virtual void addColorStop(float offset, float r, float g, float b, float a) = 0;
	virtual void setColorStop(int i, float offset, float r, float g, float b, float a) = 0;
	virtual int getNumStops() const = 0;
	virtual NSVGgradient *getNSVGgradient() const = 0;
	virtual void getGradientMatrix(ToveMatrix3x3 &m) const = 0;

	virtual void setRGBA(float r, float g, float b, float a) {
	}
	virtual void getRGBA(RGBA *rgba, float opacity) const {
		rgba->r = 0.0;
		rgba->g = 0.0;
		rgba->b = 0.0;
		rgba->a = 0.0;
	}

	virtual void animate(const PaintRef &a, const PaintRef &b, float t) {

	}
};

class Color : public AbstractPaint {
private:
	uint32_t color;

public:
	inline Color(float r, float g, float b, float a = 1) {
		color = nsvg::makeColor(r, g, b, a);
	}

	virtual PaintRef clone() const {
		RGBA rgba;
		getRGBA(&rgba, 1.0);
		return std::make_shared<Color>(rgba.r, rgba.g, rgba.b, rgba.a);
	}

	virtual TovePaintType getType() const {
		return PAINT_SOLID;
	}

	virtual bool isGradient() const {
		return false;
	}

	virtual void transform(float sx, float sy, float tx, float ty) {
	}

	virtual void addColorStop(float offset, float r, float g, float b, float a) {
	}

	virtual void setColorStop(int i, float offset, float r, float g, float b, float a) {
	}

	virtual int getNumStops() const {
		return 0;
	}

	virtual NSVGgradient *getNSVGgradient() const {
		return nullptr;
	}

	virtual void getGradientMatrix(ToveMatrix3x3 &m) const {
	}

	virtual void setRGBA(float r, float g, float b, float a = 1) {
		color = nsvg::makeColor(r, g, b, a);
		changed();
	}

	virtual void store(NSVGpaint &paint) {
		paint.type = NSVG_PAINT_COLOR;
		paint.color = color;
	}

	virtual void getRGBA(RGBA *rgba, float opacity) const {
		uint32_t c;
		if (opacity < 1.0) {
			c = nsvg::applyOpacity(color, opacity);
		} else {
			c = color;
		}
		rgba->r = (c & 0xff) / 255.0;
		rgba->g = ((c >> 8) & 0xff) / 255.0;
		rgba->b = ((c >> 16) & 0xff) / 255.0;
		rgba->a = ((c >> 24) & 0xff) / 255.0;
	}
};

class AbstractGradient : public AbstractPaint {
protected:
	NSVGgradient *nsvg;
	NSVGgradient *nsvgInverse;
	mutable bool sorted;
	float xformInverse[6];

	static inline size_t getRecordSize(int nstops) {
		return sizeof(NSVGgradient) + (nstops - 1) * sizeof(NSVGgradientStop);
	}

	void sort() const;

	NSVGgradient *getInverseNSVGgradient();

public:
	AbstractGradient(int nstops);
	AbstractGradient(const NSVGgradient *gradient);
	AbstractGradient(const AbstractGradient &gradient);

	virtual ~AbstractGradient() {
		free(nsvg);
		free(nsvgInverse);
	}

	virtual bool isGradient() const {
		return true;
	}

	virtual void transform(float sx, float sy, float tx, float ty);

	virtual void addColorStop(float offset, float r, float g, float b, float a) {
		const size_t size = getRecordSize(nsvg->nstops + 1);
		nsvg = static_cast<NSVGgradient*>(realloc(nsvg, nextpow2(size)));
		if (!nsvg) {
			throw std::bad_alloc();
		}
		nsvg->nstops += 1;
		setColorStop(nsvg->nstops - 1, offset, r, g, b, a);
		sorted = false;
	}

	virtual void setColorStop(int i, float offset, float r, float g, float b, float a) {
		if (i >= 0 && i < nsvg->nstops) {
			nsvg->stops[i].color = nsvg::makeColor(r, g, b, a);
			nsvg->stops[i].offset = offset;
			changed();
			sorted = false;
		}
	}

	virtual int getNumStops() const {
		return nsvg->nstops;
	}

	virtual NSVGgradient *getNSVGgradient() const {
		sort();
		return nsvg;
	}

	virtual void getGradientMatrix(ToveMatrix3x3 &m) const {
		const float *xform = xformInverse;

		m[0] = xform[0];
		m[1] = xform[2];
		m[2] = xform[4];

		m[3] = xform[1];
		m[4] = xform[3];
		m[5] = xform[5];

		m[6] = 0;
		m[7] = 0;
		m[8] = 1;
	}
};

class LinearGradient : public AbstractGradient {
public:
	LinearGradient(float x1, float y1, float x2, float y2);

	inline LinearGradient(int nstops) : AbstractGradient(nstops) {
	}

	inline LinearGradient(const NSVGgradient *gradient) : AbstractGradient(gradient) {
	}

	inline LinearGradient(const LinearGradient &gradient) : AbstractGradient(gradient) {
	}

	virtual PaintRef clone() const {
		return std::make_shared<LinearGradient>(*this);
	}

	virtual TovePaintType getType() const {
		return PAINT_LINEAR_GRADIENT;
	}

	virtual void store(NSVGpaint &paint) {
		sort();
		paint.type = NSVG_PAINT_LINEAR_GRADIENT;
		paint.gradient = getInverseNSVGgradient();
	}
};

class RadialGradient : public AbstractGradient {
public:
	RadialGradient(float cx, float cy, float fx, float fy, float r);

	inline RadialGradient(int nstops) : AbstractGradient(nstops) {
	}

	inline RadialGradient(const NSVGgradient *gradient) : AbstractGradient(gradient) {
	}

	inline RadialGradient(const RadialGradient &gradient) : AbstractGradient(gradient) {
	}

	virtual PaintRef clone() const {
		return std::make_shared<RadialGradient>(*this);
	}

	virtual TovePaintType getType() const {
		return PAINT_RADIAL_GRADIENT;
	}

	virtual void store(NSVGpaint &paint) {
		sort();
		paint.type = NSVG_PAINT_RADIAL_GRADIENT;
		paint.gradient = getInverseNSVGgradient();
	}
};

PaintRef NSVGpaintToPaint(const NSVGpaint &paint);

#endif // __TOVE_PAINT
