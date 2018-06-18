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
#include "flatten.h"
#include "../interface.h"
#include "../utils.h"
#include "../../thirdparty/polypartition.h"

class TriangleStore {
private:
	const ToveTrianglesMode mode;
	int size;
	uint16_t *triangles;

public:
	uint16_t *allocate(int n, bool exact = false) {
		const int offset = size;

		const int k = (mode == TRIANGLES_LIST) ? 3 : 1;
	    size += n * k;

		const int count = exact ? size : nextpow2(size);
	    triangles = static_cast<uint16_t*>(realloc(
	    	triangles, count * sizeof(uint16_t)));

	    if (!triangles) {
			TOVE_BAD_ALLOC();
			return nullptr;
	    }

		return &triangles[offset];
	}

private:
	void _add(const std::list<TPPLPoly> &triangles, bool exact) {
	    uint16_t *indices = allocate(triangles.size(), exact);
	    for (auto i = triangles.begin(); i != triangles.end(); i++) {
			const TPPLPoly &poly = *i;
	    	for (int j = 0; j < 3; j++) {
				// use 1-based indices for Tove's Mesh:setVertexMap()
	    		*indices++ = poly[j].id + 1;
	    	}
	    }
	}

public:
	TriangleStore(ToveTrianglesMode mode) :
		triangles(nullptr), mode(mode), size(0) {
	}

	~TriangleStore() {
		if (triangles) {
			free(triangles);
		}
	}

	TriangleStore(const std::list<TPPLPoly> &triangles) :
		triangles(nullptr), mode(TRIANGLES_LIST), size(0) {

		_add(triangles, true);
	}

	void add(const std::list<TPPLPoly> &triangles) {
		assert(mode == TRIANGLES_LIST);
		_add(triangles, false);
	}

	void clear() {
		size = 0;
	}

	ToveTriangles get() const {
		ToveTriangles array;
		array.array = triangles;
		array.mode = mode;
		array.size = size;
		return array;
	}
};

struct Triangulation {
	Triangulation(ToveTrianglesMode mode) : triangles(mode) {
	}

	Triangulation(const std::list<TPPLPoly> &convex) :
		partition(convex),
		useCount(0),
		keyframe(false),
		triangles(TRIANGLES_LIST) {
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

	void evict() {
		uint64_t minCount = std::numeric_limits<uint64_t>::max();
		int minIndex = -1;

		for (int i = 0; i < triangulations.size(); i++) {
			const Triangulation *t = triangulations[i];
			if (!t->keyframe) {
				const uint64_t count = t->useCount;
				if (count < minCount) {
					minCount = count;
					minIndex = i;
				}
			}
		}

		if (minIndex >= 0) {
			triangulations.erase(triangulations.begin() + minIndex);
			current = std::min(current, (int)(triangulations.size() - 1));
		}
	}

public:
	TriangleCache(int cacheSize = 8) : current(0), cacheSize(cacheSize) {
	}

	~TriangleCache() {
		for (int i = 0; i < triangulations.size(); i++) {
			delete triangulations[i];
		}
	}

	void add(Triangulation *t) {
		if (t->partition.empty()) {
			return;
		}

		if (triangulations.size() + 1 > cacheSize) {
			evict();
		}

		triangulations.insert(triangulations.begin() + current, t);
	}

	void cache(bool keyframe) {
		if (triangulations.size() == 0) {
			return;
		}
		assert(current < triangulations.size());
		triangulations[current]->keyframe = keyframe;
	}

	uint16_t *allocate(ToveTrianglesMode mode, int n) {
		if (triangulations.empty()) {
			triangulations.push_back(new Triangulation(mode));
		}
		assert(current < triangulations.size());
		return triangulations[current]->triangles.allocate(n);
	}

	void add(const std::list<TPPLPoly> &triangles) {
		if (triangulations.empty()) {
			triangulations.push_back(new Triangulation(TRIANGLES_LIST));
		}
		assert(current < triangulations.size());
		triangulations[current]->triangles.add(triangles);
	}

	void clear() {
		if (current < triangulations.size()) {
			triangulations[current]->triangles.clear();
		}
	}

	ToveTriangles get() const {
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

	bool check(const float *vertices, bool &triangleCacheFlag) {
		const int n = triangulations.size();
		if (n == 0) {
			return false;
		}

		assert(current < n);
		if (triangulations[current]->partition.check(vertices)) {
			triangulations[current]->useCount++;
			return true;
		}

		const int k = std::max(current, n - current);
		for (int i = 1; i <= k; i++) {
			if (current + i < n) {
				int forward = (current + i) % n;
				if (triangulations[forward]->partition.check(vertices)) {
					std::swap(triangulations[(current + 1) % n], triangulations[forward]);
					current = (current + 1) % n;
					triangulations[current]->useCount++;
					triangleCacheFlag = true;
					return true;
				}
			}

			if (current - i >= 0) {
				int backward = (current + n - i) % n;
				if (triangulations[backward]->partition.check(vertices)) {
					std::swap(triangulations[(current + n - 1) % n], triangulations[backward]);
					current = (current + n - 1) % n;
					triangulations[current]->useCount++;
					triangleCacheFlag = true;
					return true;
				}
			}
		}

		return false;
	}
};

#endif // __TOVE_MESH_TRIANGLES
