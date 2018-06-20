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

#ifndef __TOVE_GRAPHICS
#define __TOVE_GRAPHICS 1

#include "path.h"

class Graphics {
private:
	std::vector<PathRef> paths;
	bool newPath;

	float bounds[4];
	float exactBounds[4];

	PaintRef fillColor;
	PaintRef strokeColor;

	float strokeWidth;
	std::vector<float> strokeDashes;
	float strokeDashOffset;
	int strokeLineJoin;
	int strokeLineCap;
	int miterLimit;
	int fillRule;

	ToveChangeFlags changes;

	inline const PathRef &current() const {
		return paths[paths.size() - 1];
	}

	void setNumPaths(int n);
	void _appendPath(const PathRef &path);

	PathRef beginPath();
	void closePath(bool closeCurves = false);

	void initialize(float width, float height);

	template<typename Get>
	void computeBounds(float *bounds, const Get &get) {
	    for (int i = 0; i < paths.size(); i++) {
	        const float *pathBounds = get(paths[i]);
	        if (i == 0) {
	            for (int j = 0; j < 4; j++) {
	                bounds[j] = pathBounds[j];
	            }
	        } else {
	            bounds[0] = std::min(bounds[0], pathBounds[0]);
	            bounds[1] = std::min(bounds[1], pathBounds[1]);
	            bounds[2] = std::max(bounds[2], pathBounds[2]);
	            bounds[3] = std::max(bounds[3], pathBounds[3]);
	        }
	    }
	}

public:
	NSVGimage nsvg;

	Graphics();
	Graphics(const NSVGimage *image);
	Graphics(const GraphicsRef &graphics);

	inline ~Graphics() {
		clear();
	}

	void clear();
	SubpathRef beginSubpath();
	void closeSubpath();
	void invertSubpath();

	inline void setFillColor(const PaintRef &color) {
		fillColor = color;
	}

	void setLineDash(const float *dashes, int count);

	inline void setLineDashOffset(float offset) {
		strokeDashOffset = offset;
	}

	inline void setLineWidth(float strokeWidth) {
 		this->strokeWidth = strokeWidth;
	}

	inline void setMiterLimit(float limit) {
		this->miterLimit = limit;
	}

	inline void setLineColor(const PaintRef &color) {
		strokeColor = color;
	}

	void fill();
	void stroke();

	inline PathRef getCurrentPath() const {
		if (!paths.empty()) {
			return current();
		} else {
			return PathRef();
		}
	}

	void addPath(const PathRef &path);

	inline int getNumPaths() const {
		return paths.size();
	}

	inline PathRef getPath(int i) const {
		return paths.at(i);
	}

	NSVGimage *getImage();

	const float *getBounds();
	const float *getExactBounds();

	void clean(float eps = 0.0);
	PathRef hit(float x, float y) const;

	void setOrientation(ToveOrientation orientation);

	void set(const GraphicsRef &source, const nsvg::Transform &transform);

	inline void changed(ToveChangeFlags flags) {
		if (flags & (CHANGED_GEOMETRY | CHANGED_POINTS | CHANGED_BOUNDS)) {
			flags |= CHANGED_BOUNDS | CHANGED_EXACT_BOUNDS;
		}
		changes |= flags;
	}

	ToveChangeFlags fetchChanges(ToveChangeFlags flags, bool clearAll = false);

	void animate(const GraphicsRef &a, const GraphicsRef &b, float t);
};

#endif // __TOVE_GRAPHICS
