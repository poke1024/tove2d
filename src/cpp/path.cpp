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
#include "intersect.h"

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

void Path::setTrajectoryCount(int n) {
	if (trajectories.size() != n) {
		clear();
		while (trajectories.size() < n) {
			addTrajectory(std::make_shared<Trajectory>());
		}
	}
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

	for (const auto &t : trajectories) {
		t->setIsClosed((bool)fillColor);
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

	NSVGpath *path = shape->paths;
	while (path) {
		_append(std::make_shared<Trajectory>(path));
		path = path->next;
	}
	changed(CHANGED_GEOMETRY);
}

Path::Path() : changes(CHANGED_BOUNDS) {
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

	newTrajectory = true;
}

Path::Path(Graphics *graphics) : changes(CHANGED_BOUNDS) {
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

Path::Path(const Path *path) : changes(path->changes) {
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

	trajectories.reserve(path->trajectories.size());
	for (int i = 0; i < path->trajectories.size(); i++) {
		_append(std::make_shared<Trajectory>(path->trajectories[i]));
	}

	newTrajectory = true;
}

void Path::clear() {
	if (trajectories.size() > 0) {
		for (const auto &t : trajectories) {
			t->unclaim(this);
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

void Path::closeTrajectory(bool closeCurves) {
	if (newTrajectory) {
		if (closeCurves && !trajectories.empty()) {
			current()->setIsClosed(true);
		}
		return;
	}
	if (!trajectories.empty()) {
		const TrajectoryRef &t = current();
		t->updateBounds();
		if ((changes & CHANGED_BOUNDS) == 0) {
			updateBoundsPartial(trajectories.size() - 1);
		}
		if (closeCurves) {
			t->setIsClosed(true);
		}
	}
	newTrajectory = true;
}

void Path::updateBoundsPartial(int from) {
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
	changes &= ~CHANGED_BOUNDS;
}

void Path::updateBounds() {
	if ((changes & CHANGED_BOUNDS) == 0) {
		return;
	}
	updateBoundsPartial(0);
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

void Path::set(const PathRef &path, const nsvg::Transform &transform) {
	const int n = path->trajectories.size();

	const float lineScale = transform.wantsScaleLineWidth() ?
		transform.getScale() : 1.0f;

	bool hasGeometryChanged = false;

	if (path.get() != this) {
		setOpacity(path->nsvg.opacity);
		if (nsvg.strokeDashCount != path->nsvg.strokeDashCount) {
			nsvg.strokeDashCount = path->nsvg.strokeDashCount;
			hasGeometryChanged = true;
		}
		if (nsvg.strokeLineJoin != path->nsvg.strokeLineJoin) {
			nsvg.strokeLineJoin = path->nsvg.strokeLineJoin;
			hasGeometryChanged = true;
		}
		if (nsvg.strokeLineCap != path->nsvg.strokeLineCap) {
			nsvg.strokeLineCap = path->nsvg.strokeLineCap;
			hasGeometryChanged = true;
		}
		setMiterLimit(path->nsvg.miterLimit);
		if (nsvg.fillRule != path->nsvg.fillRule) {
			nsvg.fillRule = path->nsvg.fillRule;
			hasGeometryChanged = true;
		}
		if (nsvg.flags != path->nsvg.flags) {
			nsvg.flags = path->nsvg.flags;
			hasGeometryChanged = true;
		}
	}

	if (path.get() != this || !transform.isIdentity()) {
		{
			PaintRef newFillColor;
			if (path->fillColor) {
				newFillColor = fillColor;
				path->fillColor->cloneTo(newFillColor, transform);
			}
			_setFillColor(newFillColor);
		}

		{
			PaintRef newLineColor;
			if (path->lineColor) {
				newLineColor = lineColor;
				path->lineColor->cloneTo(newLineColor, transform);
			}
			_setLineColor(newLineColor);
		}
	}

	if (path.get() != this || lineScale != 1.0f) {
		setLineWidth(path->nsvg.strokeWidth * lineScale);

		const float newDashOffset = path->nsvg.strokeDashOffset * lineScale;
		if (nsvg.strokeDashOffset != newDashOffset) {
			nsvg.strokeDashOffset = newDashOffset;
			hasGeometryChanged = true;
		}
		for (int i = 0; i < path->nsvg.strokeDashCount; i++) {
			const float len = path->nsvg.strokeDashArray[i] * lineScale;
			if (nsvg.strokeDashArray[i] != len) {
				nsvg.strokeDashArray[i] = len;
				hasGeometryChanged = true;
			}
		}
	}

	if (hasGeometryChanged) {
		geometryChanged();
	}

	setTrajectoryCount(n);

	for (int i = 0; i < n; i++) {
		trajectories[i]->set(path->trajectories[i], transform);
	}
}

int Path::getNumCurves() const {
	int numCurves = 0;
	for (const auto &t : trajectories) {
		numCurves += t->getNumCurves();
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
	setTrajectoryCount(n);
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
	for (const auto &t : trajectories) {
		t->clean(eps);
	}
}

void Path::setOrientation(ToveOrientation orientation) {
	for (const auto &t : trajectories) {
		t->setOrientation(orientation);
	}
}

bool Path::isInside(float x, float y) {
	updateBounds();
	if (x < nsvg.bounds[0] || x > nsvg.bounds[2]) {
		return false;
	}
	if (y < nsvg.bounds[1] || y > nsvg.bounds[3]) {
		return false;
	}

	switch (getFillRule()) {
		case FILLRULE_NON_ZERO: {
			NonZeroInsideTest test;
			for (const auto &t : trajectories) {
				t->testInside(x, y, test);
			}
			return test.get();
		} break;
		case FILLRULE_EVEN_ODD: {
			EvenOddInsideTest test;
			for (const auto &t : trajectories) {
				t->testInside(x, y, test);
			}
			return test.get();
		}
		default: {
			return false;
		} break;
	}
}

void Path::intersect(float x1, float y1, float x2, float y2) const {
	RuntimeRay ray(x1, y1, x2, y2);
	Intersecter intersecter;
	for (const auto &t : trajectories) {
		t->intersect(ray, intersecter);
	}
	// return intersecter.get()
}

void Path::updateNSVG() {
	updateBounds();

	for (const auto &t : trajectories) {
		t->updateNSVG();
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
	for (const auto &t : trajectories) {
		t->clearChanges(flags);
	}
}

void Path::changed(ToveChangeFlags flags) {
	if (flags & (CHANGED_GEOMETRY | CHANGED_POINTS)) {
		flags |= CHANGED_BOUNDS;
	}
	if ((changes & flags) == flags) {
		return;
	}
	changes |= flags;
	if (claimer) {
		claimer->changed(flags);
	}
}
