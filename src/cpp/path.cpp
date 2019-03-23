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
#include "nsvg.h"
#include <sstream>

BEGIN_TOVE_NAMESPACE

void Path::setSubpathCount(int n) {
	if (subpaths.size() != n) {
		removeSubpaths();
		while (subpaths.size() < n) {
			addSubpath(tove_make_shared<Subpath>());
		}
	}
}

void Path::_append(const SubpathRef &trajectory) {
	if (subpaths.empty()) {
		nsvg.paths = &trajectory->nsvg;
	} else {
		subpaths[subpaths.size() - 1]->setNext(trajectory);
	}
	trajectory->addObserver(this);
	subpaths.push_back(trajectory);
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
		fillColor->removeObserver(this);
	}

	fillColor = color;

	if (color) {
		color->addObserver(this);
		color->store(nsvg.fill);
	} else {
		nsvg.fill.type = NSVG_PAINT_NONE;
	}

	for (const auto &t : subpaths) {
		t->setIsClosed((bool)fillColor);
	}

	changed(CHANGED_FILL_STYLE);
}

void Path::_setLineColor(const PaintRef &color) {
	if (lineColor == color) {
		return;
	}

	if ((lineColor.get() != nullptr) != (color.get() != nullptr)) {
		changed(CHANGED_LINE_ARGS);
	}

	if (lineColor) {
		lineColor->removeObserver(this);
	}

	lineColor = color;

	if (color) {
		color->addObserver(this);
		color->store(nsvg.stroke);
	} else {
		nsvg.stroke.type = NSVG_PAINT_NONE;
	}

	changed(CHANGED_LINE_STYLE);
}

bool Path::areColorsSolid() const {
	return nsvg.stroke.type <= NSVG_PAINT_COLOR &&
		nsvg.fill.type <= NSVG_PAINT_COLOR;
}

void Path::set(const NSVGshape *shape) {
	memset(&nsvg, 0, sizeof(nsvg));

	strcpy(nsvg.id, shape->id);
	name = nsvg.id;

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
		exactBounds[i] = 0.0f;
	}

	NSVGpath *path = shape->paths;
	while (path) {
		_append(tove_make_shared<Subpath>(path));
		path = path->next;
	}

#ifdef NSVG_CLIP_PATHS
	nsvg.clip.count = shape->clip.count;
	if (shape->clip.count > 0) {
		clipIndices.resize(shape->clip.count);
		nsvg.clip.index = clipIndices.data();
		std::memcpy(
			nsvg.clip.index,
			shape->clip.index,
			shape->clip.count * sizeof(TOVEclipPathIndex));
	}
#endif

	changed(CHANGED_GEOMETRY);
}

Path::Path() :
	changes(CHANGED_BOUNDS | CHANGED_EXACT_BOUNDS),
	pathIndex(-1) {

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
		nsvg.bounds[i] = 0.0f;
		exactBounds[i] = 0.0f;
	}

	newSubpath = true;
}

Path::Path(const NSVGshape *shape) :
	changes(0),
	pathIndex(-1) {

	set(shape);
	newSubpath = true;
}

Path::Path(const char *d) : changes(0), pathIndex(-1) {
	NSVGimage *image = nsvg::parsePath(d);
	set(image->shapes);
	nsvgDelete(image);

	newSubpath = true;
}

Path::Path(const Path *path) :
	changes(CHANGED_BOUNDS | CHANGED_EXACT_BOUNDS),
	pathIndex(-1) {

	memset(&nsvg, 0, sizeof(nsvg));

	strcpy(nsvg.id, path->nsvg.id);
	name = path->name;

	_setFillColor(path->fillColor ? path->fillColor->clone() : PaintRef());
	_setLineColor(path->lineColor ? path->lineColor->clone() : PaintRef());

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
		exactBounds[i] = path->exactBounds[i];
	}

	subpaths.reserve(path->subpaths.size());
	for (int i = 0; i < path->subpaths.size(); i++) {
		_append(tove_make_shared<Subpath>(path->subpaths[i]));
	}

	newSubpath = true;
}

void Path::removeSubpaths() {
	if (subpaths.size() > 0) {
		for (const auto &t : subpaths) {
			t->removeObserver(this);
		}
		subpaths.clear();
		nsvg.paths = nullptr;
		changed(CHANGED_GEOMETRY);
	}
}

void Path::clear() {
	removeSubpaths();

	_setFillColor(PaintRef());
	_setLineColor(PaintRef());

#ifdef NSVG_CLIP_PATHS
	clipIndices.clear();
	nsvg.clip.index = nullptr;
	nsvg.clip.count = 0;
#endif
}

SubpathRef Path::beginSubpath() {
	if (!newSubpath) {
		return current();
	}
	closeSubpath();

	SubpathRef trajectory = tove_make_shared<Subpath>();
	trajectory->addObserver(this);
	if (subpaths.empty()) {
		nsvg.paths = &trajectory->nsvg;
	} else {
		current()->setNext(trajectory);
	}
	subpaths.push_back(trajectory);
	newSubpath = false;

	return trajectory;
}

void Path::closeSubpath(bool closeCurves) {
	if (newSubpath) {
		if (closeCurves && !subpaths.empty()) {
			current()->setIsClosed(true);
		}
		return;
	}
	if (!subpaths.empty()) {
		const SubpathRef &t = current();
		t->updateBounds();
		if ((changes & CHANGED_BOUNDS) == 0) {
			updateBoundsPartial(subpaths.size() - 1);
		}
		if (closeCurves) {
			t->setIsClosed(true);
		}
	}
	newSubpath = true;
}

void Path::invertSubpath() {
	if (!subpaths.empty()) {
		current()->invert();
	}
}

void Path::updateBoundsPartial(int from) {
	const float w = hasStroke() ? nsvg.strokeWidth * 0.5 : 0.0;
	for (int i = from; i < subpaths.size(); i++) {
		const SubpathRef &t = subpaths[i];
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
}

void Path::updateBounds() {
	if ((changes & CHANGED_BOUNDS) == 0) {
		return;
	}
	updateBoundsPartial(0);
	changes &= ~CHANGED_BOUNDS;
}

const float *Path::getBounds() {
	updateBounds();
	return nsvg.bounds;
}

const float *Path::getExactBounds() {
	if (!hasStroke()) {
		return getBounds();
	}

	if ((changes & CHANGED_EXACT_BOUNDS) == 0) {
		return exactBounds;
	}

	updateNSVG();

	if (!nsvg::shapeStrokeBounds(exactBounds, &nsvg, 1.0f, nullptr)) {
		updateBounds();
		for (int i = 0; i < 4; i++) {
			exactBounds[i] = nsvg.bounds[i];
		}
	}

	changes &= ~CHANGED_EXACT_BOUNDS;
	return exactBounds;
}

void Path::addSubpath(const SubpathRef &t) {
	closeSubpath();
	_append(t);
}

void Path::setName(const char *name) {
	this->name = name;
}

void Path::setLineDash(const float *dashes, const int count) {
	if (count == 0 && nsvg.strokeDashCount == 0) {
		return;
	}

	float sum = 0;
	for (int i = 0; i < count; i++) {
		sum += dashes[i];
	}
	if (sum <= 1e-6f) {
		if (nsvg.strokeDashCount != 0) {
			nsvg.strokeDashCount = 0;
			geometryChanged();
		}
		return;
	}

	bool changed = false;
	const int n = std::min(count, nsvg::maxDashes());

	if (nsvg.strokeDashCount != n) {
		nsvg.strokeDashCount = n;
		changed = true;
	}
	for (int i = 0; i < n; i++) {
		const float d = dashes[i];
		if (nsvg.strokeDashArray[i] != d) {
			nsvg.strokeDashArray[i] = d;
			changed = true;
		}
	}

	if (changed) {
		geometryChanged();
	}
}

void Path::setLineDashOffset(float offset) {
	if (offset != nsvg.strokeDashOffset) {
		nsvg.strokeDashOffset = offset;
		geometryChanged();
	}
}

void Path::setOpacity(float opacity) {
	if (opacity != nsvg.opacity) {
		nsvg.opacity = opacity;
		changed(CHANGED_LINE_STYLE);
		changed(CHANGED_FILL_STYLE);
	}
}

void Path::set(const PathRef &path, const nsvg::Transform &transform) {
	const int n = path->subpaths.size();

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

	setSubpathCount(n);

	for (int i = 0; i < n; i++) {
		subpaths[i]->set(path->subpaths[i], transform);
	}
}

int Path::getNumCurves() const {
	int numCurves = 0;
	for (const auto &t : subpaths) {
		numCurves += t->getNumCurves();
	}
	return numCurves;
}

void Path::setLineWidth(float width) {
	const float oldWidth = nsvg.strokeWidth;
	if (width != oldWidth) {
		nsvg.strokeWidth = width;
		if ((oldWidth > 0.0) != (width > 0.0)) {
			geometryChanged();
		}
		changed(CHANGED_POINTS | CHANGED_LINE_ARGS);
	}
}

ToveLineJoin Path::getLineJoin() const {
	return nsvg::toveLineJoin(
		static_cast<NSVGlineJoin>(nsvg.strokeLineJoin));
}

void Path::setLineJoin(ToveLineJoin join) {
	NSVGlineJoin nsvgJoin = nsvg::nsvgLineJoin(join);
	if (nsvgJoin != nsvg.strokeLineJoin) {
		nsvg.strokeLineJoin = nsvgJoin;
		geometryChanged();
	}
}

void Path::setMiterLimit(float limit) {
	if (limit != nsvg.miterLimit) {
		if ((nsvg.miterLimit != 0.0f) != (limit != 0.0f)) {
			changed(CHANGED_GEOMETRY);
		}
		nsvg.miterLimit = limit;
		changed(CHANGED_POINTS | CHANGED_LINE_ARGS);
	}
}

ToveFillRule Path::getFillRule() const {
	switch (static_cast<NSVGfillRule>(nsvg.fillRule)) {
		case NSVG_FILLRULE_NONZERO:
			return TOVE_FILLRULE_NON_ZERO;
		case NSVG_FILLRULE_EVENODD:
			return TOVE_FILLRULE_EVEN_ODD;
		default:
			return TOVE_FILLRULE_NON_ZERO;
	}
}

void Path::setFillRule(ToveFillRule rule) {
	NSVGfillRule nsvgRule = NSVG_FILLRULE_NONZERO;

	if (rule == TOVE_FILLRULE_NON_ZERO) {
		nsvgRule = NSVG_FILLRULE_EVENODD;
	} else if (rule == TOVE_FILLRULE_EVEN_ODD) {
		nsvgRule = NSVG_FILLRULE_NONZERO;
	}

	if (nsvgRule != nsvg.fillRule) {
		nsvg.fillRule = nsvgRule;
		geometryChanged();
	}
}

void Path::animateLineDash(const PathRef &a, const PathRef &b, float t) {
	const float s = 1.0f - t;

	setLineDashOffset(a->getLineDashOffset() * s + b->getLineDashOffset() * t);

	if (a->nsvg.strokeDashCount == b->nsvg.strokeDashCount) {
		const int n = a->nsvg.strokeDashCount;
		bool changed = false;

		if (n != nsvg.strokeDashCount) {
			nsvg.strokeDashCount = n;
			changed = true;		
		}
		for (int i = 0; i < n; i++) {
			const float dash = a->nsvg.strokeDashArray[i] * s +
				b->nsvg.strokeDashArray[i] * t;

			if (dash != nsvg.strokeDashArray[i]) {
				nsvg.strokeDashArray[i] = dash;
				changed = true;
			}
		}

		if (changed) {
			geometryChanged();
		}
	} else {
		setLineDash(nullptr, 0);

		if (tove::report::warnings()) {
			std::ostringstream message;
			message << "cannot animate over line dash sizes " <<
				a->nsvg.strokeDashCount << " <-> " <<
				b->nsvg.strokeDashCount << " in path "
				<< (pathIndex + 1) << ".";
			tove::report::warn(message.str().c_str());
		}
	}	
}

void Path::animate(const PathRef &a, const PathRef &b, float t) {
	const int n = a->subpaths.size();
	if (n != b->subpaths.size()) {
		if (tove::report::warnings()) {
			std::ostringstream message;
			message << "cannot animate over sub path counts " <<
				a->subpaths.size() << " <-> " <<
				b->subpaths.size() << " in path "
				<< (pathIndex + 1) << ".";
			tove::report::warn(message.str().c_str());
		}
		return;
	}
	setSubpathCount(n);
	for (int i = 0; i < n; i++) {
		if (!subpaths[i]->animate(a->subpaths[i], b->subpaths[i], t)) {
			if (tove::report::warnings()) {
				std::ostringstream message;
				message << "cannot animate over point sizes " <<
					a->subpaths[i]->nsvg.npts << " <-> " <<
					b->subpaths[i]->nsvg.npts << " in sub path "
					<< (pathIndex + 1) << "/" << (i + 1) << ".";
				tove::report::warn(message.str().c_str());
			}
		}
	}
	if (a->fillColor && b->fillColor) {
		if (!fillColor) {
			setFillColor(a->fillColor->clone());
		}
		if (!fillColor->animate(a->fillColor, b->fillColor, t)) {
			if (tove::report::warnings()) {
				std::ostringstream message;
				message << "cannot animate fill color " <<
					" in path " << (pathIndex + 1) << ".";
				tove::report::warn(message.str().c_str());
			}
		}
	} else {
		setFillColor(PaintRef());
	}
	if (a->lineColor && b->lineColor) {
		if (!lineColor) {
			setLineColor(a->lineColor->clone());
		}
		if (!lineColor->animate(a->lineColor, b->lineColor, t)) {
			if (tove::report::warnings()) {
				std::ostringstream message;
				message << "cannot animate line color " <<
					" in path " << (pathIndex + 1) << ".";
				tove::report::warn(message.str().c_str());
			}
		}
	} else {
		setLineColor(PaintRef());
	}

	const float s = 1.0f - t;

	setLineWidth(a->getLineWidth() * s + b->getLineWidth() * t);
	setMiterLimit(a->getMiterLimit() * s + b->getMiterLimit() * t);
	animateLineDash(a, b, t);

	setOpacity(a->getOpacity() * s + b->getOpacity() * t);
}

PathRef Path::clone() const {
	return tove_make_shared<Path>(this);
}

void Path::clean(float eps) {
	for (const auto &t : subpaths) {
		t->clean(eps);
	}
}

void Path::setOrientation(ToveOrientation orientation) {
	for (const auto &t : subpaths) {
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
		case TOVE_FILLRULE_NON_ZERO: {
			NonZeroInsideTest test;
			for (const auto &t : subpaths) {
				if (t->isClosed()) {
					t->testInside(x, y, test);
				}
			}
			return test.get();
		} break;
		case TOVE_FILLRULE_EVEN_ODD: {
			EvenOddInsideTest test;
			for (const auto &t : subpaths) {
				if (t->isClosed()) {
					t->testInside(x, y, test);
				}
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
	for (const auto &t : subpaths) {
		t->intersect(ray, intersecter);
	}
	// return intersecter.get()
}

void Path::updateNSVG() {
	updateBounds();

	NSVGpath **link = &nsvg.paths;

	for (const auto &t : subpaths) {
		t->updateNSVG();

		if (t->getNumPoints() > 0) {
			// NanoSVG will crash on subpaths
			// with 0 points. exlude them now.
			*link = &t->nsvg;
			link = &t->nsvg.next;
		}
	}

	*link = nullptr;
}

void Path::geometryChanged() {
	changed(CHANGED_GEOMETRY);
}

void Path::observableChanged(Observable *observable, ToveChangeFlags flags) {
	if ((flags & CHANGED_COLORS) != 0) {
		ToveChangeFlags newFlags = 0;
		if (observable == lineColor.get()) {
			lineColor->store(nsvg.stroke);
			newFlags |= CHANGED_LINE_STYLE;
		}
		if (observable == fillColor.get()) {
			fillColor->store(nsvg.fill);
			newFlags |= CHANGED_FILL_STYLE;
		}
		flags = newFlags;
	}

	changed(flags);
}

void Path::changed(ToveChangeFlags flags) {
	if (flags & (CHANGED_GEOMETRY | CHANGED_POINTS | CHANGED_BOUNDS)) {
		changes |= CHANGED_BOUNDS | CHANGED_EXACT_BOUNDS;
	}
	broadcastChange(flags);
}

int Path::getFlattenedSize(const RigidFlattener &flattener) const {
	int size = 0;
	for (const auto &subpath : subpaths) {
		size += flattener.size(subpath);
	}
	return size;
}

ClipperLib::PolyFillType Path::getClipperFillType() const {
	switch (nsvg.fillRule) {
		case NSVG_FILLRULE_NONZERO: {
			return ClipperLib::pftNonZero;
		} break;
		case NSVG_FILLRULE_EVENODD: {
			return ClipperLib::pftEvenOdd;
		} break;
		default: {
			return ClipperLib::pftNonZero;
		} break;
	}
}

END_TOVE_NAMESPACE
