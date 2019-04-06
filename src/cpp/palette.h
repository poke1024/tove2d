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

struct RGBA {
	uint8_t r;
	uint8_t g;
	uint8_t b;
	uint8_t a;
};

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
    const int _size;
#if TOVE_NANOFLANN
    KDColors kdcolors;
    ColorIndex index;
#endif 

public:
    static Palette *deref(void *palette) {
        // binding code for NanoSVG tove extensions.
        return static_cast<PaletteRef*>(palette)->get();
    }

    inline int size() const {
        return _size;
    }

    Palette(const uint8_t *c, const int n) :
        colors(n * 3),
        _size(n)
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

    inline RGBA closest(const uint8_t r, const uint8_t g, const uint8_t b, const uint8_t a) const {
        const int n = _size;
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

            int32_t best_d = std::numeric_limits<int32_t>::max();
            best_p = p;

            // brute force search. don't underestimate the speed of this. it's
            // about 25% faster than nanoflann for everything <= 64 colors. at
            // 128 colors, runtimes are rougly the same.

            for (int i = 0; i < n; i++, p += 3) {
                const int16_t dr = r - int16_t(p[0]);
                const int16_t dg = g - int16_t(p[1]);
                const int16_t db = b - int16_t(p[2]);

                const int32_t d =
                    int32_t(dr * dr) +
                    int32_t(dg * dg) +
                    int32_t(db * db);
                if (d < best_d) {
                    best_d = d;
                    best_p = p;
                }
            }
#if TOVE_NANOFLANN
        }
#endif

        return RGBA{best_p[0], best_p[1], best_p[2], a};
    }
};

END_TOVE_NAMESPACE

#endif // __TOVE_PALETTE
