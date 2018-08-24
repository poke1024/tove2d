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

#include "../common.h"
#include "../interface.h"

BEGIN_TOVE_NAMESPACE

class GeometryData {
protected:
	ToveShaderGeometryData &data;

public:
	GeometryData(
		int maxCurves,
		int maxSubPaths,
		bool fragmentShaderStrokes,
		ToveShaderGeometryData &data);
	~GeometryData();
};

struct GeometryNoLinkData : public GeometryData {
	GeometryNoLinkData(
		int maxCurves,
		int maxSubPaths,
		bool fragmentShaderStrokes,
		ToveShaderGeometryData &data);
	~GeometryNoLinkData();
};

END_TOVE_NAMESPACE

#endif // __TOVE_SHADER_DATA
