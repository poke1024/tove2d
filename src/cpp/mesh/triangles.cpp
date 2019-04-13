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

    assert(mMode == TRIANGLES_LIST);

    ToveVertexIndex *indices = allocate(
        triangles.size(), isFinalSize);

    int numBadTriangles = 0;

    for (const auto &t : triangles) {
        bool good = true;
        for (int i = 0; i < 3; i++) {
            const int index = t[i].id;
            if (index < 0) {
                good = false;
                break;
            }
            indices[i] = ToLoveVertexMapIndex(index);
        }
        if (good) {
            indices += 3;
        } else {
            numBadTriangles += 1;
        }
    }

    mSize -= numBadTriangles * 3;
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

void TriangleCache::add(Triangulation *t) {
    if (t->partition.empty()) {
        return;
    }

    if (triangulations.size() + 1 > cacheSize) {
        evict();
    }

    t->useCount = 0;
    triangulations.push_back(t);
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
        trianglesChanged = false;
        good = true;
    } else {
        for (int i = 0; i < n; i++) {
            if (i != current && triangulations[i]->check(vertices)) {
                current = i;
                trianglesChanged = true;
                good = true;
                break;
            }
        }
    }

    const int switchedTo = current;

    if (trianglesChanged && good) {
        triangulations[current]->useCount++;
        std::swap(triangulations[0], triangulations[current]);
        current = 0;
    }

    if (debug) {
        std::ostringstream s;
        if (good) {
            const int duration = std::chrono::duration_cast<std::chrono::nanoseconds>(
                std::chrono::high_resolution_clock::now() - t0).count();
            if (switchedTo == 0) {
                s << "[" << *name << "]" << " verified triangulation in " <<
                    duration / 1000.0f << " μs";
            } else {
                s << "[" << *name << "]" << " switched to cached triangulation [" << switchedTo << "] in " <<
                    duration / 1000.0f << " μs";
            }
        } else {
            s << "[" << *name << "] no cached triangulation";
        }
        tove::report::report(s.str().c_str(), TOVE_REPORT_DEBUG);            
    }

    return good;
}

END_TOVE_NAMESPACE
