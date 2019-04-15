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
#include "area.h"
#include "../paint.h"
#include "../subpath.h"
#include <map>
#include "../../thirdparty/robin-map/include/tsl/robin_map.h"

BEGIN_TOVE_NAMESPACE

struct hash_int_point {
	// see http://szudzik.com/ElegantPairing.pdf
	inline size_t operator()(const ClipperLib::IntPoint &p) const {
		const auto x = p.X;
		const auto y = p.Y;
		return x >= y ? x * x + x + y : y * y + x;
	}
};

struct equal_int_point {
	inline bool operator()(const ClipperLib::IntPoint &a, const ClipperLib::IntPoint &b) const {
		return a.X == b.X && a.Y == b.Y;
	}
};
typedef tsl::robin_map<
	ClipperLib::IntPoint,
	ToveVertexIndex,
	hash_int_point,
	equal_int_point,
	std::allocator<std::pair<ClipperLib::IntPoint, ToveVertexIndex*>>,
	true /* store hash */> IntVertexMap;

class RigidFlattener;

typedef uint32_t SubmeshId;

class Submesh;

class AbstractMesh : public Referencable {
protected:
	void *mVertices;
	int32_t mVertexCount;
	bool mOwnsBuffer;

	const NameRef mName;
	const uint16_t mStride;

	std::map<SubmeshId, Submesh*> mSubmeshes;
	mutable std::vector<ToveVertexIndex> mCoalescedTriangles;

	void reserve(int32_t n);

	void setNewExternalVertexBuffer(
		void *buffer, size_t bufferByteSize);

public:
	AbstractMesh(const NameRef &name, uint16_t stride);
	virtual ~AbstractMesh();

	ToveTrianglesMode getIndexMode() const;

	int32_t getIndexCount() const;

	void copyIndexData(
		ToveVertexIndex *indices,
		int32_t indexCount) const;

	inline void clip(int n) {
		mVertexCount = std::min(mVertexCount, n);
	}

	inline Vertices vertices(int from, int n) {
		if (from + n > mVertexCount) {
			reserve(from + n);
		}
		return Vertices(mVertices, mStride, from);
	}

	void cacheKeyFrame();
	void setCacheSize(int size);
	void clear(bool ensureOwnBuffer = false);
	void clearTriangles();

	virtual void setLineColor(
		const PathRef &path,
		const PathPaintInd &paint,
		const int vertexIndex,
		const int vertexCount);
	virtual void setFillColor(
		const PathRef &path,
		const PathPaintInd &paint,
		const int vertexIndex,
		const int vertexCount);

	inline int getVertexCount() const {
		return mVertexCount;
	}

	inline void setExternalVertexBuffer(void *buffer, size_t bufferByteSize) {
		if (buffer != mVertices) {
			setNewExternalVertexBuffer(buffer, bufferByteSize);
		}
	}

	Submesh *submesh(int pathIndex, int line);

	inline const NameRef &getName() const {
		return mName;
	}
};

class Submesh {
private:
	AbstractMesh * const mMesh;
	TriangleCache mTriangles;
	SubpathCleaner mCleaner;

public:
	inline Submesh(AbstractMesh *mesh) :
		mMesh(mesh),
		mTriangles(mesh->getName()) {
	}

	inline ToveTrianglesMode getIndexMode() const {
		return mTriangles.getIndexMode();
	}

	inline int32_t getIndexCount() const {
		return mTriangles.getIndexCount();
	}

	inline void copyIndexData(
		ToveVertexIndex *indices,
		int32_t indexCount) const {

		mTriangles.copyIndexData(indices, indexCount);
	}

	void cacheKeyFrame();
	void setCacheSize(int size);
	void clearTriangles();

	inline Vertices vertices(int from, int n) {
		return mMesh->vertices(from, n);
	}

	// used by adaptive flattener.
	void addClipperPaths(
		const ClipperPaths &paths,
		float scale);

	// used by fixed flattener.
	void triangulateFixedResolutionFill(
		const int vertexIndex0,
		const PathRef &path,
		const RigidFlattener &flattener);
	void triangulateFixedResolutionLine(
		const int pathVertex,
		const bool miter,
		const bool reduceOverlap,
		const int verticesPerSegment,
		const PathRef &path,
		const RigidFlattener &flattener);

	inline bool findCachedTriangulation(
		bool &trianglesChanged) {
		
		return mTriangles.findCachedTriangulation(
			vertices(0, mMesh->getVertexCount()),
			trianglesChanged);
	}

	inline const NameRef &getName() const {
		return mMesh->getName();
	}
};

class Mesh : public AbstractMesh {
public:
	Mesh(const NameRef &name);
};

class ColorMesh : public AbstractMesh {
protected:
	void setColor(int vertexIndex, int vertexCount,
		const MeshPaint &paint);

public:
	ColorMesh(const NameRef &name);

	virtual void setLineColor(
		const PathRef &path,
		const PathPaintInd &paint,
		const int vertexIndex,
		const int vertexCount);
	virtual void setFillColor(
		const PathRef &path,
		const PathPaintInd &paint,
		const int vertexIndex,
		const int vertexCount);
};

class PaintMesh : public AbstractMesh {
protected:
	void setPaintIndex(
		const PaintIndex &paintIndex,
		const int vertexIndex,
		const int vertexCount);

public:
	PaintMesh(const NameRef &name);

	virtual void setLineColor(
		const PathRef &path,
		const PathPaintInd &paint,
		const int vertexIndex,
		const int vertexCount);
	virtual void setFillColor(
		const PathRef &path,
		const PathPaintInd &paint,
		const int vertexIndex,
		const int vertexCount);
};

END_TOVE_NAMESPACE

#endif // __TOVE_MESH
