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

// purely for debugging purposes, DEBUG_EARCUT enables mapbox's
// earcut triangulator; this is much slower than the default and
// does not support caching triangulations.
#define DEBUG_EARCUT 0

#if DEBUG_EARCUT
#include <array>
#include "../../thirdparty/earcut/earcut.hpp"
#endif

static void dumpPolygons(const std::list<TPPLPoly> &polys) {
	for (auto p = polys.begin(); p != polys.end(); p++) {
		printf("poly:\n");
		for (int i = 0; i < p->GetNumPoints(); i++) {
			printf("%f %f\n", (*p)[i].x, (*p)[i].y);
		}
	}
}

inline void triangulationFailed(const std::list<TPPLPoly> &polys) {
#if 0
	printf("triangulation failed:\n");
	dumpPolygons(polys);
#endif
	TOVE_WARN("Triangulation failed.");
}

static void applyHoles(ToveHoles mode, TPPLPoly &poly) {
	switch (mode) {
		case HOLES_NONE:
			if (poly.GetOrientation() == TPPL_CW) {
				poly.Invert();
			}
			break;
		case HOLES_CW:
			if (poly.GetOrientation() == TPPL_CW) {
				poly.SetHole(true);
			}
			break;
		case HOLES_CCW:
			poly.Invert();
			if (poly.GetOrientation() == TPPL_CW) {
				poly.SetHole(true);
			}
			break;
	}
}

AbstractMesh::AbstractMesh(uint16_t stride) : mStride(stride) {
	mVertices = nullptr;
	mVertexCount = 0;
}

AbstractMesh::~AbstractMesh() {
	if (mVertices) {
		free(mVertices);
	}
	for (auto i : mSubmeshes) {
		delete i.second;
	}
}

ToveTriangles AbstractMesh::getTriangles() const {
	const int n = mSubmeshes.size();
	if (n < 1) {
		return ToveTriangles {
			TRIANGLES_LIST,
			nullptr,
			0
		};
	} else if (n == 1) {
		return mSubmeshes.begin()->second->getTriangles();
	} else {
		// subtle point: mSubmeshes needs to be ordered (e.g.
		// a map here) otherwise our triangle order would be
		// messed up, resulting in wrong visuals.

		size_t k = 0;
		for (auto submesh : mSubmeshes) {
			k += submesh.second->getTriangles().size;
		}
		mCoalescedTriangles.resize(k);

		size_t offset = 0;
		for (auto submesh : mSubmeshes) {
			const ToveTriangles t =
				submesh.second->getTriangles();
			assert(t.mode == TRIANGLES_LIST);
			std::memcpy(
				&mCoalescedTriangles[offset],
				t.array,
				t.size * sizeof(ToveVertexIndex));
			offset += t.size;
		}

		return ToveTriangles {
			TRIANGLES_LIST,
			mCoalescedTriangles.data(),
			int(mCoalescedTriangles.size())
		};
	}
}

void AbstractMesh::reserve(uint32_t n) {
	if (n > mVertexCount) {
		mVertexCount = n;
	    mVertices = realloc(
	    	mVertices,
			nextpow2(mVertexCount) * mStride);
	}
}

void AbstractMesh::cache(bool keyframe) {
	for (auto submesh : mSubmeshes) {
		submesh.second->cache(keyframe);
	}
}

void AbstractMesh::clear() {
	mVertexCount = 0;
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

ToveTriangles Submesh::getTriangles() const {
	return mTriangles.get();
}

void Submesh::cache(bool keyframe) {
	mTriangles.cache(keyframe);
}

void Submesh::addClipperPaths(
	const ClipperPaths &paths,
	float scale,
	ToveHoles holes) {

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

		applyHoles(holes, poly);
		polys.push_back(poly);
	}

	TPPLPartition partition;
	std::list<TPPLPoly> triangles;
	if (partition.Triangulate_MONO(&polys, &triangles) == 0) {
		if (partition.Triangulate_EC(&polys, &triangles) == 0) {
			triangulationFailed(polys);
			return;
		}
	}

	mTriangles.add(triangles);
#endif
}

void Submesh::clearTriangles() {
	mTriangles.clear();
}

static void stripToList(
	const uint16_t *in,
	uint16_t *out,
	int triangleCount) {

	if (triangleCount > 0) {
		uint16_t i0 = *in++;
		uint16_t i1 = *in++;
		for (int i = 0; i < triangleCount; i++) {
			uint16_t i2 = *in++;
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

void Submesh::triangulateFixedLine(
	int v0,
	bool miter,
	const PathRef &path,
	const FixedFlattener &flattener) {

	const int numSubpaths = path->getNumSubpaths();
	int i0 = 1 + v0; // 1-based for love2d

	for (int t = 0; t < numSubpaths; t++) {
		const bool closed = path->getSubpath(t)->isClosed();

		const int numVertices = path->getSubpathSize(t, flattener);
		if (numVertices < 2) {
			TOVE_WARN("cannot render line with npts < 2");
			continue;
		}
		const int numSegments = numVertices - 1;

		uint16_t *indices;
		const int num0 = 4 * numSegments + (numSegments - 1) * (miter ? 1 : 0);
		const int numIndices = num0 + (closed ? 2 + (miter ? 1 : 0) : 0);
		std::vector<uint16_t> tempIndices;

		if (true) { //mTriangles.hasMode(TRIANGLES_LIST) || numSubpaths > 1) {
			// this only happens for compound (flat) meshes and
			// multiple subpaths (we don't want them connected).
			tempIndices.resize(numIndices);
			indices = tempIndices.data();
		} else {
			// we have our own mesh. use triangle strips.
			indices = mTriangles.allocate(
				TRIANGLES_STRIP, numIndices);
		}

		int j = i0 + (miter ? 1 : 0);

		for (int i = 0; i < num0; i++) {
			*indices++ = j++;
		}

		if (closed) {
			if (miter) {
				*indices++ = i0; // miter point
			}
			*indices++ = i0 + 0;
			*indices++ = i0 + 1;
		}

		if (!tempIndices.empty()) {
			const int triangleCount = numIndices - 2;
			stripToList(tempIndices.data(), mTriangles.allocate(
				TRIANGLES_LIST, triangleCount), triangleCount);
		}

		i0 += (miter ? 5 : 4) * numVertices; // advance vertex index
	}
}

void Submesh::triangulateFixedFill(
	const int vertexIndex0,
	const PathRef &path,
	const FixedFlattener &flattener,
	const ToveHoles holes) {

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
	std::list<TPPLPoly> polys;
   	int vertexIndex = vertexIndex0;

	for (int i = 0; i < numSubpaths; i++) {
		const int n = path->getSubpathSize(i, flattener);
		if (n < 3) {
			continue;
		}

	   	const auto vertex = vertices(vertexIndex, n);

		TPPLPoly poly;
		poly.Init(n);

		int written = 0;
		for (int j = 0; j < n; ) {
			// duplicate points are a very common issue that breaks the
			// complete triangulation, that's why we special case here.
			poly[written].x = vertex[j].x;
			poly[written].y = vertex[j].y;
			poly[written].id = vertexIndex + j;
			written++;

			const int next = find_unequal_forward(vertex, j, n);
			if (next < j) {
				break;
			}
			j = next;

			if (j == n - 1 && !unequal(
				vertex[0].x, vertex[0].y,
				vertex[j].x, vertex[j].y)) {
				break;
			}
		}
		vertexIndex += n;

		if (written < n) {
			// there were indeed duplicate points. we need to copy, alas.
			TPPLPoly poly2;
			poly2.Init(written);
			for (int j = 0; j < written; j++) {
				poly2[j] = poly[j];
			}
			poly = poly2;
		}

		applyHoles(holes, poly);
		polys.push_back(poly);
	}

	TPPLPartition partition;

	std::list<TPPLPoly> convex;
	if (partition.ConvexPartition_HM(&polys, &convex) == 0) {
		TOVE_WARN("ConvexPartition_HM failed.");
		return;
	}

	Triangulation *triangulation = new Triangulation(convex);
	for (auto i = convex.begin(); i != convex.end(); i++) {
		std::list<TPPLPoly> triangles;
		TPPLPoly &p = *i;

		if (partition.Triangulate_MONO(&p, &triangles) == 0) {
			if (partition.Triangulate_EC(&p, &triangles) == 0) {
				triangulationFailed(polys);
				continue;
			}
		}

		triangulation->triangles.add(triangles);
	}

	mTriangles.add(triangulation);
#endif
}

void AbstractMesh::setLineColor(
	const PathRef &path, int vertexIndex, int vertexCount) {
}

void AbstractMesh::setFillColor(
	const PathRef &path, int vertexIndex, int vertexCount) {
}

Submesh *AbstractMesh::submesh(const PathRef &path, int line) {
	const SubmeshId id = path->getIndex() * 2 + line;
	const auto i = mSubmeshes.find(id);
	if (i != mSubmeshes.end()) {
		return i->second;
	} else {
		Submesh *submesh = new Submesh(this);
		mSubmeshes[id] = submesh;
		return submesh;
	}
}


Mesh::Mesh() : AbstractMesh(sizeof(float) * 2) {
}


ColorMesh::ColorMesh() : AbstractMesh(sizeof(float) * 2 + 4) {
}

void ColorMesh::setLineColor(
	const PathRef &path, int vertexIndex, int vertexCount) {
	MeshPaint paint;
	NSVGshape *shape = path->getNSVG();
	paint.initialize(shape->stroke, shape->opacity, 1.0f);
	setColor(vertexIndex, vertexCount, paint);
}

void ColorMesh::setFillColor(
	const PathRef &path, int vertexIndex, int vertexCount) {
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


PaintMesh::PaintMesh() : AbstractMesh(sizeof(float) * 3) {
}

void PaintMesh::setLineColor(
	const PathRef &path, int vertexIndex, int vertexCount) {

	setPaintIndex(2 * path->getIndex() + 0, vertexIndex, vertexCount);
}

void PaintMesh::setFillColor(
	const PathRef &path, int vertexIndex, int vertexCount) {

	setPaintIndex(2 * path->getIndex() + 1, vertexIndex, vertexCount);
}

void PaintMesh::setPaintIndex(
	int paintIndex, int vertexIndex, int vertexCount) {

	auto vertex = vertices(vertexIndex, vertexCount);
	const float value = paintIndex;

	for (int i = 0; i < vertexCount; i++) {
		float *p = reinterpret_cast<float*>(vertex.attr());
		*p = value;
		vertex++;
	}
}
