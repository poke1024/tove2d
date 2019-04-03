/*
 * TÖVE - Animated vector graphics for LÖVE.
 * https://github.com/poke1024/tove2d
 *
 * Copyright (c) 2019, Bernhard Liebl
 *
 * Distributed under the MIT license. See LICENSE file for details.
 *
 * All rights reserved.
 */

#ifndef __TOVE_PALETTE
#define __TOVE_PALETTE 1

#include "common.h"
#if TOVE_NANOFLANN
#include "../thirdparty/nanoflann/include/nanoflann.hpp"
#endif

BEGIN_TOVE_NAMESPACE

class Palette {
private:
#if TOVE_NANOFLANN
    struct KDColors {
        const std::vector<uint8_t> &colors;

        inline KDColors(const std::vector<uint8_t> &colors) : colors(colors) {
        }

        inline size_t kdtree_get_point_count() const {
            return colors.size() / 3;
        }

        inline int kdtree_get_pt(const size_t idx, const size_t dim) const {
            return colors[idx * 3 + dim];
        }

        template <class BBOX>
        bool kdtree_get_bbox(BBOX& /* bb */) const {
            return false;
        }
    };

    typedef nanoflann::KDTreeSingleIndexAdaptor<
        nanoflann::L2_Simple_Adaptor<int, KDColors>,
        KDColors,
        3> ColorIndex;
#endif

    std::vector<uint8_t> colors;
    const int size;
#if TOVE_NANOFLANN
    KDColors kdcolors;
    ColorIndex index;
#endif 

public:
    static Palette *deref(void *palette) {
        // binding code for NanoSVG tove extensions.
        return static_cast<PaletteRef*>(palette)->get();
    }

    Palette(const uint8_t *c, const int n) :
        colors(n * 3),
        size(n)
#if TOVE_NANOFLANN
        , kdcolors(colors)
        , index(3, kdcolors, nanoflann::KDTreeSingleIndexAdaptorParams(24))
#endif
        {
        
        std::memcpy(colors.data(), c, n * 3);
#if TOVE_NANOFLANN
        index.buildIndex();
#endif
    }

    inline uint32_t closest(const int r, const int g, const int b) const {
        const int n = size;
        const uint8_t *best_p;
#if TOVE_NANOFLANN
        if (n >= 128) {
            const int query_pt[3] = {r, g, b};
            size_t i;
            int distance;
            index.knnSearch(&query_pt[0], 1, &i, &distance);
            best_p = colors.data() + 3 * i;
        } else {
#endif
            const uint8_t *p = colors.data();

            long best_d = std::numeric_limits<long>::max();
            best_p = p;

            // brute force search. don't underestimate the speed of this. it's
            // about 25% faster than nanoflann for everything <= 64 colors. at
            // 128 colors, runtimes are rougly the same.

            for (int i = 0; i < n; i++, p += 3) {
                const int32_t dr = r - p[0];
                const int32_t dg = g - p[1];
                const int32_t db = b - p[2];

                const long d = dr * dr + dg * dg + db * db;
                if (d < best_d) {
                    best_d = d;
                    best_p = p;
                }
            }
#if TOVE_NANOFLANN
        }
#endif

        int cr = best_p[0];
        int cg = best_p[1];
        int cb = best_p[2];
        return cr | (cg << 8) | (cb << 16);
    }
};

END_TOVE_NAMESPACE

#endif // __TOVE_PALETTE
