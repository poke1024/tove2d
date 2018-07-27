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

void copyColor(RGBA &rgba, uint32_t color, float opacity);

class AbstractPaint : public Claimable<Path> {
protected:
	void changed();

public:
	virtual ~AbstractPaint() {
	}

	virtual void getGradientParameters(ToveGradientParameters &p) = 0;
	virtual PaintRef clone() const = 0;
	virtual void cloneTo(PaintRef &target, const nsvg::Transform &transform) = 0;
	virtual TovePaintType getType() const = 0;
	virtual void transform(const nsvg::Transform &transform) { }

	virtual bool isGradient() const = 0;
	virtual void store(NSVGpaint &paint) = 0;

	virtual void addColorStop(float offset, float r, float g, float b, float a) = 0;
	virtual void setColorStop(int i, float offset, float r, float g, float b, float a) = 0;
	virtual float getColorStop(int i, RGBA &rgba, float opacity) = 0;
	virtual int getNumColorStops() const = 0;
	virtual NSVGgradient *getNSVGgradient() const = 0;
	virtual void getGradientMatrix(ToveMatrix3x3 &m, float scale) const = 0;

	virtual void setRGBA(float r, float g, float b, float a) {
	}
	virtual void getRGBA(RGBA &rgba, float opacity) const {
		rgba.r = 0.0;
		rgba.g = 0.0;
		rgba.b = 0.0;
		rgba.a = 0.0;
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
		getRGBA(rgba, 1.0);
		return std::make_shared<Color>(rgba.r, rgba.g, rgba.b, rgba.a);
	}

	virtual void getGradientParameters(ToveGradientParameters &p);

	virtual void cloneTo(PaintRef &target, const nsvg::Transform &transform);

	virtual TovePaintType getType() const {
		return PAINT_SOLID;
	}

	virtual bool isGradient() const {
		return false;
	}

	virtual void addColorStop(float offset, float r, float g, float b, float a) {
	}

	virtual void setColorStop(int i, float offset, float r, float g, float b, float a) {
	}

	virtual float getColorStop(int i, RGBA &rgba, float opacity) {
		getRGBA(rgba, opacity);
		return 0.0f;
	}

	virtual int getNumColorStops() const {
		return 1;
	}

	virtual NSVGgradient *getNSVGgradient() const {
		return nullptr;
	}

	virtual void getGradientMatrix(ToveMatrix3x3 &m, float scale) const {
	}

	virtual void setRGBA(float r, float g, float b, float a = 1) {
		color = nsvg::makeColor(r, g, b, a);
		changed();
	}

	virtual void store(NSVGpaint &paint);

	virtual void getRGBA(RGBA &rgba, float opacity) const {
		copyColor(rgba, color, opacity);
	}
};

class AbstractGradient : public AbstractPaint {
protected:
	NSVGgradient *nsvg;
	mutable bool sorted;
	NSVGgradient *nsvgInverse;
	float xformInverse[6];

	static inline size_t getRecordSize(int nstops) {
		return sizeof(NSVGgradient) + (nstops - 1) * sizeof(NSVGgradientStop);
	}

	void sort() const;

	inline void ensureSort() const {
		if (!sorted) {
			sort();
		}
	}

	NSVGgradient *getInverseNSVGgradient();

	void set(const AbstractGradient *source);

public:
	AbstractGradient(int nstops);
	AbstractGradient(const NSVGgradient *gradient);
	AbstractGradient(const AbstractGradient &gradient);

	virtual ~AbstractGradient() {
		free(nsvg);
		if (nsvgInverse) {
			free(nsvgInverse);
		}
	}

	virtual void transform(const nsvg::Transform &transform);

	virtual bool isGradient() const {
		return true;
	}

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

	virtual float getColorStop(int i, RGBA &rgba, float opacity) {
		if (i >= 0 && i < nsvg->nstops) {
			copyColor(rgba, nsvg->stops[i].color, opacity);
			return nsvg->stops[i].offset;
		} else {
			return 0.0f;
		}
	}

	virtual void setColorStop(int i, float offset, float r, float g, float b, float a) {
		if (i >= 0 && i < nsvg->nstops) {
			nsvg->stops[i].color = nsvg::makeColor(r, g, b, a);
			nsvg->stops[i].offset = offset;
			changed();
			sorted = false;
		}
	}

	virtual int getNumColorStops() const {
		return nsvg->nstops;
	}

	virtual NSVGgradient *getNSVGgradient() const {
		ensureSort();
		return nsvg;
	}

	virtual void getGradientMatrix(ToveMatrix3x3 &m, float scale) const {
		const float *xform = xformInverse;

		m[0] = xform[0] / scale;
		m[1] = xform[2] / scale;
		m[2] = xform[4];

		m[3] = xform[1] / scale;
		m[4] = xform[3] / scale;
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

	virtual void getGradientParameters(ToveGradientParameters &p);

	virtual PaintRef clone() const {
		return std::make_shared<LinearGradient>(*this);
	}

	virtual void cloneTo(PaintRef &target, const nsvg::Transform &transform);

	virtual TovePaintType getType() const {
		return PAINT_LINEAR_GRADIENT;
	}

	virtual void store(NSVGpaint &paint) {
		ensureSort();
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

	virtual void getGradientParameters(ToveGradientParameters &p);

	virtual PaintRef clone() const {
		return std::make_shared<RadialGradient>(*this);
	}

	virtual void cloneTo(PaintRef &target, const nsvg::Transform &transform);

	virtual TovePaintType getType() const {
		return PAINT_RADIAL_GRADIENT;
	}

	virtual void store(NSVGpaint &paint) {
		ensureSort();
		paint.type = NSVG_PAINT_RADIAL_GRADIENT;
		paint.gradient = getInverseNSVGgradient();
	}
};

PaintRef NSVGpaintToPaint(const NSVGpaint &paint);

#endif // __TOVE_PAINT
