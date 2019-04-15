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

#include "../common.h"
#include "mesh.h"
#include "../path.h"
#if TOVE_DEBUG
#include <iostream>
#endif

BEGIN_TOVE_NAMESPACE

// purely for debugging purposes, DEBUG_EARCUT enables mapbox's
// earcut triangulator; this is much slower than the default and
// does not support caching triangulations.
#define DEBUG_EARCUT 0

#if DEBUG_EARCUT
#include <array>
#include "../../thirdparty/earcut/earcut.hpp"
#endif

#if TOVE_DEBUG
static void dumpPolygons(std::ostream &os, const std::list<TPPLPoly> &polys) {
	int poly_i = 0;
	for (auto p = polys.begin(); p != polys.end(); p++) {
		os << "Polygon " << ++poly_i << ":" << std::endl;
		for (int i = 0; i < p->GetNumPoints(); i++) {
			os << (*p)[i].x << " " <<  (*p)[i].y << std::endl;
		}
	}
}
#endif

inline void triangulationFailed(const std::list<TPPLPoly> &polys) {
#if TOVE_DEBUG
	std::cerr << "Triangulation failed:" << std::endl;
	dumpPolygons(std::cerr, polys);
#endif
	tove::report::warn("triangulation failed.");
}

AbstractMesh::AbstractMesh(const NameRef &name, uint16_t stride) :
	mVertices(nullptr),
	mVertexCount(0),
	mOwnsBuffer(true),
	mName(name),
	mStride(stride) {
}

AbstractMesh::~AbstractMesh() {
	if (mOwnsBuffer && mVertices) {
		free(mVertices);
		mVertices = nullptr;
	}
	for (auto i : mSubmeshes) {
		delete i.second;
	}
}

ToveTrianglesMode AbstractMesh::getIndexMode() const {
	if (mSubmeshes.size() == 1) {
		return mSubmeshes.begin()->second->getIndexMode();
	} else {
		return TRIANGLES_LIST;
	}
}

int32_t AbstractMesh::getIndexCount() const {
	int32_t k = 0;
	for (auto submesh : mSubmeshes) {
		k += submesh.second->getIndexCount();
	}
	return k;
}

void AbstractMesh::copyIndexData(
	ToveVertexIndex *indices,
	int32_t indexCount) const {

	const int n = mSubmeshes.size();
	if (n == 1) {
		mSubmeshes.begin()->second->copyIndexData(
			indices, indexCount);
	} else {
		// subtle point: mSubmeshes needs to be ordered (e.g.
		// a map here) otherwise our triangle order would be
		// messed up, resulting in wrong visuals.

		int32_t offset = 0;
		for (auto submesh : mSubmeshes) {
			Submesh *m = submesh.second;
			m->copyIndexData(
				indices + offset, indexCount - offset);
			offset += m->getIndexCount();
		}
	}
}

void AbstractMesh::setNewExternalVertexBuffer(
	void *buffer,
	size_t bufferByteSize) {

	if (mVertices) {
		std::memcpy(
			buffer,
			mVertices,
			std::min(bufferByteSize, size_t(mVertexCount * mStride)));
	}
	
	if (mOwnsBuffer && mVertices) {
		free(mVertices);
		mVertices = nullptr;
	}

	mVertices = buffer;
	mVertexCount = bufferByteSize / mStride;
	mOwnsBuffer = false;
}

void AbstractMesh::reserve(int32_t n) {
	if (n > mVertexCount) {
		void *previousBuffer = nullptr;
		size_t previousCount = 0;

		if (!mOwnsBuffer) {
			previousBuffer = mVertices;
			previousCount = mVertexCount;

			mVertices = nullptr;
			mOwnsBuffer = true;
		}

		mVertexCount = n;

	    mVertices = realloc(
	    	mVertices,
			nextpow2(mVertexCount) * mStride);

		if (!mVertices) {
			TOVE_FATAL("out of memory during vertex allocation");
		}

		if (previousBuffer) {
			std::memcpy(
				mVertices, previousBuffer, previousCount * mStride);
		}
	}
}

void AbstractMesh::cacheKeyFrame() {
	for (auto submesh : mSubmeshes) {
		submesh.second->cacheKeyFrame();
	}
}

void AbstractMesh::setCacheSize(int size) {
	for (auto submesh : mSubmeshes) {
		submesh.second->setCacheSize(size);
	}
}

void AbstractMesh::clear(bool ensureOwnBuffer) {
	mVertexCount = 0;
	if (ensureOwnBuffer && !mOwnsBuffer) {
		mVertices = nullptr;
		mOwnsBuffer = true;
	}
	for (auto submesh : mSubmeshes) {
		delete submesh.second;
	}
	mSubmeshes.clear();
}

void AbstractMesh::clearTriangles() {
	for (auto submesh : mSubmeshes) {
		submesh.second->clearTriangles();
	}
}

void Submesh::cacheKeyFrame() {
	mTriangles.cacheKeyFrame();
}

void Submesh::setCacheSize(int size) {
	mTriangles.setCacheSize(size);
}

void Submesh::addClipperPaths(
	const ClipperPaths &paths,
	float scale) {

#if DEBUG_EARCUT
	using Point = std::array<float, 2>;
	std::vector<std::vector<Point>> polygon;
	polygon.reserve(paths.size());

	const ToveVertexIndex i0 = mMesh->getVertexCount();
	for (const ClipperPath &path : paths) {
		const int n = path.size();
 	   	auto v = vertices(mMesh->getVertexCount(), n);
		std::vector<Point> subpath;
		subpath.reserve(n);

		for (int j = 0; j < n; j++) {
			const ClipperPoint &p = path[j];

			const float x = p.X / scale;
			const float y = p.Y / scale;

			v->x = x;
			v->y = y;
			v++;
			subpath.push_back({x, y});
		}

		polygon.push_back(std::move(subpath));
	}
	const std::vector<ToveVertexIndex> indices =
		mapbox::earcut<ToveVertexIndex>(polygon);
	mTriangles.add(indices, i0);
#else
	std::list<TPPLPoly> polys;
	for (const ClipperPath &path : paths) {
		const int n = path.size();
		TPPLPoly poly;
		poly.Init(n);
		int index = mMesh->getVertexCount();
 	   	auto v = vertices(index, n);

		for (int j = 0; j < n; j++) {
			const ClipperPoint &p = path[j];

			const float x = p.X / scale;
			const float y = p.Y / scale;

			v->x = x;
			v->y = y;
			v++;

			poly[j].x = x;
			poly[j].y = y;
			poly[j].id = index++;
		}

		if (poly.GetOrientation() == TPPL_CW) {
			poly.SetHole(true);
		}

		polys.push_back(poly);
	}

	TPPLPartition partition;
	std::list<TPPLPoly> triangles;
	//if (partition.Triangulate_MONO(&polys, &triangles) == 0) {
		if (partition.Triangulate_EC(&polys, &triangles) == 0) {
			triangulationFailed(polys);
			return;
		}
	//}

	mTriangles.add(triangles);
#endif
}

void Submesh::clearTriangles() {
	mTriangles.clear();
}

static void stripRangeToList(
	int index,
	uint16_t *out,
	int triangleCount) {

	if (triangleCount > 0) {
		int i0 = index++;
		int i1 = index++;
		for (int i = 0; i < triangleCount; i++) {
			const int i2 = index++;
			if (i & 1) {
				*out++ = i1;
				*out++ = i0;
				*out++ = i2;
			} else {
				*out++ = i0;
				*out++ = i1;
				*out++ = i2;
			}
			i0 = i1;
			i1 = i2;
		}
	}
}

#if TOVE_RT_CLIP_PATH
static uint16_t *reducedOverlapTriangles(
	const int subpathVertex,
	const int numVertices,
	const bool miter,
	const bool closed,
	const int stride,
	uint16_t *data) {

	if (numVertices < 1) {
		return data;
	}

	for (int edge = 0; edge < 2; edge++) {
		int middle = subpathVertex + (miter ? 1 : 0);
		int outer = middle + (edge == 0 ? 2 : 1);

		// outer
		if (closed) {
			const int last = stride * (numVertices - 1);

			// bevel
			*data++ = outer + last;
			*data++ = middle + last;
			*data++ = outer;

			*data++ = middle + last;
			*data++ = middle;
			*data++ = outer;

			/**data++ = outer;
			*data++ = middle;
			*data++ = outer + 2;*/
		}

		for (int i = 0; i < numVertices - 1; i++) {
			if (i > 0) {
				// bevel
				*data++ = outer; outer += stride - 2;
				*data++ = middle;
				*data++ = outer;
			}

			*data++ = middle; middle += stride;
			*data++ = middle;
			*data++ = outer;

			*data++ = outer; outer += 2;
			*data++ = middle;
			*data++ = outer;
		}
	}

	return data;
}
#endif

void Submesh::triangulateFixedResolutionLine(
	const int pathVertex,
	const bool miter,
	const bool reduceOverlap,
	const int verticesPerSegment,
	const PathRef &path,
	const RigidFlattener &flattener) {

	const int numSubpaths = path->getNumSubpaths();
	int subpathVertex = ToLoveVertexMapIndex(pathVertex);

	mTriangles.clear();

	for (int t = 0; t < numSubpaths; t++) {
		const int numVertices = path->getSubpathSize(t, flattener);
		if (numVertices < 2) {
			tove::report::warn("cannot render line with less than 2 vertices.");
		} else {
			const bool closed = path->getSubpath(t)->isClosed();
			const bool skipped = !closed && miter ? 1 : 0;

			const int numSegments = numVertices - (closed ? 0 : 1);
			const int numIndices = verticesPerSegment * numSegments - skipped;

			const int firstIndex = subpathVertex + skipped;

			if (mTriangles.hasMode(TRIANGLES_STRIP)) {
				if (reduceOverlap) {
					tove::report::warn("internal error. reduce-overlap on tri strip.");
					break;
				}

				// we have our own separate mesh just for lines. use triangle strips.
				uint16_t *indices = mTriangles.allocate(
					TRIANGLES_STRIP, numIndices);

				for (int i = 0, j = firstIndex; i < numIndices; i++) {
					*indices++ = j++;
				}
			} else if (!reduceOverlap) {
				const int triangleCount = numIndices - 2;

				stripRangeToList(firstIndex, mTriangles.allocate(
					TRIANGLES_LIST, triangleCount), triangleCount);
			} else {
#if TOVE_RT_CLIP_PATH
				// reduced overlap variant.

				const int n = 2 * (3 * (numVertices - 1) - 1 + (closed ? 3 - 1 : 0));

				uint16_t *data = mTriangles.allocate(TRIANGLES_LIST, n);

				uint16_t *end = reducedOverlapTriangles(
					subpathVertex, numVertices, miter, closed, verticesPerSegment, data);

				assert((end - data) == n * 3);
#else
				assert(false);
#endif
			}
		}

		subpathVertex += numVertices * verticesPerSegment;
	}
}

static ClipperLib::Path toClipperPath(SubpathCleaner &c, IntVertexMap &m, float s) {
	const int n = c.size();
	const auto &pts = c.getPoints();
	const auto &indices = c.getIndices();

	ClipperLib::Path path;
	path.reserve(n);
	for (int i = 0; i < n; i++) {
		const ClipperLib::IntPoint p(pts[i].x * s, pts[i].y * s);
		m[p] = indices[i];
		path.push_back(p);
	}
	return path;
}


void Submesh::triangulateFixedResolutionFill(
	const int vertexIndex0,
	const PathRef &path,
	const RigidFlattener &flattener) {

	const int numSubpaths = path->getNumSubpaths();

#if DEBUG_EARCUT
	using Point = std::array<float, 2>;
	std::vector<std::vector<Point>> polygon;
	polygon.reserve(numSubpaths);
	int vertexIndex = vertexIndex0;

	for (int i = 0; i < numSubpaths; i++) {
		const int n = path->getSubpathSize(i, flattener);
		const auto vertex = vertices(vertexIndex, n);
		vertexIndex += n;

		std::vector<Point> subpath;
		subpath.reserve(n);
		for (int j = 0; j < n; j++) {
			subpath.push_back({vertex[j].x, vertex[j].y});
		}
		polygon.push_back(std::move(subpath));
	}

	if (numSubpaths == 1) {
		// since we cannot store arbitrary indices, we can only
		// remove duplicates points that are strictly at the end.
		auto &p = polygon[0];
		while (p.size() > 3) {
			const int n = p.size() - 1;
			if (unequal(
				p[0][0], p[0][1],
				p[n][0], p[n][1])) {
				break;
			}
			p.erase(p.end() - 1);
		}
	}

	const std::vector<ToveVertexIndex> indices =
		mapbox::earcut<ToveVertexIndex>(polygon);
	mTriangles.add(indices, vertexIndex0);
#else
   	int vertexIndex = vertexIndex0;

	int maxSubpathSize = 0;
	int numTotalPoints = 0;
	for (int i = 0; i < numSubpaths; i++) {
		const int n = path->getSubpathSize(i, flattener);
		numTotalPoints += n;
		maxSubpathSize = std::max(maxSubpathSize, n);
	}

	mCleaner.init(maxSubpathSize, numTotalPoints);

	IntVertexMap vertexMap;
	const float clipperScale = 1000.0f;
	ClipperLib::Paths clipperPaths;

	for (int i = 0; i < numSubpaths; i++) {
		const int n = path->getSubpathSize(i, flattener);
		if (n < 3) {
			vertexIndex += n;
			continue;
		}

	   	const auto vertex = vertices(vertexIndex, n);

		mCleaner.clear();
		for (int j = 0; j < n; j++) {
			mCleaner.add(vertex[j].x, vertex[j].y, vertexIndex + j);
		}
		mCleaner.clean();

		vertexIndex += n;

		clipperPaths.push_back(toClipperPath(
			mCleaner, vertexMap, clipperScale));
	}

	ClipperLib::SimplifyPolygons(
		clipperPaths, path->getClipperFillType());

	std::list<TPPLPoly> polys;

	for (const auto &path : clipperPaths) {
		polys.push_back(TPPLPoly());
		TPPLPoly &poly = polys.back();

		const int n = path.size();
		poly.Init(n);
		for (int i = 0; i < n; i++) {
			const auto &p = path[i];
			poly[i].x = p.X / clipperScale;
			poly[i].y = p.Y / clipperScale;
			const auto it = vertexMap.find(p);
			if (it != vertexMap.end()) {
				poly[i].id = it->second;
			} else {
				poly[i].id = -1;
			}
		}

		if (poly.GetOrientation() == TPPL_CW) {
			poly.SetHole(true);
		}
	}

	TPPLPartition partition;

	std::list<TPPLPoly> convex;
	if (partition.ConvexPartition_HM(&polys, &convex) == 0) {
		tove::report::warn("triangulation (ConvexPartition_HM) failed.");
		return;
	}

	std::unique_ptr<Triangulation> triangulation(new Triangulation(
		convex, mCleaner.fetchVanishing()));
	for (auto i = convex.begin(); i != convex.end(); i++) {
		std::list<TPPLPoly> triangles;
		TPPLPoly &p = *i;

		//if (partition.Triangulate_MONO(&p, &triangles) == 0) {
			if (partition.Triangulate_EC(&p, &triangles) == 0) {
				triangulationFailed(polys);
				continue;
			}
		//}

		triangulation->triangles.add(triangles);
	}

	mTriangles.addAndMakeCurrent(triangulation.release());
#endif
}

void AbstractMesh::setLineColor(
	const PathRef &path,
	const PathPaintInd &paintInd,
	const int vertexIndex,
	const int vertexCount) {
}

void AbstractMesh::setFillColor(
	const PathRef &path,
	const PathPaintInd &paintInd,
	const int vertexIndex,
	const int vertexCount) {
}

Submesh *AbstractMesh::submesh(int pathIndex, int line) {
	const SubmeshId id = pathIndex * 2 + line;
	const auto i = mSubmeshes.find(id);
	if (i != mSubmeshes.end()) {
		return i->second;
	} else {
		Submesh *submesh = new Submesh(this);
		mSubmeshes[id] = submesh;
		return submesh;
	}
}


Mesh::Mesh(const NameRef &name) : AbstractMesh(name, sizeof(float) * 2) {
}


ColorMesh::ColorMesh(const NameRef &name) : AbstractMesh(name, sizeof(float) * 2 + 4) {
}

void ColorMesh::setLineColor(
	const PathRef &path,
	const PathPaintInd &paintInd,
	const int vertexIndex,
	const int vertexCount) {

	MeshPaint paint;
	NSVGshape *shape = path->getNSVG();
	paint.initialize(shape->stroke, shape->opacity, 1.0f);
	setColor(vertexIndex, vertexCount, paint);
}

void ColorMesh::setFillColor(
	const PathRef &path,
	const PathPaintInd &paintInd,
	const int vertexIndex,
	const int vertexCount) {

	MeshPaint paint;
	NSVGshape *shape = path->getNSVG();
	paint.initialize(shape->fill, shape->opacity, 1.0f);
	setColor(vertexIndex, vertexCount, paint);
}

void ColorMesh::setColor(
	int vertexIndex, int vertexCount, const MeshPaint &paint) {

    switch (paint.getType()) {
    	case NSVG_PAINT_LINEAR_GRADIENT: {
	 	   	auto vertex = vertices(vertexIndex, vertexCount);
	 	   	for (int i = 0; i < vertexCount; i++) {
	    		const float x = vertex->x;
	    		const float y = vertex->y;

				const int color = paint.getLinearGradientColor(x, y);
				uint8_t *colors = vertex.attr();
				*colors++ = (color >> 0) & 0xff;
				*colors++ = (color >> 8) & 0xff;
				*colors++ = (color >> 16) & 0xff;
				*colors++ = (color >> 24) & 0xff;

				vertex++;
			}
		} break;

		case NSVG_PAINT_RADIAL_GRADIENT: {
	 	   	auto vertex = vertices(vertexIndex, vertexCount);
	 	   	for (int i = 0; i < vertexCount; i++) {
	    		const float x = vertex->x;
	    		const float y = vertex->y;

				const int color = paint.getRadialGradientColor(x, y);
				uint8_t *colors = vertex.attr();
				*colors++ = (color >> 0) & 0xff;
				*colors++ = (color >> 8) & 0xff;
				*colors++ = (color >> 16) & 0xff;
				*colors++ = (color >> 24) & 0xff;

				vertex++;
			}
		} break;

		default: {
	    	const int color = paint.getColor();

		    const int r = (color >> 0) & 0xff;
		    const int g = (color >> 8) & 0xff;
		    const int b = (color >> 16) & 0xff;
		    const int a = (color >> 24) & 0xff;

			auto vertex = vertices(vertexIndex, vertexCount);
	 	   	for (int i = 0; i < vertexCount; i++) {
				uint8_t *colors = vertex.attr();
				*colors++ = r;
				*colors++ = g;
				*colors++ = b;
				*colors++ = a;

				vertex++;
		    }
 		} break;
    }
}


inline uint32_t flip_endian(uint32_t x) {
	const bool is_big_endian = (*reinterpret_cast<const uint16_t*>("\0\xff") < 0x100);
	if (is_big_endian) {
		return (x >> 24) | ((x << 8) & 0x00FF0000) | ((x >> 8) & 0x0000FF00) | (x << 24);
	} else {
		return x;
	}
}

PaintMesh::PaintMesh(const NameRef &name) : AbstractMesh(name, sizeof(float) * 3) {
}

void PaintMesh::setLineColor(
	const PathRef &path,
	const PathPaintInd &paint,
	const int vertexIndex,
	const int vertexCount) {

	setPaintIndex(paint.line, vertexIndex, vertexCount);
}

void PaintMesh::setFillColor(
	const PathRef &path,
	const PathPaintInd &paint,
	const int vertexIndex,
	const int vertexCount) {

	setPaintIndex(paint.fill, vertexIndex, vertexCount);
}

void PaintMesh::setPaintIndex(
	const PaintIndex &paintIndex,
	const int vertexIndex,
	const int vertexCount) {

	auto vertex = vertices(vertexIndex, vertexCount);
	const uint32_t value = flip_endian(paintIndex.toBytes());

	for (int i = 0; i < vertexCount; i++) {
		*reinterpret_cast<uint32_t*>(vertex.attr()) = value;
		vertex++;
	}
}

END_TOVE_NAMESPACE
