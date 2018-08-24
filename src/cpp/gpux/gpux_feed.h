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

#ifndef __TOVE_GPUX_FEED
#define __TOVE_GPUX_FEED 1

#include "../shader/feed/color_feed.h"
#include "geometry_feed.h"

BEGIN_TOVE_NAMESPACE

class GPUXFeed : public AbstractFeed {
private:
	ToveShaderData data;
	LinePaintFeed lineColor;
	FillPaintFeed fillColor;
	GeometryFeed geometry;

public:
	inline GPUXFeed(const PathRef &path, bool enableFragmentShaderStrokes) :
		lineColor(path, data.color.line, 1),
		fillColor(path, data.color.fill, 1),
		geometry(path, data.geometry, data.color.line, enableFragmentShaderStrokes) {
	}

	virtual ToveChangeFlags beginUpdate() {
		return lineColor.beginUpdate() |
			fillColor.beginUpdate() |
			geometry.beginUpdate();
	}

    virtual ToveChangeFlags endUpdate() {
		return lineColor.endUpdate() |
			fillColor.endUpdate() |
			geometry.endUpdate();
    }

    virtual ToveShaderData *getData() {
    	return &data;
    }
};

END_TOVE_NAMESPACE

#endif // __TOVE_GPUX_FEED
