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

#include "geometry_feed.h"
#include "../common.h"
#include "../utils.h"
#include "../subpath.h"
#include "../path.h"
#include <algorithm>

BEGIN_TOVE_NAMESPACE

typedef LookupTable::CurveSet CurveSet;

inline void updateLookupTableMeta(ToveLookupTableMeta *meta) {
	// compute the number of needed binary search iterations.
	const int fillX = meta->n[0];
	const int fillY = meta->n[1];
	const int fill = std::max(fillX, fillY);
	meta->bsearch = int(std::floor(std::log2(fill))) + 1;
}

#if TOVE_GPUX_MESH_BAND
inline float *vertex(float *v, float x, float y) {
	*v++ = x;
	*v++ = y;
	return v;
}

inline float *vertex(float *v, float x, float y, float tx, float ty) {
	*v++ = x;
	*v++ = y;
	*v++ = tx;
	*v++ = ty;
	return v;
}
#endif // TOVE_GPUX_MESH_BAND

static void queryLUT(
	ToveShaderGeometryData *data,
	int dim, float y0, float y1,
	CurveSet &result) {

	const float *lut = data->lookupTable[dim];
	const int n = data->lookupTableMeta->n[dim];

	if (n < 1) {
		return;
	}

	int lo = 0;
	int hi = n;

	do {
		const int mid = (lo + hi) / 2;
		if (lut[mid] < y0) {
			lo = mid + 1;
		} else {
			hi = mid;
		}
	} while (lo < hi);

	while (lo > 0 && lut[lo] > y0) {
		lo -= 1;
	}

	const uint8_t *base = data->listsTexture;
	const int rowBytes = data->listsTextureRowBytes;
	base += dim * rowBytes * (data->listsTextureSize[1] / 2);

	for (int i = lo; i < n; i++) {
		if (lut[i] > y1) {
			break;
		}

		const uint8_t *list = base + rowBytes * i;
		while (*list != SENTINEL_END) {
			result.insert(*list++);
		}
	}
}

int GeometryFeed::buildLUT(int dim, const int ncurves) {
	const bool hasFragLine = geometryData.fragmentShaderLine;
	const float lineWidth = geometryData.strokeWidth;

	auto e = fillEvents.begin();
	auto se = strokeEvents.begin();

	{
		const int n = ncurves;
		const int dimtwo = 1 - dim;

		assert(fillEvents.size() >= n * 6);
		assert(strokeEvents.size() >= n * 2);

		for (int i = 0; i < n; i++) {
			const ExCurveData &ext = extended[i];

			if (ext.ignore & IGNORE_FILL) {
				continue;
			}

			const CurveBounds *bounds = ext.bounds;

			const float x0 = bounds->bounds[dimtwo + 0];
			const float y0 = bounds->bounds[dim + 0];
			const float x1 = bounds->bounds[dimtwo + 2];
			const float y1 = bounds->bounds[dim + 2];

			if (y0 == y1 && x0 == x1) {
				continue;
			}

			e->y = y0;
			e->t = EVENT_ENTER;
			e->curve = i;
			e++;

			e->y = y1;
			e->t = EVENT_EXIT;
			e->curve = i;
			e++;

			e->y = ext.endpoints.p1[dim];
			e->t = EVENT_MARK;
			e->curve = i;
			e++;

			e->y = ext.endpoints.p2[dim];
			e->t = EVENT_MARK;
			e->curve = i;
			e++;

			if (hasFragLine) {
				se->y = y0 - lineWidth;
				se->t = EVENT_ENTER;
				se->curve = i;
				se++;

				se->y = y1 + lineWidth;
				se->t = EVENT_EXIT;
				se->curve = i;
				se++;
			}

			const CurveBounds::Roots &r = bounds->roots[dim];
			for (int j = 0; j < r.count; j++) {
				e->y = r.positions[2 * j + dim];
				e->t = EVENT_MARK;
				e->curve = i;
				e++;
			}
		}

		assert(e <= fillEvents.end());
		assert(se <= strokeEvents.end());
	}

	const int numFillEvents = e - fillEvents.begin();
	const int numLineEvents = se - strokeEvents.begin();

	std::sort(fillEvents.begin(), fillEvents.begin() + numFillEvents,
		[] (const Event &a, const Event &b) {
			return a.y < b.y;
		});

	if (hasFragLine) {
		std::sort(strokeEvents.begin(), strokeEvents.begin() + numLineEvents,
			[] (const Event &a, const Event &b) {
				return a.y < b.y;
			});

		strokeEventsLUT.build(dim, strokeEvents, numLineEvents, extended);

		fillEventsLUT.build(dim, fillEvents, numFillEvents, extended, lineWidth,
			[this, dim] (float y0, float y1, const CurveSet &active, uint8_t *list, int z) {
				strokeCurves.clear();
				queryLUT(&strokeShaderData, dim, y0, y1, strokeCurves);

				bool hasStrokeSentinel = false;
				for (const auto curveIndex : strokeCurves) {
					if (active.find(curveIndex) == active.end()) {
						if (!hasStrokeSentinel) {
							*list++ = SENTINEL_STROKES;
							hasStrokeSentinel = true;
						}
						*list++ = curveIndex;
					}
				}

#if 0
				for (int i = 0; i < geometryData.numCurves; i++) {
					const int curveIndex = i;
					const ExCurveData &ext = extended[curveIndex];
					if (ext.ignore & IGNORE_FILL) {
						continue;
					}
					const CurveBounds *bounds = ext.bounds;
					const float cy0 = bounds->bounds[dim + 0];
					const float cy1 = bounds->bounds[dim + 2];
					const float lineWidth = geometryData.strokeWidth;
					if (cy1 + lineWidth <= y0 || cy0 - lineWidth >= y1) {
						if (i == 3 && dim == 0) {
							printf("skip %d %f %f %f %f\n", z, cy0 - lineWidth, cy1 + lineWidth, y0, y1);
							continue;
						}
					}
					/*if (i == 3) {
						continue;
					}*/
					if (active.find(curveIndex) == active.end() &&
						strokeCurves.find(curveIndex) == strokeCurves.end()) {
						if (!hasStrokeSentinel) {
							*list++ = SENTINEL_STROKES;
							hasStrokeSentinel = true;
						}
						*list++ = curveIndex;
					}
				}
#endif

				*list = SENTINEL_END;
			});
	} else {
		fillEventsLUT.build(dim, fillEvents, numFillEvents, extended);
	}

	return numFillEvents;
}

#if TOVE_GPUX_MESH_BAND
int GeometryFeed::computeYBands() {
	float last_y1;
	Band *bands = this->bands.data();
	int numBands = 0;

	const float *in = geometryData.lookupTable[1];
	const int n = geometryData.lookupTableMeta->n[1];
	const float margin = 0.5f;
	assert(this->bands.size() >= n);

	for (int i = 0; i < n; i++) {
		const float y = in[i];

		const float y0 = y - margin;
		const float y1 = y + margin;

		if (numBands > 0 && y0 < last_y1) {
			bands[numBands - 1].y1 = y1;
			bands[numBands - 1].i = i;
		} else {
			bands[numBands].y0 = y0;
			bands[numBands].y1 = y1;
			bands[numBands].i = i;
			numBands++;
		}

		last_y1 = y1;
	}

	if (numBands > 0) {
		const float *bounds = geometryData.bounds->bounds;
		bands[0].y0 = std::max(bands[0].y0, bounds[1]);
		bands[numBands - 1].y1 = std::min(bands[numBands - 1].y1, bounds[3]);
	}

	return numBands;
}

void GeometryFeed::createSafeYBandMesh(const int numBands) {
	float *out = geometryData.bandsVertices[0];
	float *out0 = out;

	float *bounds = geometryData.bounds->bounds;
	const float x0 = bounds[0];
	const float x1 = bounds[2];

	float y0;
	Band *bands = this->bands.data();
	assert(this->bands.size() >= numBands);
	float ty;

	const float numLists = geometryData.listsTextureSize[1];

	for (int i = 0; i < numBands; i++) {
		const Band &band = bands[i];
		const float y1 = band.y0;
		if (i > 0 && y0 < y1) {
			out = vertex(out, x0, y0, 1, ty);
			out = vertex(out, x1, y0, 1, ty);
			out = vertex(out, x0, y1, 1, ty);

			out = vertex(out, x0, y1, 1, ty);
			out = vertex(out, x1, y0, 1, ty);
			out = vertex(out, x1, y1, 1, ty);
		}
		y0 = band.y1;

		// compute texture coordinate into curve lists texture
		ty = band.i / numLists + 0.5f;
	}

	float y1 = bounds[3];

	out = vertex(out, x0, y0, 1, ty);
	out = vertex(out, x1, y0, 1, ty);
	out = vertex(out, x0, y1, 1, ty);

	out = vertex(out, x0, y1, 1, ty);
	out = vertex(out, x1, y0, 1, ty);
	out = vertex(out, x1, y1, 1, ty);

	const int floatsPerVertex = 4;
	geometryData.numBandsVertices[0] = (out - out0) / floatsPerVertex;
}

void GeometryFeed::createUnsafeYBandMesh(const int numBands) {
	float *out = geometryData.bandsVertices[1];
	float *out0 = out;

	float *bounds = geometryData.bounds->bounds;
	const float x0 = bounds[0];
	const float x1 = bounds[2];

	Band *bands = this->bands.data();
	assert(this->bands.size() >= numBands);
	for (int i = 0; i < numBands; i++) {
		const Band &band = bands[i];
		const float y0 = band.y0;
		const float y1 = band.y1;

		out = vertex(out, x0, y0);
		out = vertex(out, x1, y0);
		out = vertex(out, x0, y1);

		out = vertex(out, x0, y1);
		out = vertex(out, x1, y0);
		out = vertex(out, x1, y1);
	}

	const int floatsPerVertex = 2;
	geometryData.numBandsVertices[1] = (out - out0) / floatsPerVertex;
}

void GeometryFeed::createXBandMesh() {
	const float *in = geometryData.lookupTable[0];
	const int n = geometryData.lookupTableMeta->n[0];

	float *out = geometryData.bandsVertices[2];
	float *out0 = out;

	float *bounds = geometryData.bounds->bounds;
	const float y0 = bounds[1];
	const float y1 = bounds[3];

	const float numLists = geometryData.listsTextureSize[1];

	float x0, ty;
	for (int i = 0; i < n; i++) {
		const float x1 = in[i];

		if (i > 0) {
			out = vertex(out, x0, y0, 0, ty);
			out = vertex(out, x1, y0, 0, ty);
			out = vertex(out, x0, y1, 0, ty);

			out = vertex(out, x0, y1, 0, ty);
			out = vertex(out, x1, y0, 0, ty);
			out = vertex(out, x1, y1, 0, ty);
		}

		x0 = x1;

		// compute texture coordinate into curve lists texture
		ty = i / numLists;
	}

	const int floatsPerVertex = 4;
	geometryData.numBandsVertices[2] = (out - out0) / floatsPerVertex;
}

#endif // TOVE_GPUX_MESH_BAND

void GeometryFeed::dumpCurveData() {
#if 0
	const int w = geometryData.curvesTextureSize[0];
	const int h = geometryData.curvesTextureSize[1];

	for (int curve = 0; curve < h; curve++) {
		__fp16 *curveData = reinterpret_cast<__fp16*>(geometryData.curvesTexture) +
			geometryData.curvesTextureRowBytes * curve / sizeof(__fp16);
		printf("curve %d: ", curve);
		for (int i = 0; i < w * 4; i++) {
			printf(" %.2f", curveData[i]);
		}
		printf("\n");
	}
#endif
}

GeometryFeed::GeometryFeed(
	const PathRef &path,
	ToveShaderGeometryData &data,
	const TovePaintData &lineColorData,
	bool enableFragmentShaderStrokes) :

	path(path),
	maxCurves(path->getNumCurves()),
	maxSubPaths(path->getNumSubpaths()),
	geometryData(data),
	lineColorData(lineColorData),
	fillEventsLUT(maxCurves, geometryData, IGNORE_FILL),
	strokeEventsLUT(maxCurves, strokeShaderData, IGNORE_LINE),
	strokeCurves(4),
	allocData(maxCurves, maxSubPaths,
		enableFragmentShaderStrokes && path->hasStroke(), geometryData),
	allocStrokeData(maxCurves, maxSubPaths, true, strokeShaderData),
	enableFragmentShaderStrokes(enableFragmentShaderStrokes) {

	if (maxCurves < 1) {
		tove::report::warn("cannot render empty paths.");
	} else if (maxCurves > 253) {
		tove::report::warn("path too complex; only up to 253 curves.");
	}

	fillEvents.resize(6 * maxCurves);
	strokeEvents.resize(2 * maxCurves);
	extended.resize(maxCurves);
#if TOVE_GPUX_MESH_BAND
	bands.resize(geometryData.lookupTableSize);
#endif

	path->addObserver(this);
	changes = CHANGED_LINE_STYLE | CHANGED_INITIAL;
}

GeometryFeed::~GeometryFeed() {
	path->removeObserver(this);	
}

 void GeometryFeed::observableChanged(Observable *observable, ToveChangeFlags what) {
	changes |= what;
}

ToveChangeFlags GeometryFeed::beginUpdate() {
	if ((changes & (CHANGED_GEOMETRY | CHANGED_INITIAL)) == CHANGED_GEOMETRY) {
		changes = 0;
		return CHANGED_RECREATE;
	}

	if (changes & (CHANGED_LINE_ARGS | CHANGED_INITIAL)) {
		if ((enableFragmentShaderStrokes && path->hasStroke()) !=
			geometryData.fragmentShaderLine) {
			return CHANGED_RECREATE;
		}

		const float lineWidth = lineColorData.style > 0 ?
			std::max(path->getLineWidth(), 1.0f) : 0.0f;
		geometryData.strokeWidth = lineWidth;

		switch (path->getLineJoin()) {
			case TOVE_LINEJOIN_BEVEL:
			case TOVE_LINEJOIN_ROUND: // not supported.
				geometryData.miterLimit = 0;
				break;
			case TOVE_LINEJOIN_MITER:
				geometryData.miterLimit = path->getMiterLimit();
				break;
		}
	}

	geometryData.fillRule = path->getFillRule();

	return 0;
}

ToveChangeFlags GeometryFeed::endUpdate() {
	if ((changes & CHANGED_LINE_STYLE) && !(geometryData.fragmentShaderLine)) {
		geometryData.opaqueLine = path->hasOpaqueLine();
	}

	changes &= CHANGED_POINTS | CHANGED_LINE_ARGS | CHANGED_INITIAL;
	if (changes == 0) {
		return 0;
	}

#if TOVE_DEBUG
	assert(path->getNumCurves() <= maxCurves);

	assert(geometryData.lookupTable[0] != nullptr);
	assert(geometryData.lookupTable[1] != nullptr);
	assert(geometryData.lookupTableMeta != nullptr);

	assert(geometryData.listsTexture != nullptr);
	assert(geometryData.listsTextureRowBytes >=
		geometryData.listsTextureSize[0] * 4);

	assert(geometryData.curvesTexture != nullptr);
	assert(geometryData.curvesTextureRowBytes >=
		geometryData.curvesTextureSize[0] * 2);
	assert(geometryData.curvesTextureSize[1] == maxCurves);
#endif

	// build curve data

	int numSubpaths = path->getNumSubpaths();
	assert(numSubpaths <= maxSubPaths);

	ToveLineRun *lineRuns = geometryData.lineRuns;

	int curveIndex = 0;
	for (int i = 0; i < numSubpaths; i++) {
		const SubpathRef t = path->getSubpath(i);
		const int n = t->getNumCurves(false);

		if (lineRuns) {
			lineRuns->curveIndex = curveIndex;
			lineRuns->numCurves = n;
			lineRuns->isClosed = t->isClosed();
			lineRuns++;
		}

		if (true) { // FIXME: check for subpath changes
			for (int j = 0; j < n; j++) {
				assert(curveIndex < maxCurves);
				if (t->computeShaderCurveData(
					&geometryData, j, curveIndex, extended[curveIndex])) {

					extended[curveIndex].ignore = 0;
				} else {
					extended[curveIndex].ignore = IGNORE_FILL | IGNORE_LINE;
				}
				curveIndex++;
			}
		} else {
			assert(false); // not implemented yet
			//curveIndex += t->getNumCurves();
		}
	}
	assert(curveIndex <= maxCurves);
	geometryData.numCurves = curveIndex;
	geometryData.numSubPaths = numSubpaths;

#if 0
	if (initial) {
		dumpCurveData();
	}
#endif

	float *bounds = geometryData.bounds->bounds;

	for (int dim = 0; dim < 2; dim++) {
		int numEvents = buildLUT(dim, curveIndex);

		if (numEvents > 0) {
			bounds[dim + 0] = fillEvents[0].y;
			bounds[dim + 2] = fillEvents[numEvents - 1].y;
		} else {
			// all curves are points.
			bounds[dim + 0] = 0.0f;
			bounds[dim + 2] = 0.0f;
		}
	}

	if (geometryData.fragmentShaderLine && lineColorData.style > 0) {
		const float lineWidth = geometryData.strokeWidth;
		bounds[0] -= lineWidth;
		bounds[1] -= lineWidth;
		bounds[2] += lineWidth;
		bounds[3] += lineWidth;
	}

#if TOVE_GPUX_MESH_BAND
	if (geometryData.bandsVertices[0]) {
		const int numBands = computeYBands();

		createSafeYBandMesh(numBands);
		createUnsafeYBandMesh(numBands);
		createXBandMesh();

		for (int i = 0; i < 3; i++) {
			assert(geometryData.numBandsVertices[i] <= geometryData.maxBandsVertices);
		}
	}
#endif

	updateLookupTableMeta(geometryData.lookupTableMeta);

	const ToveChangeFlags returnedChanges = changes;
	changes = 0;

	if ((returnedChanges & CHANGED_LINE_ARGS) && !path->hasStroke()) {
		return returnedChanges & ~CHANGED_LINE_ARGS;
	} else {
		return returnedChanges;
	}
}

END_TOVE_NAMESPACE
