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
#include "observer.h"
#include "mesh/flatten.h"

BEGIN_TOVE_NAMESPACE

class Path : public Observable, public Observer {
private:
	bool newSubpath;
	std::vector<SubpathRef> subpaths;
#ifdef NSVG_CLIP_PATHS
	std::vector<TOVEclipPathIndex> clipIndices;
#endif

	PaintRef fillColor;
	PaintRef lineColor;
	std::string name;

	uint8_t changes;
	float exactBounds[4];

	inline const SubpathRef &current() const {
		return subpaths[subpaths.size() - 1];
	}

	void setSubpathCount(int n);
	void _append(const SubpathRef &trajectory);

	void _setFillColor(const PaintRef &color);
	void _setLineColor(const PaintRef &color);

	void set(const NSVGshape *shape);

	void animateLineDash(const PathRef &a, const PathRef &b, float t, int pathIndex);

public:
	NSVGshape nsvg;

	Path();
	Path(const NSVGshape *shape);
	Path(const char *d);
	Path(const Path *path);

	virtual ~Path() {
		clear();
	}

	void removeSubpaths();
	void clear();

	SubpathRef beginSubpath();
	void closeSubpath(bool closeCurves = false);
	void invertSubpath();

	void updateBoundsPartial(int from);
	void updateBounds();

	const float *getBounds();
	const float *getExactBounds();

	void addSubpath(const SubpathRef &t);

	void setName(const char *name);
	inline const char *getName() const {
		return name.c_str();
	}

	inline void setFillColor(const PaintRef &color) {
		_setFillColor(color);
	}

	inline const PaintRef &getFillColor() const {
		return fillColor;
	}

	bool areColorsSolid() const;
	PathPaintInd createPaintIndices(PaintIndex &it) const;

	void setLineDash(const float *dashes, const int count);
	inline float getLineDashOffset() const {
		return nsvg.strokeDashOffset;
	}
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


	inline bool hasOpaqueLine() const {
		return nsvg.opacity >= 1.0f && lineColor && lineColor->isOpaque();
	}

	void setOpacity(float opacity);

	void set(const PathRef &path, const nsvg::Transform &transform);

	inline int getNumSubpaths() const {
		return subpaths.size();
	}

	inline SubpathRef getSubpath(int i) const {
		return subpaths.at(i);
	}

	int getNumCurves() const;

	inline float getLineWidth() const {
		return nsvg.strokeWidth;
	}
	void setLineWidth(float width);

	ToveLineJoin getLineJoin() const;
	void setLineJoin(ToveLineJoin join);

	ToveLineCap getLineCap() const;
	void setLineCap(ToveLineCap cap);

	inline float getMiterLimit() const {
		return nsvg.miterLimit;
	}
	void setMiterLimit(float limit);

	ToveFillRule getFillRule() const;
	void setFillRule(ToveFillRule rule);

	void animate(const PathRef &a, const PathRef &b, float t, int pathIndex);
	void refine(int factor);
	void rotate(ToveElementType what, int k);
	static bool morphify(const std::vector<PathRef> &paths);

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

	void changed(ToveChangeFlags flags);

	virtual void observableChanged(Observable *observable, ToveChangeFlags flags);

	inline int getSubpathSize(int i, const RigidFlattener &flattener) const {
		return flattener.size(subpaths[i]);
	}

	int getFlattenedSize(const RigidFlattener &flattener) const;

	inline bool hasStroke() const {
		return nsvg.stroke.type > 0 && nsvg.strokeWidth > 0.0f;
	}

	inline bool hasFill() const {
		return nsvg.fill.type > 0;
	}

	inline bool hasNormalFillStrokeOrder() const {
		int order[NSVG_PAINTORDER_COUNT];
		for (int i = 0; i < NSVG_PAINTORDER_COUNT; i++) {
			int p = (int)nsvg.paintOrder[i];
			assert(p >= 0 && p < NSVG_PAINTORDER_COUNT);
			order[p] = i;
		}
		return order[NSVG_PAINTORDER_FILL] < order[NSVG_PAINTORDER_STROKE];
	}

#ifdef NSVG_CLIP_PATHS
	const std::vector<TOVEclipPathIndex> &getClipIndices() {
		return clipIndices;
	}
#endif

	ClipperLib::PolyFillType getClipperFillType() const;
};

END_TOVE_NAMESPACE

#endif // __TOVE_PATH
