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

#include "common.h"
#include "path.h"
#include "graphics.h"

inline NSVGlineJoin nsvgLineJoin(ToveLineJoin join) {
	switch (join) {
		case LINEJOIN_MITER:
			return NSVG_JOIN_MITER;
		case LINEJOIN_ROUND:
			return NSVG_JOIN_ROUND;
		case LINEJOIN_BEVEL:
			return NSVG_JOIN_BEVEL;
	}
	return NSVG_JOIN_MITER;
}

void Path::_append(const TrajectoryRef &trajectory) {
	if (trajectories.empty()) {
		nsvg.paths = &trajectory->nsvg;
	} else {
		trajectories[trajectories.size() - 1]->setNext(trajectory);
	}
	trajectory->claim(this);
	trajectories.push_back(trajectory);
	if (fillColor) {
		trajectory->setIsClosed(true);
	}
	changed(CHANGED_GEOMETRY);
}

void Path::_setFillColor(const PaintRef &color) {
	if (fillColor == color) {
		return;
	}

	if (fillColor) {
		fillColor->unclaim(this);
	}

	fillColor = color;

	if (color) {
		color->claim(this);
		color->store(nsvg.fill);
	} else {
		nsvg.fill.type = NSVG_PAINT_NONE;
	}

	for (int i = 0; i < trajectories.size(); i++) {
		trajectories[i]->setIsClosed((bool)fillColor);
	}

	changed(CHANGED_FILL_STYLE);
}

void Path::_setLineColor(const PaintRef &color) {
	if (lineColor == color) {
		return;
	}

	if (lineColor) {
		lineColor->unclaim(this);
	}

	lineColor = color;

	if (color) {
		color->claim(this);
		color->store(nsvg.stroke);
	} else {
		nsvg.stroke.type = NSVG_PAINT_NONE;
	}

	changed(CHANGED_LINE_STYLE);
}

void Path::set(const NSVGshape *shape) {
	memset(&nsvg, 0, sizeof(nsvg));

	strcpy(nsvg.id, shape->id);
	_setLineColor(NSVGpaintToPaint(shape->stroke));
	_setFillColor(NSVGpaintToPaint(shape->fill));
	nsvg.opacity = shape->opacity;
	nsvg.strokeWidth = shape->strokeWidth;
	nsvg.strokeDashOffset = shape->strokeDashOffset;
	for (int i = 0; i < shape->strokeDashCount; i++) {
		nsvg.strokeDashArray[i] = shape->strokeDashArray[i];
	}
	nsvg.strokeDashCount = shape->strokeDashCount;
	nsvg.strokeLineJoin = shape->strokeLineJoin;
	nsvg.strokeLineCap = shape->strokeLineCap;
	nsvg.miterLimit = shape->miterLimit;
	nsvg.fillRule = shape->fillRule;
	nsvg.flags = shape->flags;
	for (int i = 0; i < 4; i++) {
		nsvg.bounds[i] = shape->bounds[i];
	}
	boundsDirty = true;

	NSVGpath *path = shape->paths;
	while (path) {
		_append(std::make_shared<Trajectory>(path));
		path = path->next;
	}
}

Path::Path() : changes(0) {
	memset(&nsvg, 0, sizeof(nsvg));

	nsvg.stroke.type = NSVG_PAINT_NONE;
	nsvg.fill.type = NSVG_PAINT_NONE;
	nsvg.opacity = 1.0;
	nsvg.strokeWidth = 1.0;
	nsvg.strokeDashOffset = 0.0;
	nsvg.strokeDashCount = 0;
	nsvg.strokeLineJoin = NSVG_JOIN_MITER;
	nsvg.strokeLineCap = NSVG_CAP_BUTT;
	nsvg.miterLimit = 4;
	nsvg.fillRule = NSVG_FILLRULE_NONZERO;
	nsvg.flags = NSVG_FLAGS_VISIBLE;
	for (int i = 0; i < 4; i++) {
		nsvg.bounds[i] = 0.0;
	}
	boundsDirty = true;

	newTrajectory = true;
}

Path::Path(Graphics *graphics) : changes(0) {
	memset(&nsvg, 0, sizeof(nsvg));
	nsvg.stroke.type = NSVG_PAINT_NONE;
	nsvg.fill.type = NSVG_PAINT_NONE;
	nsvg.opacity = 1.0;
	nsvg.strokeWidth = 1.0;
	nsvg.strokeDashOffset = 0.0;
	nsvg.strokeDashCount = 0;
	nsvg.strokeLineJoin = NSVG_JOIN_MITER;
	nsvg.strokeLineCap = NSVG_CAP_BUTT;
	nsvg.miterLimit = 4;
	nsvg.fillRule = NSVG_FILLRULE_NONZERO;
	nsvg.flags = NSVG_FLAGS_VISIBLE;
	for (int i = 0; i < 4; i++) {
		nsvg.bounds[i] = 0.0;
	}
	boundsDirty = true;

	newTrajectory = true;
	claim(graphics);
}

Path::Path(Graphics *graphics, const NSVGshape *shape) : changes(0) {
	set(shape);
	newTrajectory = true;
	claim(graphics);
}

Path::Path(const char *d) : changes(0) {
	NSVGimage *image = nsvg::parsePath(d);
	set(image->shapes);
	nsvgDelete(image);

	newTrajectory = true;
}

Path::Path(const Path *path) : changes(0) {
	memset(&nsvg, 0, sizeof(nsvg));

	strcpy(nsvg.id, path->nsvg.id);
	_setFillColor(path->fillColor->clone());
	_setLineColor(path->lineColor->clone());

	nsvg.opacity = path->nsvg.opacity;
	nsvg.strokeWidth = path->nsvg.strokeWidth;
	nsvg.strokeDashOffset = path->nsvg.strokeDashOffset;
	for (int i = 0; i < path->nsvg.strokeDashCount; i++) {
		nsvg.strokeDashArray[i] = path->nsvg.strokeDashArray[i];
	}
	nsvg.strokeDashCount = path->nsvg.strokeDashCount;
	nsvg.strokeLineJoin = path->nsvg.strokeLineJoin;
	nsvg.strokeLineCap = path->nsvg.strokeLineCap;
	nsvg.miterLimit = path->nsvg.miterLimit;
	nsvg.fillRule = path->nsvg.fillRule;
	nsvg.flags = path->nsvg.flags;
	for (int i = 0; i < 4; i++) {
		nsvg.bounds[i] = path->nsvg.bounds[i];
	}
	boundsDirty = path->boundsDirty;

	trajectories.reserve(path->trajectories.size());
	for (int i = 0; i < path->trajectories.size(); i++) {
		_append(std::make_shared<Trajectory>(path->trajectories[i]));
	}

	newTrajectory = true;
}

void Path::clear() {
	if (trajectories.size() > 0) {
		for (int i = 0; i < trajectories.size(); i++) {
			trajectories[i]->unclaim(this);
		}
		trajectories.clear();
		nsvg.paths = nullptr;
		changed(CHANGED_GEOMETRY);
	}
}

TrajectoryRef Path::beginTrajectory() {
	if (!newTrajectory) {
		return current();
	}
	closeTrajectory();

	TrajectoryRef trajectory = std::make_shared<Trajectory>();
	trajectory->claim(this);
	if (trajectories.empty()) {
		nsvg.paths = &trajectory->nsvg;
	} else {
		current()->setNext(trajectory);
	}
	trajectories.push_back(trajectory);
	newTrajectory = false;

	return trajectory;
}

void Path::closeTrajectory(bool closeIndeed) {
	if (!trajectories.empty()) {
		const TrajectoryRef &t = current();
		t->updateBounds();
		updateBounds(trajectories.size() - 1);
		if (closeIndeed) {
			t->setIsClosed(true);
		}
	}
	newTrajectory = true;
}

void Path::updateBounds(int from) {
	if (!boundsDirty) {
		return;
	}
	float w = nsvg.strokeWidth * 0.5;
	for (int i = from; i < trajectories.size(); i++) {
		const TrajectoryRef &t = trajectories[i];
		t->updateBounds();
		if (i == 0) {
			nsvg.bounds[0] = t->nsvg.bounds[0] - w;
			nsvg.bounds[1] = t->nsvg.bounds[1] - w;
			nsvg.bounds[2] = t->nsvg.bounds[2] + w;
			nsvg.bounds[3] = t->nsvg.bounds[3] + w;
		} else {
			nsvg.bounds[0] = std::min(nsvg.bounds[0], t->nsvg.bounds[0] - w);
			nsvg.bounds[1] = std::min(nsvg.bounds[1], t->nsvg.bounds[1] - w);
			nsvg.bounds[2] = std::max(nsvg.bounds[2], t->nsvg.bounds[2] + w);
			nsvg.bounds[3] = std::max(nsvg.bounds[3], t->nsvg.bounds[3] + w);
		}
	}
	boundsDirty = false;
}

void Path::addTrajectory(const TrajectoryRef &t) {
	closeTrajectory();
	_append(t);
}

void Path::setLineDash(const float *dashes, int count) {
	float sum = 0;
	for (int i = 0; i < count; i++) {
		sum += dashes[i];
	}
	if (sum <= 1e-6f) {
		count = 0;
	}

	nsvg.strokeDashCount = std::min(count, nsvg::maxDashes());
	for (int i = 0; i < count; i++) {
		nsvg.strokeDashArray[i] = dashes[i];
	}

	geometryChanged();
}

void Path::setLineDashOffset(float offset) {
	nsvg.strokeDashOffset = offset;
	geometryChanged();
}

void Path::setOpacity(float opacity) {
	if (opacity != nsvg.opacity) {
		nsvg.opacity = opacity;
		changed(CHANGED_LINE_STYLE);
		changed(CHANGED_FILL_STYLE);
	}
}

void Path::transform(float sx, float sy, float tx, float ty) {
	closeTrajectory();

	nsvg.bounds[0] = (nsvg.bounds[0] + tx) * sx;
	nsvg.bounds[1] = (nsvg.bounds[1] + ty) * sy;
	nsvg.bounds[2] = (nsvg.bounds[2] + tx) * sx;
	nsvg.bounds[3] = (nsvg.bounds[3] + ty) * sy;

	for (int i = 0; i < trajectories.size(); i++) {
		trajectories[i]->transform(sx, sy, tx, ty);
	}

	if (fillColor) {
		fillColor->transform(sx, sy, tx, ty);
	}
	if (lineColor) {
		lineColor->transform(sx, sy, tx, ty);
	}

	const float avgs = (sx + sy) / 2.0f;
	nsvg.strokeWidth *= avgs;
	nsvg.strokeDashOffset *= avgs;
	for (int i = 0; i < nsvg.strokeDashCount; i++) {
		nsvg.strokeDashArray[i] *= avgs;
	}
}

int Path::getNumCurves() const {
	int numCurves = 0;
	for (int i = 0; i < trajectories.size(); i++) {
		numCurves += trajectories[i]->getNumCurves();
	}
	return numCurves;
}

void Path::setLineWidth(float width) {
	const float oldWidth = nsvg.strokeWidth;
	if (width != oldWidth) {
		nsvg.strokeWidth = width;
		if ((oldWidth > 0.0) == (width > 0.0)) {
			changed(CHANGED_POINTS | CHANGED_LINE_ARGS);
		} else {
			geometryChanged();
		}
	}
}

void Path::setLineJoin(ToveLineJoin join) {
	NSVGlineJoin nsvgJoin = nsvgLineJoin(join);
	if (nsvgJoin != nsvg.strokeLineJoin) {
		nsvg.strokeLineJoin = nsvgJoin;
		geometryChanged();
	}
}

void Path::setMiterLimit(float limit) {
	if (limit != nsvg.miterLimit) {
		nsvg.miterLimit = limit;
		changed(CHANGED_POINTS | CHANGED_LINE_ARGS);
	}
}

ToveFillRule Path::getFillRule() const {
	switch (static_cast<NSVGfillRule>(nsvg.fillRule)) {
		case NSVG_FILLRULE_NONZERO:
			return FILLRULE_NON_ZERO;
		case NSVG_FILLRULE_EVENODD:
			return FILLRULE_EVEN_ODD;
		default:
			return FILLRULE_NON_ZERO;
	}
}

void Path::setFillRule(ToveFillRule rule) {
	NSVGfillRule nsvgRule = NSVG_FILLRULE_NONZERO;

	if (rule == FILLRULE_NON_ZERO) {
		nsvgRule = NSVG_FILLRULE_EVENODD;
	} else if (rule == FILLRULE_EVEN_ODD) {
		nsvgRule = NSVG_FILLRULE_NONZERO;
	}

	if (nsvgRule != nsvg.fillRule) {
		nsvg.fillRule = nsvgRule;
		geometryChanged();
	}
}

void Path::animate(const PathRef &a, const PathRef &b, float t) {
	const int n = a->trajectories.size();
	if (n != b->trajectories.size()) {
		return;
	}
	if (trajectories.size() != n) {
		clear();
		while (trajectories.size() < n) {
			addTrajectory(std::make_shared<Trajectory>());
		}
	}
	for (int i = 0; i < n; i++) {
		trajectories[i]->animate(a->trajectories[i], b->trajectories[i], t);
	}
	if (a->fillColor && b->fillColor) {
		if (!fillColor) {
			setFillColor(a->fillColor->clone());
		}
		fillColor->animate(a->fillColor, b->fillColor, t);
	} else {
		setFillColor(PaintRef());
	}
	if (a->lineColor && b->lineColor) {
		if (!lineColor) {
			setLineColor(a->lineColor->clone());
		}
		lineColor->animate(a->lineColor, b->lineColor, t);
	} else {
		setLineColor(PaintRef());
	}
	setLineWidth(a->getLineWidth() * (1 - t) + b->getLineWidth() * t);
}

PathRef Path::clone() const {
	return std::make_shared<Path>(this);
}

void Path::clean(float eps) {
	for (int i = 0; i < trajectories.size(); i++) {
		trajectories[i]->clean(eps);
	}
}

void Path::setOrientation(ToveOrientation orientation) {
	for (int i = 0; i < trajectories.size(); i++) {
		trajectories[i]->setOrientation(orientation);
	}
}

void Path::updateNSVG() {
	updateBounds();

	for (int i = 0; i < trajectories.size(); i++) {
		trajectories[i]->updateNSVG();
	}
}

void Path::geometryChanged() {
	changed(CHANGED_GEOMETRY);
}

void Path::colorChanged(AbstractPaint *paint) {
	if (paint == lineColor.get()) {
		changed(CHANGED_LINE_STYLE);
		lineColor->store(nsvg.stroke);
	} else if (paint == fillColor.get()) {
		changed(CHANGED_FILL_STYLE);
		fillColor->store(nsvg.fill);
	}
}

void Path::clearChanges(ToveChangeFlags flags) {
	changes &= ~flags;
	for (int i = 0; i < trajectories.size(); i++) {
		trajectories[i]->clearChanges(flags);
	}
}

void Path::changed(ToveChangeFlags flags) {
	changes |= flags;
	if (flags & (CHANGED_GEOMETRY | CHANGED_POINTS)) {
		boundsDirty = true;
	}
	if (claimer) {
		claimer->changed(flags);
	}
}
