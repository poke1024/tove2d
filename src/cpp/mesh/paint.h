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

#ifndef __TOVE_MESH_PAINT
#define __TOVE_MESH_PAINT 1

#include "../nsvg.h"
#include "../utils.h"

BEGIN_TOVE_NAMESPACE

class MeshPaint {
private:
	nsvg::CachedPaint cache;
	float scale;
	float cacheColors[256];

public:
	inline MeshPaint() : cache(cacheColors, sizeof(uint32_t), 256) {
	}

	inline void initialize(const NSVGpaint &paint, float opacity, float scale) {
		cache.init(paint, opacity);
		this->scale = scale;
	}

	inline int getType() const {
		return cache.type;
	}

	inline int getColor() const {
		return cache.colors[0];
	}

	inline int getLinearGradientColor(float x, float y) const {
		const float *t = cache.xform;
		const float gy = (x / scale) * t[1] + (y / scale) * t[3] + t[5];
		return cache.colors[(int)clamp(gy * 255.0f, 0, 255.0f)];
	}

	inline int getRadialGradientColor(float x, float y) const {
		const float *t = cache.xform;
		x /= scale;
		y /= scale;
		const float gx = x * t[0] + y * t[2] + t[4];
		const float gy = x * t[1] + y * t[3] + t[5];
		const float gd = sqrtf(gx * gx + gy * gy);
		return cache.colors[(int)clamp(gd * 255.0f, 0, 255.0f)];
	}
};

END_TOVE_NAMESPACE

#endif // __TOVE_MESH_PAINT
