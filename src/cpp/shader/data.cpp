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
	data.bounds = nullptr;
    data.strokeWidth = 0;

    data.lookupTable = nullptr;
	data.lookupTableMeta = nullptr;
    data.lookupTableSize = numCurves * 4 + 2; // number of (x, y) pairs

    data.listsTexture = nullptr;
    data.listsTextureSize[0] = div4(numCurves + 2);
    data.listsTextureSize[1] = 2 * (numCurves * 2 + 2);
    data.listsTextureFormat = "rgba8";

    data.curvesTexture = nullptr;
    data.curvesTextureSize[0] = div4(16);
    data.curvesTextureSize[1] = numCurves;
    data.curvesTextureFormat = "rgba16f";

	// except for AllocateGeometryNoLinkData, the following fields will
	// be initialized in tove's lua lib using LÖVE ByteData:

	// - bounds
	// - lookupTable
	// - lookupTableMeta
	// - listsTexture
	// - curvesTexture
}

AllocateGeometryData::~AllocateGeometryData() {
}

AllocateGeometryNoLinkData::AllocateGeometryNoLinkData(
	int numCurves, ToveShaderGeometryData &data) :
	AllocateGeometryData(numCurves, data) {

	// do not use this variant for data linked to tove's lua lib.

	data.bounds = new ToveBounds;

	data.listsTextureRowBytes = data.listsTextureSize[0] * sizeof(uint8_t) * 4;
    data.listsTexture = new uint8_t[
		data.listsTextureRowBytes * data.listsTextureSize[1]];

    data.curvesTextureRowBytes = data.curvesTextureSize[0] * sizeof(uint16_t) * 4;
    data.curvesTexture = new uint16_t[
		data.curvesTextureRowBytes * data.curvesTextureSize[1] / sizeof(uint16_t)];

	data.lookupTable = new float[data.lookupTableSize * 2];
	data.lookupTableMeta = new ToveLookupTableMeta;
}

AllocateGeometryNoLinkData::~AllocateGeometryNoLinkData() {
	delete data.bounds;

    delete[] data.listsTexture;
    delete[] data.curvesTexture;

	delete[] data.lookupTable;
	delete data.lookupTableMeta;
}
