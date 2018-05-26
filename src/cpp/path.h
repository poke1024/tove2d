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

#include "trajectory.h"
#include "paint.h"
#include "claimable.h"
#include "mesh/flatten.h"

class Path : public Claimable<Graphics> {
private:
	bool newTrajectory;
	std::vector<TrajectoryRef> trajectories;

	PaintRef fillColor;
	PaintRef lineColor;

	uint8_t changes;
	bool boundsDirty;

	inline const TrajectoryRef &current() const {
		return trajectories[trajectories.size() - 1];
	}

	void _append(const TrajectoryRef &trajectory);

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

	TrajectoryRef beginTrajectory();
	void closeTrajectory(bool closeIndeed = false);

	void updateBounds(int from = 0);

	void addTrajectory(const TrajectoryRef &t);

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

	void transform(float sx, float sy, float tx, float ty);

	inline int getNumTrajectories() const {
		return trajectories.size();
	}

	inline TrajectoryRef getTrajectory(int i) const {
		return trajectories.at(i);
	}

	int getNumCurves() const;

	inline float getLineWidth() const {
		return nsvg.strokeWidth;
	}

	void setLineWidth(float width);

	ToveFillRule getFillRule() const;
	void setFillRule(ToveFillRule rule);

	void animate(const PathRef &a, const PathRef &b, float t);

	PathRef clone() const;

	void clean(float eps = 0.0);

	void setOrientation(ToveOrientation orientation);

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

	inline int getTrajectorySize(int i, const FixedFlattener &flattener) const {
		return flattener.size(&trajectories[i]->nsvg);
	}

	inline bool hasStroke() const {
		return nsvg.stroke.type > 0 && nsvg.strokeWidth > 0.0f;
	}
};

#endif // __TOVE_PATH
