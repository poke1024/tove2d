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
#include "../observer.h"
#include "geometry_data.h"
#include "lookup.h"

BEGIN_TOVE_NAMESPACE

class GeometryFeed : public Observer {
private:
	const PathRef path;
	ToveChangeFlags changes;

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

	GeometryData allocData;
	GeometryNoLinkData allocStrokeData;
	const bool enableFragmentShaderStrokes;

	int buildLUT(int dim, const int ncurves);

#if TOVE_GPUX_MESH_BAND
	struct Band {
		float y0;
		float y1;
		int16_t i;
	};

	std::vector<Band> bands;

	int computeYBands();
	void createSafeYBandMesh(const int numBands);
	void createUnsafeYBandMesh(const int numBands);
	void createXBandMesh();
#endif

	void dumpCurveData();

public:
	GeometryFeed(
		const PathRef &path,
		ToveShaderGeometryData &data,
		const TovePaintData &lineColorData,
		bool enableFragmentShaderStrokes);
	virtual ~GeometryFeed();

	ToveChangeFlags beginUpdate();
    ToveChangeFlags endUpdate();

	virtual void observableChanged(Observable *observable, ToveChangeFlags what);
};

END_TOVE_NAMESPACE
