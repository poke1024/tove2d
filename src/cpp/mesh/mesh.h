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
#include "utils.h"

class FixedFlattener;

class AbstractMesh {
protected:
	void *mVertices;
	uint32_t mVertexCount;
	const uint16_t mStride;
	TriangleCache mTriangles;

	void reserve(uint32_t n);

public:
	AbstractMesh(uint16_t stride);
	~AbstractMesh();

	inline Vertices vertices(int from, int n) {
		if (from + n > mVertexCount) {
			reserve(from + n);
		}
		return Vertices(mVertices, mStride, from);
	}

	void cache(bool keyframe);
	void clear();

	void triangulate(const ClipperPaths &paths, float scale, ToveHoles holes);
	void addTriangles(const std::list<TPPLPoly> &triangles);
	void clearTriangles();
	void triangulateFill(const int vertexIndex0,
		const PathRef &path, const FixedFlattener &flattener, ToveHoles holes);
	void triangulateLine(int v0, bool miter,
		const PathRef &path, const FixedFlattener &flattener);

	virtual void initializePaint(MeshPaint &paint,
		const NSVGpaint &nsvg,
		float opacity,
		float scale) = 0;
	virtual void addColor(int vertexIndex, int vertexCount,
		const MeshPaint &paint) = 0;
	void add(const ClipperPaths &paths, float scale,
		const MeshPaint &paint, const ToveHoles holes);

	ToveTriangles getTriangles() const;

	inline int getVertexCount() const {
		return mVertexCount;
	}

	inline void copyVertexData(void *buffer, size_t bufferByteSize) {
		const size_t size = mStride * mVertexCount;
		assert(bufferByteSize == size);
		std::memcpy(buffer, mVertices, size);
	}

	inline bool checkTriangles(bool &triangleCacheFlag) {
		return mTriangles.check(
			Vertices(mVertices, mStride), triangleCacheFlag);
	}
};

class Mesh : public AbstractMesh {
public:
	Mesh();
	virtual void initializePaint(MeshPaint &paint, const NSVGpaint &nsvg, float opacity, float scale);
	virtual void addColor(int vertexIndex, int vertexCount, const MeshPaint &paint);
};

class ColorMesh : public AbstractMesh {
public:
	ColorMesh();
	virtual void initializePaint(MeshPaint &paint, const NSVGpaint &nsvg, float opacity, float scale);
	virtual void addColor(int vertexIndex, int vertexCount, const MeshPaint &paint);
};

#endif // __TOVE_MESH
