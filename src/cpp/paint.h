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
#include "observer.h"
#include "nsvg.h"

BEGIN_TOVE_NAMESPACE

class AbstractPaint : public Observable {
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
	virtual bool isOpaque() const = 0;
	virtual void store(NSVGpaint &paint) = 0;

	virtual void addColorStop(float offset, float r, float g, float b, float a) = 0;
	virtual void setColorStop(int i, float offset, float r, float g, float b, float a) = 0;
	virtual float getColorStop(int i, ToveRGBA &rgba, float opacity) = 0;
	virtual int getNumColorStops() const = 0;
	virtual NSVGgradient *getNSVGgradient() const = 0;
	virtual void getGradientMatrix(float *m, float scale) const = 0;

	virtual void setRGBA(float r, float g, float b, float a) {
	}
	virtual void getRGBA(ToveRGBA &rgba, float opacity) const {
		rgba.r = 0.0;
		rgba.g = 0.0;
		rgba.b = 0.0;
		rgba.a = 0.0;
	}

	virtual bool animate(const PaintRef &a, const PaintRef &b, float t) = 0;

#if TOVE_DEBUG
	virtual std::ostream &dump(std::ostream &os) = 0;
#endif
};

class Color final : public AbstractPaint {
private:
	uint32_t color;

public:
	inline Color(float r, float g, float b, float a = 1) {
		color = nsvg::makeColor(r, g, b, a);
	}

	virtual PaintRef clone() const {
		ToveRGBA rgba;
		getRGBA(rgba, 1.0);
		return tove_make_shared<Color>(rgba.r, rgba.g, rgba.b, rgba.a);
	}

	virtual void getGradientParameters(ToveGradientParameters &p);

	virtual void cloneTo(PaintRef &target, const nsvg::Transform &transform);

	virtual TovePaintType getType() const {
		return PAINT_SOLID;
	}

	virtual bool isGradient() const {
		return false;
	}

	virtual bool isOpaque() const;

	virtual void addColorStop(float offset, float r, float g, float b, float a) {
	}

	virtual void setColorStop(int i, float offset, float r, float g, float b, float a) {
	}

	virtual float getColorStop(int i, ToveRGBA &rgba, float opacity) {
		getRGBA(rgba, opacity);
		return 0.0f;
	}

	virtual int getNumColorStops() const {
		return 1;
	}

	virtual NSVGgradient *getNSVGgradient() const {
		return nullptr;
	}

	virtual void getGradientMatrix(float *m, float scale) const {
	}

	virtual void setRGBA(float r, float g, float b, float a = 1) {
		color = nsvg::makeColor(r, g, b, a);
		changed();
	}

	virtual void store(NSVGpaint &paint);

	virtual void getRGBA(ToveRGBA &rgba, float opacity) const;

	virtual bool animate(const PaintRef &a, const PaintRef &b, float t);

#if TOVE_DEBUG
	virtual std::ostream &dump(std::ostream &os);
#endif
};

class AbstractGradient : public AbstractPaint {
protected:
	NSVGgradient *nsvg;
	mutable bool sorted;
	NSVGgradient *nsvgInverse;
	nsvg::Matrix3x2 xformInverse;

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

	void setNumColorStops(int numStops);

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

	virtual bool isOpaque() const;

	virtual void addColorStop(float offset, float r, float g, float b, float a);

	virtual float getColorStop(int i, ToveRGBA &rgba, float opacity);

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

	virtual void getGradientMatrix(float *m, float scale) const {
		m[0] = xformInverse[0] / scale;
		m[1] = xformInverse[2] / scale;
		m[2] = xformInverse[4];

		m[3] = xformInverse[1] / scale;
		m[4] = xformInverse[3] / scale;
		m[5] = xformInverse[5];
	}

	virtual bool animate(const PaintRef &a, const PaintRef &b, float t);

#if TOVE_DEBUG
	virtual std::ostream &dump(std::ostream &os);
#endif
};

class LinearGradient final : public AbstractGradient {
public:
	LinearGradient(float x1, float y1, float x2, float y2);

	inline LinearGradient(int nstops) : AbstractGradient(nstops) {
	}

	inline LinearGradient(const NSVGgradient *gradient);

	inline LinearGradient(const LinearGradient &gradient) : AbstractGradient(gradient) {
	}

	virtual void getGradientParameters(ToveGradientParameters &p);

	virtual PaintRef clone() const {
		return tove_make_shared<LinearGradient>(*this);
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

#if TOVE_DEBUG
	virtual std::ostream &dump(std::ostream &os);
#endif
};

class RadialGradient final : public AbstractGradient {
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
		return tove_make_shared<RadialGradient>(*this);
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

#if TOVE_DEBUG
	virtual std::ostream &dump(std::ostream &os);
#endif
};

PaintRef NSVGpaintToPaint(const NSVGpaint &paint);

END_TOVE_NAMESPACE

#endif // __TOVE_PAINT
