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

#include <vector>
#include "../common.h"
#include "data.h"
#include "lookup.h"

class GeometryShaderLinkImpl {
private:
	const int maxCurves;
	const int maxSubPaths;

	ToveShaderGeometryData &geometryData;
	const ToveShaderColorData &lineColorData;
	ToveShaderGeometryData strokeShaderData;

    std::vector<Event> fillEvents;
    std::vector<Event> strokeEvents;
    LookupTable fillEventsLUT;
    LookupTable strokeEventsLUT;
    LookupTable::CurveSet strokeCurves;
	std::vector<ExCurveData> extended;

	AllocateGeometryData allocData;
	AllocateGeometryNoLinkData allocStrokeData;
	const bool enableFragmentShaderStrokes;

	int buildLUT(int dim, const int ncurves);
	void dumpCurveData();

public:
	GeometryShaderLinkImpl(
		ToveShaderGeometryData &data,
		const ToveShaderColorData &lineColorData,
		const PathRef &path,
		bool enableFragmentShaderStrokes);

	ToveChangeFlags beginUpdate(const PathRef &path, bool initial);
    int endUpdate(const PathRef &path, bool initial);
};
