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

AbstractMesh::AbstractMesh() {
	meshData.vertices = NULL;
	meshData.colors = NULL;
	meshData.nvertices = 0;
}

AbstractMesh::~AbstractMesh() {
	if (meshData.colors) {
		free(meshData.colors);
	}
	if (meshData.vertices) {
		free(meshData.vertices);
	}
}

const ToveMeshRecord &AbstractMesh::getData() const {
	return meshData;
}

ToveIndex16Array AbstractMesh::getTriangles() const {
	return triangles.get();
}

float *AbstractMesh::vertices(int from, int n) {
	if (from + n > meshData.nvertices) {
		meshData.nvertices = from + n;
	    meshData.vertices = static_cast<float*>(realloc(
	    	meshData.vertices, nextpow2(meshData.nvertices) * 2 * sizeof(float)));
	}
    return &meshData.vertices[2 * from];
}

void AbstractMesh::cache(bool keyframe) {
	triangles.cache(keyframe);
}

void AbstractMesh::clear() {
	triangles.clear();
	meshData.nvertices = 0;
}

void AbstractMesh::triangulate(const ClipperPaths &paths, ToveHoles holes) {
	std::list<TPPLPoly> polys;
	for (int i = 0; i < paths.size(); i++) {
		const int n = paths[i].size();
		TPPLPoly poly;
		poly.Init(n);
		int index = meshData.nvertices;
 	   	float *vertex = vertices(index, n);

		for (int j = 0; j < n; j++) {
			const ClipperPoint &p = paths[i][j];
			*vertex++ = p.X;
			*vertex++ = p.Y;
			poly[j].x = p.X;
			poly[j].y = p.Y;
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
	triangles.add(tris);
}

void AbstractMesh::clearTriangles() {
	triangles.clear();
}

void AbstractMesh::triangulateLine(
	int v0,
	const PathRef &path,
	const FixedFlattener &flattener) {

	const int numSubpaths = path->getNumSubpaths();
	int i0 = 1 + v0;
	for (int t = 0; t < numSubpaths; t++) {
		const bool closed = path->getSubpath(t)->isClosed();

		const int n = path->getSubpathSize(t, flattener);
		uint16_t *indices = triangles.allocate(2 * (n - 1) + (closed ? 2 : 0));

		for (int i = 0; i < n - 1; i++) {
			*indices++ = i0 + i;
			*indices++ = i0 + i + 1;
			*indices++ = i0 + i + n;

			*indices++ = i0 + i + n;
			*indices++ = i0 + i + 1;
			*indices++ = i0 + i + n + 1;
		}

		if (closed) {
			*indices++ = i0 + n - 1;
			*indices++ = i0 + 0;
			*indices++ = i0 + (n - 1) + n;

			*indices++ = i0 + (n - 1) + n;
			*indices++ = i0 + 0;
			*indices++ = i0 + n;
		}

		i0 += 2 * n;
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

	   	const float *vertex = vertices(vertexIndex, n);

		TPPLPoly poly;
		poly.Init(n);

		int next = 0;
		int written = 0;

		for (int j = 0; j < n; j++) {
			// duplicated points are a very common issue that breaks the
			// complete triangulation, that's why we special case here.
			if (j == next) {
				poly[written].x = vertex[2 * j + 0];
				poly[written].y = vertex[2 * j + 1];
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

	triangles.add(triangulation);
}

void AbstractMesh::add(const ClipperPaths &paths, const MeshPaint &paint, const ToveHoles holes) {
	const int vertexIndex = meshData.nvertices;
	triangulate(paths, holes);
	const int vertexCount = meshData.nvertices - vertexIndex;
	addColor(vertexIndex, vertexCount, paint);
}


void Mesh::initializePaint(MeshPaint &paint, const NSVGpaint &nsvg, float opacity, float scale) {
}

void Mesh::addColor(int vertexIndex, int vertexCount, const MeshPaint &paint) {
}


void ColorMesh::initializePaint(MeshPaint &paint, const NSVGpaint &nsvg, float opacity, float scale) {
	paint.initialize(nsvg, opacity, scale);
}

void ColorMesh::addColor(int vertexIndex, int vertexCount, const MeshPaint &paint) {
    meshData.colors = static_cast<uint8_t*>(realloc(
    	meshData.colors, nextpow2(vertexIndex + vertexCount) * 4 * sizeof(uint8_t)));

    if (!meshData.colors) {
		TOVE_BAD_ALLOC();
		return;
    }

    uint8_t *colors = &meshData.colors[4 * vertexIndex];

    switch (paint.getType()) {
    	case NSVG_PAINT_LINEAR_GRADIENT: {
	 	   	float *vertex = vertices(vertexIndex, vertexCount);
	 	   	for (int i = 0; i < vertexCount; i++) {
	    		const float x = *vertex++;
	    		const float y = *vertex++;

				const int color = paint.getLinearGradientColor(x, y);
				*colors++ = (color >> 0) & 0xff;
				*colors++ = (color >> 8) & 0xff;
				*colors++ = (color >> 16) & 0xff;
				*colors++ = (color >> 24) & 0xff;
			}
		} break;

		case NSVG_PAINT_RADIAL_GRADIENT: {
	 	   	float *vertex = vertices(vertexIndex, vertexCount);
	 	   	for (int i = 0; i < vertexCount; i++) {
	    		const float x = *vertex++;
	    		const float y = *vertex++;

				const int color = paint.getRadialGradientColor(x, y);
				*colors++ = (color >> 0) & 0xff;
				*colors++ = (color >> 8) & 0xff;
				*colors++ = (color >> 16) & 0xff;
				*colors++ = (color >> 24) & 0xff;
			}
		} break;

		default: {
	    	const int color = paint.getColor();

		    const int r = (color >> 0) & 0xff;
		    const int g = (color >> 8) & 0xff;
		    const int b = (color >> 16) & 0xff;
		    const int a = (color >> 24) & 0xff;

	 	   	float *vertex = vertices(vertexIndex, vertexCount);
	 	   	for (int i = 0; i < vertexCount; i++) {
				*colors++ = r;
				*colors++ = g;
				*colors++ = b;
				*colors++ = a;
		    }
 		} break;
    }
}
