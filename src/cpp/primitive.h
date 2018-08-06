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

#ifndef __TOVE_PRIMITIVE
#define __TOVE_PRIMITIVE 1

#include "common.h"
#include <cmath>

BEGIN_TOVE_NAMESPACE

#define TOVE_KAPPA90 (0.5522847493f) // taken from nsvg.

inline void polarToCartesian(float cx, float cy, float r, float angleInDegrees, float *x, float *y) {
	float angleInRadians = angleInDegrees * M_PI / 180.0;
	*x = cx + (r * std::cos(angleInRadians));
	*y = cy + (r * std::sin(angleInRadians));
}

enum ToveCommandType {
	TOVE_MOVE_TO,
	TOVE_LINE_TO,
	TOVE_CURVE_TO,
	TOVE_DRAW_RECT,
	TOVE_DRAW_ELLIPSE
};

static float *lineTo(float *p, float x, float y) {
	float px = p[-2 + 0];
	float py = p[-2 + 1];
	float dx = x - px;
	float dy = y - py;
	p[0] = px + dx / 3.0f; p[1] = py + dy / 3.0f;
	p[2] = x - dx / 3.0f; p[3] = y - dy / 3.0f;
	p[4] = x; p[5] = y;
	return p + 3 * 2;
}

class LinePrimitive {
public:
	float x, y;

	inline LinePrimitive() {
	}

	inline LinePrimitive(float x, float y) :
		x(x), y(y) {
	}

	inline int size() const {
		return 3;
	}

	inline void write(float *p) {
		lineTo(p, x, y);
	}
};

class RectPrimitive {
private:
	enum RectType {
		RECT_EMPTY,
		RECT_SHARP,
		RECT_ROUND
	};

	RectType type;

public:
	float x, y, w, h, rx, ry;

	inline RectPrimitive() : type(RECT_EMPTY) {
	}

	RectPrimitive(float x, float y, float w, float h, float rx, float ry) :
		x(x), y(y), w(w), h(h), rx(rx), ry(ry) {

		normalize();

		if (w != 0.0f && h != 0.0f) {
			if (rx < std::numeric_limits<float>::epsilon() || ry < std::numeric_limits<float>::epsilon()) {
				type = RECT_SHARP;
			} else {
				type = RECT_ROUND;
			}
		} else {
			type = RECT_EMPTY;
		}
	}

	void normalize() {
		if (rx < 0.0f && ry > 0.0f) rx = ry;
		if (ry < 0.0f && rx > 0.0f) ry = rx;
		if (rx < 0.0f) rx = 0.0f;
		if (ry < 0.0f) ry = 0.0f;
		if (rx > w/2.0f) rx = w/2.0f;
		if (ry > h/2.0f) ry = h/2.0f;
	}

	int size() const {
		switch (type) {
			case RECT_EMPTY:
				return 0;
			case RECT_SHARP:
				return 1 + 12;
			case RECT_ROUND:
				return 1 + 4 * 6;
		}
		assert(false);
	}

	void write(float *p) {
		switch (type) {
			case RECT_EMPTY: {
			} break;
			case RECT_SHARP: {
				*p++ = x; *p++ = y;
				p = lineTo(p, x + w, y);
				p = lineTo(p, x + w, y + h);
				p = lineTo(p, x, y + h);
				p = lineTo(p, x, y);
			} break;
			case RECT_ROUND: {
				*p++ = x+rx; *p++ = y;
				p = lineTo(p, x+w-rx, y);
				*p++ = x+w-rx*(1-TOVE_KAPPA90); *p++ = y; *p++ =x+w; *p++ =y+ry*(1-TOVE_KAPPA90); *p++ = x+w; *p++ = y+ry;
				p = lineTo(p, x+w,  y+h-ry);
				*p++ = x+w; *p++ = y+h-ry*(1-TOVE_KAPPA90); *p++ = x+w-rx*(1-TOVE_KAPPA90); *p++ = y+h; *p++ = x+w-rx; *p++ = y+h;
				p = lineTo(p, x+rx, y+h);
				*p++ = x+rx*(1-TOVE_KAPPA90); *p++ = y+h; *p++ = x; *p++ = y+h-ry*(1-TOVE_KAPPA90); *p++ = x; *p++ = y+h-ry;
				p = lineTo(p, x, y+ry);
				*p++ = x; *p++ = y+ry*(1-TOVE_KAPPA90); *p++ = x+rx*(1-TOVE_KAPPA90); *p++ = y; *p++ = x+rx; *p++ = y;
			} break;
		}
	}
};

class EllipsePrimitive {
public:
	float cx, cy, rx, ry;

	inline EllipsePrimitive() {
	}

	EllipsePrimitive(float cx, float cy, float rx, float ry) : cx(cx), cy(cy), rx(rx), ry(ry) {
	}

	inline int size() const {
		return 1 + 3 * 4;
	}

	void write(float *p) {
		*p++ = cx + rx; *p++ = cy;
		*p++ = cx+rx; *p++ = cy+ry*TOVE_KAPPA90; *p++ = cx+rx*TOVE_KAPPA90; *p++ = cy+ry; *p++ = cx; *p++ = cy+ry;
		*p++ = cx-rx*TOVE_KAPPA90; *p++ = cy+ry; *p++ = cx-rx; *p++ = cy+ry*TOVE_KAPPA90; *p++ = cx-rx; *p++ = cy;
		*p++ = cx-rx; *p++ = cy-ry*TOVE_KAPPA90; *p++ = cx-rx*TOVE_KAPPA90; *p++ = cy-ry; *p++ = cx; *p++ = cy-ry;
		*p++ = cx+rx*TOVE_KAPPA90; *p++ = cy-ry; *p++ = cx+rx; *p++ = cy-ry*TOVE_KAPPA90; *p++ = cx+rx; *p++ = cy;
	}
};

END_TOVE_NAMESPACE

#endif // __TOVE_PRIMITIVE
