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

#ifndef __TOVE_SHADER_DATA
#define __TOVE_SHADER_DATA 1

#include "../interface.h"

class AllocateGeometryData {
protected:
	ToveShaderGeometryData &data;

public:
	AllocateGeometryData(
		int maxCurves,
		bool fragmentShaderStrokes,
		ToveShaderGeometryData &data);
	~AllocateGeometryData();
};

struct AllocateGeometryNoLinkData : public AllocateGeometryData {
	AllocateGeometryNoLinkData(
		int maxCurves,
		bool fragmentShaderStrokes,
		ToveShaderGeometryData &data);
	~AllocateGeometryNoLinkData();
};

#endif // __TOVE_SHADER_DATA
