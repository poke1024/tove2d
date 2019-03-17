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

#include "geometry_data.h"
#include "../common.h"
#include "../utils.h"
#include <memory.h>
#include <iostream>

BEGIN_TOVE_NAMESPACE

GeometryData::GeometryData(
	int maxCurves,
	int maxSubPaths,
	bool fragmentShaderStrokes,
	ToveShaderGeometryData &data) : data(data) {

	std::memset(&data, 0, sizeof(ToveShaderGeometryData));

    data.maxCurves = maxCurves;
	data.numCurves = 0;
	data.bounds = nullptr;
    data.strokeWidth = 0;
	data.fragmentShaderLine = fragmentShaderStrokes;
	data.opaqueLine = true;

	// lookup table size: each curve can produce 2 regular
	// bounds (e.g. top, bottom) and 2 additional roots per
	// axis, i.e. 4 values per curve.

	int baseLookupTableSize = maxCurves * 4; // number of (x, y) pairs

    data.lookupTable[0] = nullptr;
    data.lookupTable[1] = nullptr;
	data.lookupTableMeta = nullptr;
	if (fragmentShaderStrokes) {
		data.lookupTableSize = baseLookupTableSize + 2; // 2 for padding
	} else {
		data.lookupTableSize = baseLookupTableSize;
	}

#if TOVE_GPUX_MESH_BAND
	// 2 triangles (3 vertices each) for each band.
	data.maxBandsVertices = 2 * 3 * data.lookupTableSize;
#endif

	// lists texture size: per axis, we can have up to 2 entries per
	// curve.

    data.listsTexture = nullptr;
	if (fragmentShaderStrokes) {
	    data.listsTextureSize[0] = div4(maxCurves + 2); // two markers
	    data.listsTextureSize[1] = 2 * (maxCurves * 2 + 2);
	} else {
		data.listsTextureSize[0] = div4(maxCurves + 1); // one marker
	    data.listsTextureSize[1] = 2 * (maxCurves * 2);
	}
    data.listsTextureFormat = "rgba8";

    data.curvesTexture = nullptr;
#if 1
	// for some reason, using a texture width of 12 for the "curves"
	// texture has problems in texture lookup in the shader on some
	// drivers (e.g. NVidia). use 16 all the time.
	data.curvesTextureSize[0] = div4(16);
#else
	if (fragmentShaderStrokes) {
		data.curvesTextureSize[0] = div4(16);
	} else {
		data.curvesTextureSize[0] = div4(12);
	}
#endif
    data.curvesTextureSize[1] = maxCurves;
	if (sizeof(gpu_float_t) == sizeof(float)) {
	    data.curvesTextureFormat = "rgba32f";
	} else if (sizeof(gpu_float_t) == sizeof(uint16_t)) {
	    data.curvesTextureFormat = "rgba16f";
	} else {
		std::cerr << "illegal gpu_float_t typedef" << std::endl;
		assert(false);
	}

	if (fragmentShaderStrokes) {
		data.lineRuns = nullptr;
	} else {
		data.lineRuns = new ToveLineRun[maxSubPaths];
	}

	// except for AllocateGeometryNoLinkData, the following fields will
	// be initialized in tove's lua lib using LÖVE ByteData:

	// - bounds
	// - lookupTable
	// - lookupTableMeta
	// - listsTexture
	// - curvesTexture
}

GeometryData::~GeometryData() {
	delete[] data.lineRuns;
}

GeometryNoLinkData::GeometryNoLinkData(
	int maxCurves, int maxSubPaths,
	bool fragmentShaderStrokes, ToveShaderGeometryData &data) :
	GeometryData(maxCurves, maxSubPaths, fragmentShaderStrokes, data) {

	// do not use this variant for data linked to tove's lua lib.

	data.bounds = new ToveBounds;

	data.listsTextureRowBytes = data.listsTextureSize[0] * sizeof(uint8_t) * 4;
    data.listsTexture = new uint8_t[
		data.listsTextureRowBytes * data.listsTextureSize[1]];

    data.curvesTextureRowBytes = data.curvesTextureSize[0] * sizeof(gpu_float_t) * 4;
    data.curvesTexture = new gpu_float_t[
		data.curvesTextureRowBytes * data.curvesTextureSize[1] / sizeof(gpu_float_t)];

	data.lookupTable[0] = new float[data.lookupTableSize];
	data.lookupTable[1] = new float[data.lookupTableSize];
	data.lookupTableMeta = new ToveLookupTableMeta;
}

GeometryNoLinkData::~GeometryNoLinkData() {
	delete data.bounds;

    delete[] data.listsTexture;
    delete[] data.curvesTexture;

	delete[] data.lookupTable[0];
	delete[] data.lookupTable[1];
	delete data.lookupTableMeta;
}

END_TOVE_NAMESPACE
