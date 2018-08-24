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
#include "../../thirdparty/polypartition.h"

BEGIN_TOVE_NAMESPACE

class TriangleStore {
private:
	int size;
	ToveVertexIndex *triangles;

public:
	const ToveTrianglesMode mode;

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
		triangles(nullptr), mode(mode), size(0) {
	}

	inline ~TriangleStore() {
		if (triangles) {
			free(triangles);
		}
	}

	inline TriangleStore(const std::list<TPPLPoly> &triangles) :
		triangles(nullptr), mode(TRIANGLES_LIST), size(0) {

		_add(triangles, true);
	}

	inline void add(const std::list<TPPLPoly> &triangles) {
		assert(mode == TRIANGLES_LIST);
		_add(triangles, false);
	}

	inline void add(const std::vector<ToveVertexIndex> &triangles, ToveVertexIndex i0) {
		assert(mode == TRIANGLES_LIST);
		_add(triangles, i0, false);
	}

	inline void clear() {
		size = 0;
	}

	inline ToveTriangles get() const {
		ToveTriangles array;
		array.array = triangles;
		array.mode = mode;
		array.size = size;
		return array;
	}
};

struct Triangulation {
	inline Triangulation(ToveTrianglesMode mode) : triangles(mode) {
	}

	inline Triangulation(const std::list<TPPLPoly> &convex) :
		partition(convex),
		useCount(0),
		keyframe(false),
		triangles(TRIANGLES_LIST) {
	}

	inline ToveTrianglesMode getMode() const {
		return triangles.mode;
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

	inline ToveTriangles get() const {
		if (current < triangulations.size()) {
			return triangulations[current]->triangles.get();
		} else {
			ToveTriangles array;
			array.array = nullptr;
			array.mode = TRIANGLES_LIST;
			array.size = 0;
			return array;
		}
	}

	bool check(const Vertices &vertices, bool &trianglesChanged);
};

END_TOVE_NAMESPACE

#endif // __TOVE_MESH_TRIANGLES
