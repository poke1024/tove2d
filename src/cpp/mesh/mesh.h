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

#ifndef __TOVE_MESH
#define __TOVE_MESH 1

#include "triangles.h"
#include "paint.h"
#include "flatten.h"

typedef struct {
	float *vertices;
	uint8_t *colors;
	int nvertices;
} ToveMeshRecord;

class AbstractMesh {
protected:
	ToveMeshRecord meshData;
	TriangleCache triangles;

public:
	AbstractMesh();
	~AbstractMesh();

	float *vertices(int from, int n);
	void cache(bool keyframe);
	void clear();

	void triangulate(const ClipperPaths &paths, ToveHoles holes);
	void addTriangles(const std::list<TPPLPoly> &triangles);
	void clearTriangles();
	void triangulateFill(const int vertexIndex0, const PathRef &path, const FixedFlattener &flattener, ToveHoles holes);
	void triangulateLine(int v0, const PathRef &path, const FixedFlattener &flattener);

	virtual void initializePaint(MeshPaint &paint, const NSVGpaint &nsvg, float opacity, float scale) = 0;
	virtual void addColor(int vertexIndex, int vertexCount, const MeshPaint &paint) = 0;
	void add(const ClipperPaths &paths, const MeshPaint &paint, const ToveHoles holes);

	const ToveMeshRecord &getData() const;
	ToveIndex16Array getTriangles() const;

	inline bool checkTriangles(bool &triangleCacheFlag) {
		return triangles.check(meshData.vertices, triangleCacheFlag);
	}
};

class Mesh : public AbstractMesh {
public:
	virtual void initializePaint(MeshPaint &paint, const NSVGpaint &nsvg, float opacity, float scale);
	virtual void addColor(int vertexIndex, int vertexCount, const MeshPaint &paint);
};

class ColorMesh : public AbstractMesh {
public:
	virtual void initializePaint(MeshPaint &paint, const NSVGpaint &nsvg, float opacity, float scale);
	virtual void addColor(int vertexIndex, int vertexCount, const MeshPaint &paint);
};

#endif // __TOVE_MESH
