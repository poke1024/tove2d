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

#ifndef __TOVE_COMMON
#define __TOVE_COMMON 1

#define BEGIN_TOVE_NAMESPACE namespace tove {
#define END_TOVE_NAMESPACE }

#include "interface.h"
#include "warn.h"

#include <memory>
#include <limits>
#include <assert.h>

BEGIN_TOVE_NAMESPACE

#define NANOSVG_CPLUSPLUS 1
#include "../thirdparty/nanosvg.h"
#include "../thirdparty/nanosvgrast.h"
END_TOVE_NAMESPACE

#include "../thirdparty/clipper.hpp"

#if _MSC_VER
#define M_PI 3.1415926535897932384626433832795
#endif

#if __clang__
inline void store_fp16(uint16_t &p, float x) {
	*reinterpret_cast<__fp16*>(&p) = x;
}
#elif __GNUC__
#include <x86intrin.h>
inline void store_fp16(uint16_t &p, float x) {
	p = _cvtss_sh(x, 0);
}
#else
#include "../thirdparty/half/include/half.hpp"
inline void store_fp16(uint16_t &p, float x) {
	half_float::half y(x);
	p = *(uint16_t*)&y;
}
#endif

BEGIN_TOVE_NAMESPACE

typedef ClipperLib::Path ClipperPath;
typedef ClipperLib::Paths ClipperPaths;
typedef ClipperLib::IntPoint ClipperPoint;

#define MAX_FLATTEN_RECURSIONS 10

class AbstractPaint;
typedef std::shared_ptr<AbstractPaint> PaintRef;

class Subpath;
typedef std::shared_ptr<Subpath> SubpathRef;

class Graphics;
typedef std::shared_ptr<Graphics> GraphicsRef;

class Path;
typedef std::shared_ptr<Path> PathRef;

class AbstractMesh;
typedef std::shared_ptr<AbstractMesh> MeshRef;

class AbstractFeed;
typedef std::shared_ptr<AbstractFeed> FeedRef;

inline int nextpow2(uint32_t v) {
	// see https://graphics.stanford.edu/~seander/bithacks.html#RoundUpPowerOf2
	if (v <= 8) {
		return 8;
	}
	v--;
	v |= v >> 1;
	v |= v >> 2;
	v |= v >> 4;
	v |= v >> 8;
	v |= v >> 16;
	v++;
	return v;
}

typedef float coeff;

END_TOVE_NAMESPACE

#endif // TOVE_COMMON
