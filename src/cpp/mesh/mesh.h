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
#include <map>

BEGIN_TOVE_NAMESPACE

class RigidFlattener;

typedef uint32_t SubmeshId;

class Submesh;

class AbstractMesh : public Referencable {
protected:
	void *mVertices;
	uint32_t mVertexCount;
	const uint16_t mStride;
	std::map<SubmeshId, Submesh*> mSubmeshes;
	mutable std::vector<ToveVertexIndex> mCoalescedTriangles;

	void reserve(uint32_t n);

public:
	AbstractMesh(uint16_t stride);
	virtual ~AbstractMesh();

	ToveTriangles getTriangles() const;

	inline Vertices vertices(int from, int n) {
		if (from + n > mVertexCount) {
			reserve(from + n);
		}
		return Vertices(mVertices, mStride, from);
	}

	void cache(bool keyframe);
	void clear();
	void clearTriangles();

	virtual void setLineColor(
		const PathRef &path, int vertexIndex, int vertexCount);
	virtual void setFillColor(
		const PathRef &path, int vertexIndex, int vertexCount);

	inline int getVertexCount() const {
		return mVertexCount;
	}

	inline void copyVertexData(void *buffer, size_t bufferByteSize) {
		const size_t size = mStride * mVertexCount;
		assert(bufferByteSize == size);
		std::memcpy(buffer, mVertices, size);
	}

	Submesh *submesh(const PathRef &path, int line);
};

class Submesh {
private:
	TriangleCache mTriangles;
	AbstractMesh * const mMesh;

public:
	inline Submesh(AbstractMesh *mesh) : mMesh(mesh) {
	}

	ToveTriangles getTriangles() const;

	void cache(bool keyframe);
	void clearTriangles();

	inline Vertices vertices(int from, int n) {
		return mMesh->vertices(from, n);
	}

	// used by adaptive flattener.
	void addClipperPaths(
		const ClipperPaths &paths,
		float scale,
		ToveHoles holes);

	// used by fixed flattener.
	void triangulateFixedFill(
		const int vertexIndex0,
		const PathRef &path,
		const RigidFlattener &flattener,
		ToveHoles holes);
	void triangulateFixedLine(
		int v0,
		bool miter,
		const PathRef &path,
		const RigidFlattener &flattener);

	inline bool checkTriangles(bool &trianglesChanged) {
		return mTriangles.check(
			vertices(0, mMesh->getVertexCount()),
			trianglesChanged);
	}
};

class Mesh : public AbstractMesh {
public:
	Mesh();
};

class ColorMesh : public AbstractMesh {
protected:
	void setColor(int vertexIndex, int vertexCount,
		const MeshPaint &paint);

public:
	ColorMesh();

	virtual void setLineColor(
		const PathRef &path, int vertexIndex, int vertexCount);
	virtual void setFillColor(
		const PathRef &path, int vertexIndex, int vertexCount);
};

class PaintMesh : public AbstractMesh {
protected:
	void setPaintIndex(
		int paintIndex, int vertexIndex, int vertexCount);

public:
	PaintMesh();

	virtual void setLineColor(
		const PathRef &path, int vertexIndex, int vertexCount);
	virtual void setFillColor(
		const PathRef &path, int vertexIndex, int vertexCount);
};

END_TOVE_NAMESPACE

#endif // __TOVE_MESH
