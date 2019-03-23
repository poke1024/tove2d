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

#ifndef __TOVE_NSVG
#define __TOVE_NSVG 1

#include "common.h"

BEGIN_TOVE_NAMESPACE

#define NSVG_CLIP_PATHS 1

namespace nsvg {

NSVGimage *parseSVG(const char *svg, const char *units, float dpi);

uint32_t makeColor(float r, float g, float b, float a);
uint32_t applyOpacity(uint32_t color, float opacity);

int maxDashes();

void curveBounds(float *bounds, float *curve);

class Matrix3x2 {
	double m_val[6];

public:
	inline Matrix3x2() {
	}

	inline Matrix3x2(const float *m) {
		load(m);
	}

	inline Matrix3x2 &operator=(const Matrix3x2& m) {
		std::memcpy(m_val, m.m_val, sizeof(m_val));
		return *this;
	}

	inline void load(const float *m) {
		for (int i = 0; i < 6; i++) {
			m_val[i] = m[i];
		}
	}

	inline void store(float *m) const {
		for (int i = 0; i < 6; i++) {
			m[i] = m_val[i];
		}
	}

	inline double operator[](int i) const {
		return m_val[i];
	}

	void setIdentity();

	Matrix3x2 inverse() const;

#if TOVE_DEBUG
	friend inline std::ostream &operator<<(std::ostream &os, const Matrix3x2 &m) {
		os << tove::debug::xform<double>(m.m_val);
		return os;
	}
#endif
};

void xformInverse(float *a, float *b);
void xformIdentity(float *m);

float *pathArcTo(float *cpx, float *cpy, float *args, int &npts);
NSVGimage *parsePath(const char *d);

struct CachedPaint {
	char type;
	char spread;
	float xform[6];

	uint32_t * const colors;
	const int rowBytes;
	const int numColors;

	inline CachedPaint(void *colors, int rowBytes, int numColors) :
	 	colors(reinterpret_cast<uint32_t*>(colors)),
		rowBytes(rowBytes),
		numColors(numColors) {
	}

	void init(const NSVGpaint &paint, float opacity);
};

const ToveRasterizeSettings *getDefaultRasterizeSettings();

bool shapeStrokeBounds(float *bounds, const NSVGshape *shape,
	float scale, const ToveRasterizeSettings *settings);

void rasterize(NSVGimage *image, float tx, float ty, float scale,
	uint8_t *pixels, int width, int height, int stride,
	const ToveRasterizeSettings *settings);

class Transform {
private:
	float matrix[6];
	bool identity;
	bool scaleLineWidth;

public:
	Transform();
	Transform(
		float a, float b, float c,
		float d, float e, float f);
	Transform(const Transform &t);

	void multiply(const Transform &t);

	void transformGradient(NSVGgradient* grad) const;
	void transformPoints(float *pts, const float *srcpts, int npts) const;

	float getScale() const;

	inline bool isIdentity() const {
		return identity;
	}

	inline float wantsScaleLineWidth() const {
		return scaleLineWidth;
	}
	inline void setWantsScaleLineWidth(bool scale) {
		scaleLineWidth = scale;
	}
};

NSVGlineJoin nsvgLineJoin(ToveLineJoin join);
ToveLineJoin toveLineJoin(NSVGlineJoin join);

}

END_TOVE_NAMESPACE

#endif // __TOVE_NSVG
