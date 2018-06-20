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

 // on the naming of things:

 // what NSVG calls a "shape" is called a "path" in TÖVE; what NSVG calls a
 // "path" is a "sub path" in TÖVE. see this explanation by Mark Kilgard:
 // (https://www.opengl.org/discussion_boards/showthread.php/175260-GPU-accelerated-path-rendering?p=1225200&viewfull=1#post1225200)

 // which leads us to this renaming:
 // 		NSVG path -> Subpath
 // 		NSVG shape -> Path
 // 		NSVG image -> Graphics

#include "graphics.h"

void Graphics::setNumPaths(int n) {
 	// close all
 	newPath = true;

	if (paths.size() != n) {
		clear();
		for (int i = 0; i < n; i++) {
			_appendPath(std::make_shared<Path>());
		}
	}
}

void Graphics::_appendPath(const PathRef &path) {
	if (paths.empty()) {
		nsvg.shapes = &path->nsvg;
	} else {
		current()->setNext(path);
	}
	path->clearNext();
	path->claim(this);
	paths.push_back(path);
    changed(CHANGED_GEOMETRY);
}

PathRef Graphics::beginPath() {
	if (!newPath) {
		return current();
	}

	PathRef path = std::make_shared<Path>(this);
	_appendPath(path);

	path->beginSubpath();
	newPath = false;

    return path;
}

void Graphics::closePath(bool closeCurves) {
	if (!paths.empty() && !newPath) {
		const PathRef &path = current();

		path->nsvg.strokeWidth = strokeWidth;
		path->nsvg.strokeDashOffset = strokeDashOffset;
		path->nsvg.strokeDashCount = std::min(
			size_t(nsvg::maxDashes()), strokeDashes.size());
		for (int i = 0; i < path->nsvg.strokeDashCount; i++) {
			path->nsvg.strokeDashArray[i] = strokeDashes[i];
		}
		path->nsvg.strokeLineJoin = strokeLineJoin;
		path->nsvg.strokeLineCap = strokeLineCap;
		path->nsvg.miterLimit = miterLimit;
		path->nsvg.fillRule = fillRule;
		path->nsvg.flags = NSVG_FLAGS_VISIBLE;

		path->closeSubpath(closeCurves);
		changed(CHANGED_GEOMETRY);

		newPath = true;
	} else if (closeCurves) {
		current()->closeSubpath(true);
	}
}

void Graphics::initialize(float width, float height) {
	memset(&nsvg, 0, sizeof(nsvg));
	nsvg.width = width;
	nsvg.height = height;

	strokeColor = std::make_shared<Color>(0.25, 0.25, 0.25);
	fillColor = std::make_shared<Color>(0.95, 0.95, 0.95);

	strokeWidth = 3.0;
	strokeDashOffset = 0.0;
	strokeLineJoin = NSVG_JOIN_MITER;
	strokeLineCap = NSVG_CAP_BUTT;
	miterLimit = 4;
	fillRule = NSVG_FILLRULE_NONZERO;

	newPath = true;

	for (int i = 0; i < 4; i++) {
		bounds[i] = 0.0;
        exactBounds[i] = 0.0;
	}
    changes |= CHANGED_BOUNDS | CHANGED_EXACT_BOUNDS;
}

Graphics::Graphics() : changes(CHANGED_BOUNDS | CHANGED_EXACT_BOUNDS) {
	initialize(1.0, 1.0);
}

Graphics::Graphics(const NSVGimage *image) :
    changes(CHANGED_BOUNDS | CHANGED_EXACT_BOUNDS) {

    initialize(image->width, image->height);

	NSVGshape *shape = image->shapes;
	while (shape) {
		PathRef path = std::make_shared<Path>(this, shape);
		if (paths.empty()) {
			nsvg.shapes = &path->nsvg;
		} else {
			paths[paths.size() - 1]->setNext(path);
		}
		path->claim(this);
		paths.push_back(path);
		shape = shape->next;
	}
}

Graphics::Graphics(const GraphicsRef &graphics) : changes(graphics->changes) {
	initialize(graphics->nsvg.width, graphics->nsvg.height);

	strokeColor = graphics->strokeColor;
	fillColor = graphics->fillColor;

	strokeWidth = graphics->strokeWidth;
	strokeDashOffset = graphics->strokeDashOffset;
	strokeLineJoin = graphics->strokeLineJoin;
	strokeLineCap = graphics->strokeLineCap;
	miterLimit = graphics->miterLimit;
	fillRule = graphics->fillRule;

	for (const auto &path : graphics->paths) {
		addPath(path->clone());
	}
}

void Graphics::clear() {
	if (paths.size() > 0) {
		for (const auto &p : paths) {
			p->unclaim(this);
		}
		paths.clear();
		nsvg.shapes = nullptr;
		changed(CHANGED_GEOMETRY);
	}
}

SubpathRef Graphics::beginSubpath() {
	return beginPath()->beginSubpath();
}

void Graphics::closeSubpath() {
	if (!paths.empty()) {
		current()->closeSubpath();
	}
}

void Graphics::invertSubpath() {
    if (!paths.empty()) {
		current()->invertSubpath();
	}
}

void Graphics::setLineDash(const float *dashes, int count) {
	float sum = 0;
	for (int i = 0; i < count; i++) {
		sum += dashes[i];
	}
	if (sum <= 1e-6f) {
		count = 0;
	}

	strokeDashes.resize(count);
	for (int i = 0; i < count; i++) {
		strokeDashes[i] = dashes[i];
	}
}

void Graphics::fill() {
	if (!paths.empty()) {
		current()->setFillColor(fillColor);
		closePath(true);
		changed(CHANGED_GEOMETRY);
	}
}

void Graphics::stroke() {
	if (!paths.empty()) {
		current()->setLineColor(strokeColor);
		current()->setLineWidth(strokeWidth);
		current()->setLineDash(&strokeDashes[0], strokeDashes.size());
		current()->setLineDashOffset(strokeDashOffset);
		closePath();
		changed(CHANGED_GEOMETRY);
	}
}

void Graphics::addPath(const PathRef &path) {
	if (path->isClaimed()) {
		addPath(path->clone());
	} else {
		closePath();

		path->closeSubpath();
		_appendPath(path);

		newPath = true;
		changed(CHANGED_GEOMETRY);
	}
}

NSVGimage *Graphics::getImage() {
	closePath();

	for (int i = 0; i < paths.size(); i++) {
		paths[i]->updateNSVG();
	}

	return &nsvg;
}

const float *Graphics::getBounds() {
	closePath();

	if (changes & CHANGED_BOUNDS) {
        computeBounds(bounds, [] (const PathRef &path) {
            return path->getBounds();
        });
		changes &= ~CHANGED_BOUNDS;
	}

	return bounds;
}

const float *Graphics::getExactBounds() {
    closePath();

    if (changes & CHANGED_EXACT_BOUNDS) {
        computeBounds(exactBounds, [] (const PathRef &path) {
            return path->getExactBounds();
        });
        changes &= ~CHANGED_EXACT_BOUNDS;
    }

    return exactBounds;
}

void Graphics::clean(float eps) {
	for (const auto &p : paths) {
		p->clean(eps);
	}
}

PathRef Graphics::hit(float x, float y) const {
    for (const auto &p : paths) {
		if (p->isInside(x, y)) {
            return p;
        }
	}
    return PathRef();
}

void Graphics::setOrientation(ToveOrientation orientation) {
	for (int i = 0; i < paths.size(); i++) {
		paths[i]->setOrientation(orientation);
	}
}

void Graphics::set(const GraphicsRef &source, const nsvg::Transform &transform) {
	const int n = source->paths.size();
	setNumPaths(n);

	for (int i = 0; i < n; i++) {
		paths[i]->set(source->paths[i], transform);
	}
}

ToveChangeFlags Graphics::fetchChanges(ToveChangeFlags flags, bool clearAll) {
    flags &= ~(CHANGED_BOUNDS | CHANGED_EXACT_BOUNDS);
	const ToveChangeFlags c = changes & flags;
	changes &= ~flags;
	if (clearAll && c != 0) {
		for (int i = 0; i < paths.size(); i++) {
			paths[i]->clearChanges(flags);
		}
	}
	return c;
}

void Graphics::animate(const GraphicsRef &a, const GraphicsRef &b, float t) {
	const int n = a->paths.size();
	if (n != b->paths.size()) {
		return;
	}
	if (paths.size() != n) {
		clear();
		while (paths.size() < n) {
			addPath(std::make_shared<Path>());
		}
	}
	for (int i = 0; i < n; i++) {
		paths[i]->animate(a->paths[i], b->paths[i], t);
	}
}
