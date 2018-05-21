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
	bool newTrajectory;

	float bounds[4];
	bool boundsDirty;

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

	void _appendPath(const PathRef &path);

	void beginPath();
	void closePath(bool closeIndeed = false);

	void initialize(float width, float height);

public:
	NSVGimage nsvg;

	Graphics();
	Graphics(const NSVGimage *image);
	Graphics(const GraphicsRef &graphics);

	inline ~Graphics() {
		clear();
	}

	void clear();
	PathRef beginTrajectory();
	void closeTrajectory();

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

	void clean(float eps = 0.0);

	void setOrientation(ToveOrientation orientation);

	void transform(float sx, float sy, float tx, float ty);

	inline void changed(ToveChangeFlags flags) {
		if (flags & (CHANGED_GEOMETRY | CHANGED_POINTS)) {
			boundsDirty = true;
		}
		changes |= flags;
	}

	ToveChangeFlags fetchChanges(ToveChangeFlags flags, bool clearAll = false);

	void animate(const GraphicsRef &a, const GraphicsRef &b, float t);
};

#endif // __TOVE_GRAPHICS
