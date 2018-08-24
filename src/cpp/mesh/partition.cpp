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

BEGIN_TOVE_NAMESPACE

Partition::Partition(const std::list<TPPLPoly> &convex) {
    parts.reserve(convex.size());

    for (auto i = convex.begin(); i != convex.end(); i++) {
        const TPPLPoly &poly = *i;
        const int n = poly.GetNumPoints();

        Part part;
        part.outline.resize(n);
        for (int j = 0; j < n; j++) {
            part.outline[j] = poly[j].id;
        }
        part.fail = 0;

        parts.push_back(part);
    }
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
        int k = 0;

        do {
            assert(i >= 0 && i < n);
            const auto &p0 = vertices[outline[i]];

            const int i1 = find_unequal_forward(
                vertices, outline.data(), i, n);
            const auto &p1 = vertices[outline[i1]];

            const int i2 = find_unequal_forward(
                vertices, outline.data(), i1, n);
            const auto &p2 = vertices[outline[i2]];

            float area = cross(p0.x, p0.y, p1.x, p1.y, p2.x, p2.y);

            const float eps = 0.1;
            if (area > eps) {
                part.fail = i;

                if (j != 0) {
                    std::swap(parts[j], parts[0]);
                }
                return false;
            }

            if (i1 > i) {
                k += i1 - i;
            } else {
                k += (n - i) + i1;
            }
            i = i1;

        } while (k < n);
    }

    return true;
}

END_TOVE_NAMESPACE
