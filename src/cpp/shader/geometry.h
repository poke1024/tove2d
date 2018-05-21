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
	ToveShaderGeometryData &geometryData;
	const ToveShaderColorData &lineColorData;
	ToveShaderGeometryData strokeShaderData;

	const int maxCurves;
    std::vector<Event> fillEvents;
    std::vector<Event> strokeEvents;
    LookupTable fillEventsLUT;
    LookupTable strokeEventsLUT;
    LookupTable::CurveSet strokeCurves;
	std::vector<ExtendedCurveData> extended;

	AllocateGeometryData allocData;
	AllocateGeometryNoLinkData allocStrokeData;

	int buildLUT(int dim);
	void dumpCurveData();

public:
	GeometryShaderLinkImpl(
		ToveShaderGeometryData &data,
		const ToveShaderColorData &lineColorData,
		int maxCurves);

	ToveChangeFlags beginUpdate(const PathRef &path, bool initial);
    int endUpdate(const PathRef &path, bool initial);
};
