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

#include "data.h"

#include "../common.h"
#include "../utils.h"
#include <memory.h>

AllocateGeometryData::AllocateGeometryData(
	int numCurves, ToveShaderGeometryData &data) : data(data) {

	std::memset(&data, 0, sizeof(ToveShaderGeometryData));

    data.numCurves = numCurves;
    data.strokeWidth = 0;

    data.lookupTable = nullptr;
    data.lookupTableSize = numCurves * 4 + 2; // number of (x, y) pairs
    data.lookupTableFill[0] = 0;
	data.lookupTableFill[1] = 0;

    data.listsTexture = nullptr;
    data.listsTextureSize[0] = mod4(numCurves + 2);
    data.listsTextureSize[1] = 2 * (numCurves * 2 + 2);
    data.listsTextureFormat = "rgba8";

    data.curvesTexture = nullptr;
    data.curvesTextureSize[0] = mod4(16);
    data.curvesTextureSize[1] = numCurves;
    data.curvesTextureFormat = "rgba16f";

	// texture allocation will usually happen in tove's lua lib; except
	// for AllocateGeometryNoLinkData.

	// now allocate the lookup table.
	const int n = data.lookupTableSize * 2;
	float *lut = new float[n];
	data.lookupTable = lut;

	// we need to initialize this with 0: as (x, y) are sent interleaved
	// via löve's shader:send later, any nan's here in the unused (less
	// filled) dimension will cause a "nil not a number" error.
	for (int i = 0; i < n; i++) {
		lut[i] = 0.0;
	}
}

AllocateGeometryData::~AllocateGeometryData() {
	delete[] data.lookupTable;
}

AllocateGeometryNoLinkData::AllocateGeometryNoLinkData(
	int numCurves, ToveShaderGeometryData &data) :
	AllocateGeometryData(numCurves, data) {

	// do not use this variant for data linked to tove's lua lib.

	data.listsTextureRowBytes = data.listsTextureSize[0] * sizeof(uint8_t) * 4;
    data.listsTexture = new uint8_t[
		data.listsTextureRowBytes * data.listsTextureSize[1]];

    data.curvesTextureRowBytes = data.curvesTextureSize[0] * sizeof(uint16_t) * 4;
    data.curvesTexture = new uint16_t[
		data.curvesTextureRowBytes * data.curvesTextureSize[1] / sizeof(uint16_t)];
}

AllocateGeometryNoLinkData::~AllocateGeometryNoLinkData() {
    delete[] data.listsTexture;
    delete[] data.curvesTexture;
}
