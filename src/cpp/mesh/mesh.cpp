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
}

ToveTriangles AbstractMesh::getTriangles() const {
	return mTriangles.get();
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
	mTriangles.cache(keyframe);
}

void AbstractMesh::clear() {
	mTriangles.clear();
	mVertexCount = 0;
}

void AbstractMesh::triangulate(
	const ClipperPaths &paths, float scale, ToveHoles holes) {

	std::list<TPPLPoly> polys;
	for (int i = 0; i < paths.size(); i++) {
		const int n = paths[i].size();
		TPPLPoly poly;
		poly.Init(n);
		int index = mVertexCount;
 	   	auto v = vertices(index, n);

		for (int j = 0; j < n; j++) {
			const ClipperPoint &p = paths[i][j];

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
			TOVE_WARN("Triangulation failed.");
			return;
		}
	}
	addTriangles(triangles);
}


void AbstractMesh::addTriangles(const std::list<TPPLPoly> &tris) {
	mTriangles.add(tris);
}

void AbstractMesh::clearTriangles() {
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


void AbstractMesh::triangulateLine(
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

		if (mTriangles.hasMode(TRIANGLES_LIST) || numSubpaths > 1) {
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

void AbstractMesh::triangulateFill(
	const int vertexIndex0,
	const PathRef &path,
	const FixedFlattener &flattener,
	const ToveHoles holes) {

	std::list<TPPLPoly> polys;
	//polys.reserve(paths.size());

   	int vertexIndex = vertexIndex0;

	const int numSubpaths = path->getNumSubpaths();

	for (int i = 0; i < numSubpaths; i++) {
		const int n = path->getSubpathSize(i, flattener);
		if (n < 3) {
			continue;
		}

	   	const auto vertex = vertices(vertexIndex, n);

		TPPLPoly poly;
		poly.Init(n);

		int next = 0;
		int written = 0;

		for (int j = 0; j < n; j++) {
			// duplicated points are a very common issue that breaks the
			// complete triangulation, that's why we special case here.
			if (j == next) {
				poly[written].x = vertex[j].x;
				poly[written].y = vertex[j].y;
				poly[written].id = vertexIndex;
				written++;
			}

			vertexIndex++;

			if (j == next) {
				next = find_unequal_forward(vertex, j, n);
			}
		}

		if (written < n) {
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
		TOVE_WARN("Convex partition failed.");
		return;
	}

	Triangulation *triangulation = new Triangulation(convex);

	for (auto i = convex.begin(); i != convex.end(); i++) {
		std::list<TPPLPoly> triangles;
		TPPLPoly &p = *i;

		if (partition.Triangulate_MONO(&p, &triangles) == 0) {
			if (partition.Triangulate_EC(&p, &triangles) == 0) {
#if 0
				printf("failure:\n");
				for (auto p = polys.begin(); p != polys.end(); p++) {
					printf("poly:\n");
					for (int i = 0; i < p->GetNumPoints(); i++) {
						printf("%f %f\n", p->GetPoint(i).x, p->GetPoint(i).y);
					}
				}
#endif

				TOVE_WARN("Triangulation failed.");
				return;
			}
		}

		triangulation->triangles.add(triangles);
	}

	mTriangles.add(triangulation);
}

void AbstractMesh::add(
	const ClipperPaths &paths,
	float scale,
	const MeshPaint &paint,
	const ToveHoles holes) {

	const int vertexIndex = mVertexCount;
	triangulate(paths, scale, holes);
	const int vertexCount = mVertexCount - vertexIndex;
	addColor(vertexIndex, vertexCount, paint);
}


Mesh::Mesh() : AbstractMesh(sizeof(float) * 2) {
}

void Mesh::initializePaint(
	MeshPaint &paint,
	const NSVGpaint &nsvg,
	float opacity,
	float scale) {
}

void Mesh::addColor(
	int vertexIndex,
	int vertexCount,
	const MeshPaint &paint) {
}


ColorMesh::ColorMesh() : AbstractMesh(sizeof(float) * 2 + 4) {
}

void ColorMesh::initializePaint(
	MeshPaint &paint,
	const NSVGpaint &nsvg,
	float opacity,
	float scale) {

	paint.initialize(nsvg, opacity, scale);
}

void ColorMesh::addColor(
	int vertexIndex, int vertexCount, const MeshPaint &paint) {

    /*meshData.colors = static_cast<uint8_t*>(realloc(
    	meshData.colors,
		nextpow2(vertexIndex + vertexCount) * 4 * sizeof(uint8_t)));

    if (!meshData.colors) {
		TOVE_BAD_ALLOC();
		return;
    }

    uint8_t *colors = &meshData.colors[4 * vertexIndex];*/

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
