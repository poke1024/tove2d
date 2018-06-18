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

#ifndef __TOVE_PATH
#define __TOVE_PATH 1

#include "subpath.h"
#include "paint.h"
#include "claimable.h"
#include "mesh/flatten.h"

class Path : public Claimable<Graphics> {
private:
	bool newSubpath;
	std::vector<SubpathRef> trajectories;

	PaintRef fillColor;
	PaintRef lineColor;

	uint8_t changes;
	float exactBounds[4];

	inline const SubpathRef &current() const {
		return trajectories[trajectories.size() - 1];
	}

	void setSubpathCount(int n);
	void _append(const SubpathRef &trajectory);

	void _setFillColor(const PaintRef &color);
	void _setLineColor(const PaintRef &color);

	void set(const NSVGshape *shape);

public:
	NSVGshape nsvg;

	Path();
	Path(Graphics *graphics);
	Path(Graphics *graphics, const NSVGshape *shape);
	Path(const char *d);
	Path(const Path *path);

	inline ~Path() {
		clear();
	}

	void clear();

	SubpathRef beginSubpath();
	void closeSubpath(bool closeCurves = false);
	void invertSubpath();

	void updateBoundsPartial(int from);
	void updateBounds();

	const float *getBounds();
	const float *getExactBounds();

	void addSubpath(const SubpathRef &t);

	inline void setFillColor(const PaintRef &color) {
		_setFillColor(color);
	}

	inline const PaintRef &getFillColor() const {
		return fillColor;
	}

	void setLineDash(const float *dashes, int count);
	void setLineDashOffset(float offset);

	inline void setLineColor(const PaintRef &color) {
		_setLineColor(color);
	}

	inline const PaintRef &getLineColor() const {
		return lineColor;
	}

	inline float getOpacity() const {
		return nsvg.opacity;
	}

	void setOpacity(float opacity);

	void set(const PathRef &path, const nsvg::Transform &transform);

	inline int getNumSubpaths() const {
		return trajectories.size();
	}

	inline SubpathRef getSubpath(int i) const {
		return trajectories.at(i);
	}

	int getNumCurves() const;

	inline float getLineWidth() const {
		return nsvg.strokeWidth;
	}
	void setLineWidth(float width);

	ToveLineJoin getLineJoin() const;
	void setLineJoin(ToveLineJoin join);

	inline float getMiterLimit() const {
		return nsvg.miterLimit;
	}
	void setMiterLimit(float limit);

	ToveFillRule getFillRule() const;
	void setFillRule(ToveFillRule rule);

	void animate(const PathRef &a, const PathRef &b, float t);

	PathRef clone() const;

	void clean(float eps = 0.0);

	void setOrientation(ToveOrientation orientation);

	bool isInside(float x, float y);
	void intersect(float x1, float y1, float x2, float y2) const;

public:
	inline void setNext(const PathRef &path) {
		nsvg.next = &path->nsvg;
	}

	inline void clearNext() {
		nsvg.next = nullptr;
	}

	void updateNSVG();

	inline NSVGshape *getNSVG() {
		updateNSVG();
		return &nsvg;
	}

	void geometryChanged();

	void colorChanged(AbstractPaint *paint);

	inline bool hasChange(ToveChangeFlags change) const {
		return changes & change;
	}

	inline ToveChangeFlags fetchChanges(ToveChangeFlags change) {
		const ToveChangeFlags f = changes & change;
		changes &= ~change;
		return f;
	}

	void clearChanges(ToveChangeFlags flags);

	void changed(ToveChangeFlags flags);

	inline int getSubpathSize(int i, const FixedFlattener &flattener) const {
		return flattener.size(&trajectories[i]->nsvg);
	}

	inline bool hasStroke() const {
		return nsvg.stroke.type > 0 && nsvg.strokeWidth > 0.0f;
	}
};

#endif // __TOVE_PATH
