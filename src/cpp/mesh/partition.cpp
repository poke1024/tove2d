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

#include "partition.h"
#include "area.h"

BEGIN_TOVE_NAMESPACE

Partition::Partition(const std::list<TPPLPoly> &convex) {
    parts.reserve(convex.size());

    int maxN = 0;
    for (auto i = convex.begin(); i != convex.end(); i++) {
        const TPPLPoly &poly = *i;
        const int n = poly.GetNumPoints();
        maxN = std::max(maxN, n);

        Part part;
        part.outline.resize(n);
        int m = 0;
        for (int j = 0; j < n; j++) {
            const int v = poly[j].id;
            if (v >= 0) {
                part.outline[m++] = v;
            }
        }
        part.outline.resize(m);
        part.fail = 0;

        parts.push_back(part);
    }

    tempPts.resize(maxN + 2);
}

bool Partition::check(const Vertices &vertices) {
    const int nparts = parts.size();

    if (nparts == 0) {
        return false;
    }

    for (int j = 0; j < nparts; j++) {
        Part &part = parts[j];

        const Indices &outline = part.outline;
        const int n = outline.size();

        int i = part.fail;

        struct Points {
            const Vertices &vertices;
            const Indices &outline;

        public:
            inline Points(
                const Vertices &vertices,
                const Indices &outline) :
                
                vertices(vertices), 
                outline(outline) {
            }

            inline const vec2 &operator[](const int i) const {
                return vertices[outline[i]];
            }
        };

        assert(tempPts.size() >= n + 2);
        for (int k = 0; k < n; k++) {
            tempPts[k] = vertices[outline[k]];
        }
        tempPts[n] = tempPts[0];
        tempPts[n + 1] = tempPts[1];

        IsConvex isConvex;
        computeFromAreas(tempPts.data(), n, isConvex);

        if (!isConvex) {
            part.fail = i;

            if (j != 0) {
                std::swap(parts[j], parts[0]);
            }
            return false;
        }
    }

    return true;
}

END_TOVE_NAMESPACE
