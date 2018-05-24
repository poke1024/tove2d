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

#include "geometry.h"
#include "../common.h"
#include "../utils.h"
#include "../trajectory.h"
#include "../path.h"
#include <algorithm>

typedef LookupTable::CurveSet CurveSet;

inline void updateLookupTableMeta(ToveLookupTableMeta *meta) {
	// compute the number of needed binary search iterations.
	const int fillX = meta->n[0];
	const int fillY = meta->n[1];
	const int fill = std::max(fillX, fillY);
	meta->bsearch = int(std::floor(std::log2(fill))) + 1;
}

static void queryLUT(
	ToveShaderGeometryData *data,
	int dim, float y0, float y1,
	std::unordered_set<uint8_t> &result) {

	const float *lut = data->lookupTable + dim;
	const int n = data->lookupTableMeta->n[dim];

	if (n < 1 || y0 >= lut[2 * (n - 1)]) {
		return;
	}

	int lo = 1;
	int hi = n;

	do {
		int mid = (lo + hi) / 2;
		if (lut[2 * (mid - 1)] < y0) {
			lo = mid + 1;
		} else {
			hi = mid;
		}
	} while (lo < hi);

	const uint8_t *base = data->listsTexture;
	const int rowBytes = data->listsTextureRowBytes;

	for (int i = lo - 1; i < n; i++) {
		if (lut[2 * i] > y1) {
			break;
		}

		const uint8_t *list = base + rowBytes * i;
		while (*list != SENTINEL_END) {
			result.insert(*list++);
		}
	}
}

int GeometryShaderLinkImpl::buildLUT(int dim) {
	const bool hasLine = lineColorData.style > 0;
	const float lineWidth = geometryData.strokeWidth;

	auto e = fillEvents.begin();
	auto se = strokeEvents.begin();

	{
		const int n = maxCurves;
		const int dimtwo = 1 - dim;

		assert(fillEvents.size() == n * 6);
		assert(strokeEvents.size() == n * 2);

		for (int i = 0; i < n; i++) {
			const ExtendedCurveData &ext = extended[i];

			if (ext.ignore & IGNORE_FILL) {
				continue;
			}

			const float x0 = ext.bounds[dimtwo + 0];
			const float y0 = ext.bounds[dim + 0];
			const float x1 = ext.bounds[dimtwo + 2];
			const float y1 = ext.bounds[dim + 2];

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

			if (hasLine) {
				se->y = y0 - lineWidth;
				se->t = EVENT_ENTER;
				se->curve = i;
				se++;

				se->y = y1 + lineWidth;
				se->t = EVENT_EXIT;
				se->curve = i;
				se++;
			}

			const ExtendedCurveData::Roots &r = ext.roots[dim];
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

	if (hasLine) {
		std::sort(strokeEvents.begin(), strokeEvents.begin() + numLineEvents,
			[] (const Event &a, const Event &b) {
				return a.y < b.y;
			});

		strokeEventsLUT.build(dim, strokeEvents, numLineEvents, extended);
	}

	std::sort(fillEvents.begin(), fillEvents.begin() + numFillEvents,
		[] (const Event &a, const Event &b) {
			return a.y < b.y;
		});

	if (hasLine) {
		fillEventsLUT.build(dim, fillEvents, numFillEvents, extended, lineWidth,
			[this, dim] (float y0, float y1, const CurveSet &active, uint8_t *list) {
				strokeCurves.clear();
				queryLUT(&strokeShaderData, dim, y0, y1, strokeCurves);

				bool hasStrokeSentinel = false;
				for (auto i = strokeCurves.begin(); i != strokeCurves.end(); i++) {
					const int curveIndex = *i;
					if (active.find(curveIndex) == active.end()) {
						if (!hasStrokeSentinel) {
							*list++ = SENTINEL_STROKES;
							hasStrokeSentinel = true;
						}
						*list++ = curveIndex;
					}
				}

				*list = SENTINEL_END;
			});
	} else {
		fillEventsLUT.build(dim, fillEvents, numFillEvents, extended);
	}

	return numFillEvents;
}

void GeometryShaderLinkImpl::dumpCurveData() {
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

GeometryShaderLinkImpl::GeometryShaderLinkImpl(
	ToveShaderGeometryData &data,
	const ToveShaderColorData &lineColorData,
	int maxCurves) :

	geometryData(data),
	lineColorData(lineColorData),
	maxCurves(maxCurves),
	fillEventsLUT(maxCurves, geometryData, IGNORE_FILL),
	strokeEventsLUT(maxCurves, strokeShaderData, IGNORE_LINE),
	allocData(maxCurves, geometryData),
	allocStrokeData(maxCurves, strokeShaderData) {

	if (maxCurves > 253 || maxCurves < 1) {
		throw std::invalid_argument("only up to 253 curves");
	}

	fillEvents.resize(6 * maxCurves);
	strokeEvents.resize(2 * maxCurves);
	strokeCurves.reserve(maxCurves);
	extended.resize(maxCurves);
}

ToveChangeFlags GeometryShaderLinkImpl::beginUpdate(const PathRef &path, bool initial) {
	if (!initial && path->fetchChanges(CHANGED_GEOMETRY)) {
		return CHANGED_RECREATE;
	}

	const float lineWidth = lineColorData.style > 0 ?
		std::max(path->getLineWidth(), 1.0f) : 0.0f;
	geometryData.strokeWidth = lineWidth;

	geometryData.fillRule = path->getFillRule();

	return 0;
}

int GeometryShaderLinkImpl::endUpdate(const PathRef &path, bool initial) {
	const ToveChangeFlags changes = path->fetchChanges(CHANGED_POINTS);
	if (changes == 0 && !initial) {
		return 0;
	}

	assert(geometryData.lookupTable != nullptr);
	assert(geometryData.lookupTableMeta != nullptr);
	assert(geometryData.lookupTableSize == maxCurves * 4 + 2);

	assert(geometryData.listsTexture != nullptr);
	assert(geometryData.listsTextureRowBytes >=
		geometryData.listsTextureSize[0] * 4);
	assert(geometryData.listsTextureSize[1] == 2 * (maxCurves * 2 + 2));

	assert(geometryData.curvesTexture != nullptr);
	assert(geometryData.curvesTextureRowBytes >=
		geometryData.curvesTextureSize[0] * 2);
	assert(geometryData.curvesTextureSize[1] == maxCurves);

	// (2) build curve data

	int numTrajectories = path->getNumTrajectories();
	int curveIndex = 0;
	for (int i = 0; i < numTrajectories; i++) {
		const TrajectoryRef t = path->getTrajectory(i);
		if (initial || t->fetchChanges() != 0) {
			const int n = t->getNumCurves(false);
			for (int j = 0; j < n; j++) {
				assert(curveIndex < maxCurves);
				t->computeShaderCurveData(
					&geometryData, j, curveIndex, extended[curveIndex]);
				curveIndex++;
			}
			if (n > 0) {
				assert(curveIndex < maxCurves);
				t->computeShaderCloseCurveData(
					&geometryData, curveIndex, extended[curveIndex]);
				curveIndex++;
			}
		} else {
			curveIndex += t->getNumCurves();
		}
	}
	assert(curveIndex == maxCurves);

#if 0
	if (initial) {
		dumpCurveData();
	}
#endif

	float *bounds = geometryData.bounds->bounds;

	for (int dim = 0; dim < 2; dim++) {
		int numEvents = buildLUT(dim);

		if (numEvents > 0) {
			bounds[dim + 0] = fillEvents[0].y;
			bounds[dim + 2] = fillEvents[numEvents - 1].y;
		} else {
			// all curves are empty. just pick the first curve (must exist).
			bounds[dim + 0] = extended[0].bounds[dim + 0];
			bounds[dim + 2] = extended[0].bounds[dim + 2];
		}
	}

	if (lineColorData.style > 0) {
		const float lineWidth = geometryData.strokeWidth;
		bounds[0] -= lineWidth;
		bounds[1] -= lineWidth;
		bounds[2] += lineWidth;
		bounds[3] += lineWidth;
	}

	updateLookupTableMeta(geometryData.lookupTableMeta);

	return changes;
}
