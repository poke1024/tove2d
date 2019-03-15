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

#ifndef __TOVE_MESH_TRIANGLES
#define __TOVE_MESH_TRIANGLES 1

#include "partition.h"
#include "../interface.h"
#include "../utils.h"

BEGIN_TOVE_NAMESPACE

#if TOVE_TARGET == TOVE_TARGET_LOVE2D
inline ToveVertexIndex ToLoveVertexMapIndex(ToveVertexIndex i) {
	// convert to indices for LÖVE's Mesh:setVertexMap(). this
	// used to be (1 + i), but since we use the ByteData based
	// version, it's now 0-based as well.
	return i;
}
#else
inline ToveVertexIndex ToLoveVertexMapIndex(ToveVertexIndex i) {
	return i;
}
#endif

class TriangleStore {
private:
	int32_t mSize;
	ToveVertexIndex *mTriangles;

public:
	const ToveTrianglesMode mMode;

	ToveVertexIndex *allocate(int n, bool isFinalSize = false);

private:
	void _add(
		const std::list<TPPLPoly> &triangles,
		bool isFinalSize);

	void _add(
		const std::vector<ToveVertexIndex> &triangles,
		const ToveVertexIndex i0,
		bool isFinalSize);

public:
	inline TriangleStore(ToveTrianglesMode mode) :
		mSize(0), mTriangles(nullptr), mMode(mode) {
	}

	inline ~TriangleStore() {
		if (mTriangles) {
			free(mTriangles);
		}
	}

	inline TriangleStore(const std::list<TPPLPoly> &triangles) :
		mSize(0), mTriangles(nullptr), mMode(TRIANGLES_LIST) {

		_add(triangles, true);
	}

	inline void add(const std::list<TPPLPoly> &triangles) {
		assert(mMode == TRIANGLES_LIST);
		_add(triangles, false);
	}

	inline void add(const std::vector<ToveVertexIndex> &triangles, ToveVertexIndex i0) {
		assert(mMode == TRIANGLES_LIST);
		_add(triangles, i0, false);
	}

	inline void clear() {
		mSize = 0;
	}

	inline size_t size() const {
		return mSize;
	}

	inline ToveTrianglesMode mode() const {
		return mMode;
	}

	inline void copy(
		ToveVertexIndex *indices,
		int32_t indexCount) const {

		const int32_t n = std::min(mSize, indexCount);
		if (n > 0) {
			std::memcpy(indices, mTriangles,
				n * sizeof(ToveVertexIndex));
		}
	}
};

struct Triangulation {
	inline Triangulation(ToveTrianglesMode mode) : triangles(mode) {
	}

	inline Triangulation(const std::list<TPPLPoly> &convex) :
		partition(convex),
		triangles(TRIANGLES_LIST),
		useCount(0),
		keyframe(false) {
	}

	inline ToveTrianglesMode getMode() const {
		return triangles.mode();
	}

	Partition partition;
	TriangleStore triangles;
	uint64_t useCount;
	bool keyframe;
};

class TriangleCache {
private:
	std::vector<Triangulation*> triangulations;
	int current;
	int cacheSize;

	void evict();

	inline Triangulation *currentTriangulation() const {
		assert(current < triangulations.size());
		return triangulations[current];
	}

public:
	inline TriangleCache(int cacheSize = 2) :
		current(0), cacheSize(cacheSize) {
		assert(cacheSize >= 2);
	}

	~TriangleCache();

	inline void add(Triangulation *t) {
		if (t->partition.empty()) {
			return;
		}

		if (triangulations.size() + 1 > cacheSize) {
			evict();
		}

		triangulations.insert(triangulations.begin() + current, t);
	}

	inline void cache(bool keyframe) {
		if (triangulations.size() == 0) {
			return;
		}
		currentTriangulation()->keyframe = keyframe;
	}

	inline bool hasMode(ToveTrianglesMode mode) {
		if (triangulations.empty()) {
			return false;
		} else {
			return triangulations[current]->getMode() == mode;
		}
	}

	inline ToveVertexIndex *allocate(ToveTrianglesMode mode, int n) {
		if (triangulations.empty()) {
			triangulations.push_back(new Triangulation(mode));
		} else {
			assert(triangulations[current]->getMode() == mode);
		}
		return currentTriangulation()->triangles.allocate(n);
	}

	inline void add(const std::list<TPPLPoly> &triangles) {
		if (triangulations.empty()) {
			triangulations.push_back(new Triangulation(TRIANGLES_LIST));
		}
		currentTriangulation()->triangles.add(triangles);
	}

	inline void add(const std::vector<ToveVertexIndex> &triangles,
		ToveVertexIndex i0) {
		if (triangulations.empty()) {
			triangulations.push_back(new Triangulation(TRIANGLES_LIST));
		}
		currentTriangulation()->triangles.add(triangles, i0);
	}

	inline void clear() {
		if (current < triangulations.size()) {
			triangulations[current]->triangles.clear();
		}
	}

	inline ToveTrianglesMode getIndexMode() const {
		if (current < triangulations.size()) {
			return triangulations[current]->triangles.mode();
		} else {
			return TRIANGLES_LIST;
		}
	}

	inline int32_t getIndexCount() const {
		if (current < triangulations.size()) {
			return triangulations[current]->triangles.size();
		} else {
			return 0;
		}
	}

	inline void copyIndexData(
		ToveVertexIndex *indices,
		int32_t indexCount) const {

		if (current < triangulations.size()) {
			auto &t = triangulations[current]->triangles;
			t.copy(indices, indexCount);
		}
	}

	bool findCachedTriangulation(
		const Vertices &vertices, bool &trianglesChanged);
};

END_TOVE_NAMESPACE

#endif // __TOVE_MESH_TRIANGLES
