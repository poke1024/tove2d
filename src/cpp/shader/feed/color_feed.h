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
		return TovePaintColorAllocation{0, 0, 0};
	}

	virtual void bindPaintIndices(const ToveGradientData &data) {
	}
};

class ColorFeed : public AbstractFeed {
private:
	GraphicsRef graphics;
	const float scale;
	PaintIndicesRef paints;
	std::vector<PaintFeed> feeds;

public:
	ColorFeed(const GraphicsRef &graphics, float scale) :
		graphics(graphics),
		scale(scale),
		paints(graphics->getPaintIndices()) {

		const int n = graphics->getNumPaths();
		for (int i = 0; i < n; i++) {
			const PathRef &path = graphics->getPath(i);
			if (path->hasStroke()) {
				feeds.emplace_back(path, scale, CHANGED_LINE_STYLE, paints->get(i).line);
			}
			if (path->hasFill()) {
				feeds.emplace_back(path, scale, CHANGED_FILL_STYLE, paints->get(i).fill);
			}
		}

		assert(feeds.size() == paints->getNumPaints());
	}

	TovePaintColorAllocation getColorAllocation() const {
		assert(paints->getNumPaints() == graphics->getPaintIndices()->getNumPaints());

		int numColors = 1;
		for (const auto &feed : feeds) {
			numColors = std::max(numColors, feed.getNumColors());
		}

		return TovePaintColorAllocation{
			int16_t(paints->getNumPaints()),
			int16_t(paints->getNumGradients()),
			int16_t(numColors)};
	}

	void bindPaintIndices(const ToveGradientData &data) {
		const int n = feeds.size();
		for (int i = 0; i < n; i++) {
			feeds[i].bindPaintIndices(data);
		}

		// initially setup matrix[0] as 0 matrix for solid colors.
		// we initialize once now and never write to matrix[0] again.

		float *m = data.matrix;
		if (m) {
			// use a mat3(0) matrix here to map all coords to (0, 0).
			// this allows us to only init only the first row in the
			// texture write in AbstractPaintFeed::update() - otherwise
			// we'd have to fill full colorsTextureHeight on each solid
			// color change.

			m[0] = 0;
			m[1] = 0;
			m[2] = 0;

			m[3] = 0;
			m[4] = 0;
			m[5] = 0;

			const int rows = data.matrixRows;
			if (rows >= 3) { // i.e. mat3x3
				m[6] = 0;
				m[7] = 0;
				m[8] = 0;

				if (rows == 4) { // i.e. mat3x4
					m[9] = 0;
					m[10] = 0;
					m[11] = 0;
				}
			}
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
