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

#include "triangles.h"
#include <sstream>
#include <chrono>

BEGIN_TOVE_NAMESPACE

ToveVertexIndex *TriangleStore::allocate(int n, bool isFinalSize) {
    const int offset = mSize;

    const int k = (mMode == TRIANGLES_LIST) ? 3 : 1;
    mSize += n * k;

    const int count = isFinalSize ? mSize : nextpow2(mSize);
    mTriangles = static_cast<ToveVertexIndex*>(realloc(
        mTriangles, count * sizeof(ToveVertexIndex)));

    if (!mTriangles) {
        TOVE_BAD_ALLOC();
        return nullptr;
    }

    return &mTriangles[offset];
}

void TriangleStore::_add(
    const std::list<TPPLPoly> &triangles,
    bool isFinalSize) {

    ToveVertexIndex *indices = allocate(triangles.size(), isFinalSize);
    for (auto i = triangles.begin(); i != triangles.end(); i++) {
        const TPPLPoly &poly = *i;
        for (int j = 0; j < 3; j++) {
            *indices++ = ToLoveVertexMapIndex(poly[j].id);
        }
    }
}

void TriangleStore::_add(
    const std::vector<ToveVertexIndex> &triangles,
    const ToveVertexIndex i0,
    bool isFinalSize) {

    const int n = triangles.size();
    assert(n % 3 == 0);
    ToveVertexIndex *indices = allocate(n / 3, isFinalSize);
    const ToveVertexIndex *input = triangles.data();
    for (int i = 0; i < n; i++) {
        *indices++ = i0 + ToLoveVertexMapIndex(*input++);
    }
}


TriangleCache::~TriangleCache() {
    for (int i = 0; i < triangulations.size(); i++) {
        delete triangulations[i];
    }
}

void TriangleCache::evict() {
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
        delete triangulations[minIndex];
        triangulations.erase(triangulations.begin() + minIndex);
        current = std::min(current, (int)(triangulations.size() - 1));
    }
}

bool TriangleCache::findCachedTriangulation(
    const Vertices &vertices,
    bool &trianglesChanged) {
    
    const int n = triangulations.size();
    if (n == 0) {
        return false;
    }

    const bool debug = tove::report::config.level <= TOVE_REPORT_DEBUG;

    std::chrono::high_resolution_clock::time_point t0;
    if (debug) {
        t0 = std::chrono::high_resolution_clock::now();
    }

    assert(current < n);
    bool good = false;

    if (triangulations[current]->check(vertices)) {
        triangulations[current]->useCount++;
        trianglesChanged = false;
        good = true;
    } else {
        const int k = std::max(current, n - current);
        for (int i = 1; i <= k; i++) {
            if (current + i < n) {
                const int forward = (current + i) % n;
                if (triangulations[forward]->check(vertices)) {
                    std::swap(
                        triangulations[(current + 1) % n],
                        triangulations[forward]);
                    current = (current + 1) % n;
                    triangulations[current]->useCount++;
                    trianglesChanged = true;
                    good = true;
                    break;
                }
            }

            if (current - i >= 0) {
                const int backward = (current + n - i) % n;
                if (triangulations[backward]->check(vertices)) {
                    std::swap(
                        triangulations[(current + n - 1) % n],
                        triangulations[backward]);
                    current = (current + n - 1) % n;
                    triangulations[current]->useCount++;
                    trianglesChanged = true;
                    good = true;
                    break;
                }
            }
        }
    }

    if (debug) {
        std::ostringstream s;
        if (good) {
            const int duration = std::chrono::duration_cast<std::chrono::nanoseconds>(
                std::chrono::high_resolution_clock::now() - t0).count();
            s << "found cached triangulation [" << current << "] in " <<
                duration / 1000.0f << " μs for " << *name;
        } else {
            s << "no cached triangulation for " << *name;
        }
        tove::report::report(s.str().c_str(), TOVE_REPORT_DEBUG);            
    }

    return good;
}

END_TOVE_NAMESPACE
