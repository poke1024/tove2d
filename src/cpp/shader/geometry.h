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

BEGIN_TOVE_NAMESPACE

class GeometryFeedImpl {
private:
	const PathRef path;
	bool initialUpdate;

	const int maxCurves;
	const int maxSubPaths;

	ToveShaderGeometryData &geometryData;
	const TovePaintData &lineColorData;
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
	GeometryFeedImpl(
		const PathRef &path,
		ToveShaderGeometryData &data,
		const TovePaintData &lineColorData,
		bool enableFragmentShaderStrokes);

	ToveChangeFlags beginUpdate();
    ToveChangeFlags endUpdate();
};

END_TOVE_NAMESPACE
