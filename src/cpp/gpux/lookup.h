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

#ifndef __TOVE_SHADER_LOOKUP
#define __TOVE_SHADER_LOOKUP 1

#include "../../thirdparty/robin-map/include/tsl/robin_set.h"
#include <vector>

#include "curve_data.h"

BEGIN_TOVE_NAMESPACE

enum EventType {
    EVENT_ENTER,
    EVENT_EXIT,
	EVENT_MARK
};

struct Event {
    float y;
    EventType t : 8;
    uint8_t curve;
};

enum {
    SENTINEL_END = 0xff,
    SENTINEL_STROKES = 0xfe
};

class LookupTable {
public:
	typedef tsl::robin_set<uint8_t> CurveSet;

private:
	ToveShaderGeometryData &_data;
    CurveSet active;
    const int _maxCurves;
	const int _ignore;

public:
    LookupTable(int maxCurves, ToveShaderGeometryData &data, int ignore) :
		active(4),
        _maxCurves(maxCurves),
        _data(data),
        _ignore(ignore) {
    }

#if TOVE_DEBUG
	void dump(ToveShaderGeometryData *data, int dim) {
		printf("-- dump lut %d --\n", dim);
		printf("maxCurves: %d\n", _maxCurves);
		for (int i = 0; i < _maxCurves; i++) {
			printf("lut %03d: %f\n", i, data->lookupTable[dim][i]);
		}

		for (int y = 0; y < data->listsTextureSize[1]; y++) {
			if (y == data->listsTextureSize[1] / 2) {
				printf("---------\n");
			}
			printf("lists %03d: ", y);
			uint8_t *yptr = data->listsTexture + y * data->listsTextureRowBytes;
			for (int x = 0; x < data->listsTextureSize[0]; x++) {
				printf("%d ", yptr[x]);
			}
			printf("\n");
		}
		printf("-- end dump lut --\n");
	}
#endif

    template<typename F>
    void build(int dim, const std::vector<Event> &events,
		int numEvents, const std::vector<ExCurveData> &extended,
		float padding, const F &finish) {

        ToveShaderGeometryData &data = _data;

        if (numEvents < 1) {
            data.lookupTableMeta->n[dim] = 0;
            return;
        }

		const int ignore = _ignore;

        active.clear();
        float *lookupTable = data.lookupTable[dim];

        auto i = events.cbegin();
        const auto end = events.cbegin() + numEvents;

        float *ylookup = lookupTable;

		uint8_t *yptr = data.listsTexture;
		int rowBytes = data.listsTextureRowBytes;

		yptr += dim * rowBytes * (data.listsTextureSize[1] / 2);

        int z = 0;

        if (data.fragmentShaderLine && padding > 0.0) {
            const float y0 = i->y;
            *ylookup++ = y0 - padding;
            finish(y0 - padding, y0, active, yptr, z++);
            yptr += rowBytes;
        }

        while (i != end) {
            auto j = i;
            float y0 = i->y;
            while (j != end && j->y == y0) {
                switch (j->t) {
					case EVENT_ENTER:
                    	active.insert(j->curve);
						break;
					case EVENT_EXIT:
                    	active.erase(j->curve);
						break;
					case EVENT_MARK: // roots
						break;
                }
                j++;
            }

            *ylookup++ = y0;

            int k = 0;
            assert(active.size() <= _maxCurves);
            for (const auto curve : active) {
				if ((extended[curve].ignore & ignore) == 0) {
					yptr[k++] = curve;
				}
            }

            float y1;
            if (j != end) {
                y1 = j->y;
            } else {
                y1 = y0;

                if (data.fragmentShaderLine && padding > 0.0) {
                    y1 += padding;
                }
            }

            finish(y0, y1, active, &yptr[k], z++);

            i = j;
			yptr += rowBytes;
        }

        *yptr = SENTINEL_END;

        data.lookupTableMeta->n[dim] = ylookup - lookupTable;
		assert(data.lookupTableMeta->n[dim] <= data.lookupTableSize);
    }

    inline void build(int dim, const std::vector<Event> &events,
		int numEvents, const std::vector<ExCurveData> &extended) {
        build(dim, events, numEvents, extended, 0.0,
			[] (float y0, float y1, const CurveSet &active, uint8_t *list, int z) {
            	*list = SENTINEL_END;
        	});
    }
};

END_TOVE_NAMESPACE

#endif // __TOVE_SHADER_LOOKUP
