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

#ifndef __TOVE_SHADER_LINK
#define __TOVE_SHADER_LINK 1

#include "paint_feed.h"

BEGIN_TOVE_NAMESPACE

class AbstractFeed {
public:
	virtual ~AbstractFeed() {
	}

	virtual ToveChangeFlags beginUpdate() = 0;
	virtual ToveChangeFlags endUpdate() = 0;

    virtual ToveShaderData *getData() {
    	return nullptr;
    }

	virtual TovePaintColorAllocation getColorAllocation() const {
		return TovePaintColorAllocation{0, 0};
	}

	virtual void bind(const ToveGradientData &data) {
	}
};

class ColorFeed : public AbstractFeed {
private:
	const float scale;
	std::vector<PaintFeed> feeds;

public:
	ColorFeed(const GraphicsRef &graphics, float scale) :
		scale(scale) {

		const int n = graphics->getNumPaths();
		for (int i = 0; i < n; i++) {
			const PathRef &path = graphics->getPath(i);
			feeds.emplace_back(path, scale, CHANGED_LINE_STYLE);
			feeds.emplace_back(path, scale, CHANGED_FILL_STYLE);
		}
	}

	TovePaintColorAllocation getColorAllocation() const {
		int numColors = 1;
		for (const auto &feed : feeds) {
			numColors = std::max(numColors, feed.getNumColors());
		}
		return TovePaintColorAllocation{
			int16_t(feeds.size()),
			int16_t(numColors)};
	}

	void bind(const ToveGradientData &data) {
		const int n = feeds.size();
		for (int i = 0; i < n; i++) {
			feeds[i].bind(data, i);
		}
	}

	virtual ToveChangeFlags beginUpdate() {
		ToveChangeFlags changes = 0;
		for (auto &feed : feeds) {
			changes |= feed.beginUpdate();
		}
		return changes;
	}

    virtual ToveChangeFlags endUpdate() {
		ToveChangeFlags changes = 0;
		for (auto &feed : feeds) {
			changes |= feed.endUpdate();
		}
		return changes;
    }
};

END_TOVE_NAMESPACE

#endif // __TOVE_SHADER_LINK
