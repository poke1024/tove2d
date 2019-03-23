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
#include "mesh/meshifier.h"
#include "nsvg.h"
#include <sstream>

BEGIN_TOVE_NAMESPACE

GraphicsRef Graphics::createFromSVG(
	const char *svg, const char *units, float dpi) {

	GraphicsRef graphics;
	if (svg) {
		NSVGimage *image = nsvg::parseSVG(svg, units, dpi);

		graphics = tove_make_shared<Graphics>(image);
		nsvgDelete(image);
	} else {
		graphics = tove_make_shared<Graphics>();
	}
	return graphics;
}

void Graphics::rasterize(
	uint8_t *pixels,
	int width, int height, int stride,
	float tx, float ty, float scale,
	const ToveRasterizeSettings *settings) {

	nsvg::rasterize(getImage(), tx, ty, scale,
		pixels, width, height, stride, settings);
}

static void copyFromNSVG(
    Observer *observer,
    NSVGshape **anchor,
    std::vector<PathRef> &paths,
    const NSVGshape *shapes) {

    const NSVGshape *shape = shapes;
    int index = 0;
	while (shape) {
		PathRef path = tove_make_shared<Path>(shape);
		if (observer) {
			path->addObserver(observer);
		}
        path->setIndex(index++);
		if (paths.empty()) {
			*anchor = &path->nsvg;
		} else {
			paths[paths.size() - 1]->setNext(path);
		}
		paths.push_back(path);
		shape = shape->next;
	}
}

static void copyPaths(
	Observer *observer,
	NSVGshape **anchor,
	std::vector<PathRef> &paths,
	const std::vector<PathRef> &sourcePaths) {

	int index = 0;
	while (index < sourcePaths.size()) {
		PathRef path = tove_make_shared<Path>(sourcePaths[index].get());
		if (observer) {
			path->addObserver(observer);
		}
		path->setIndex(index++);
		if (paths.empty()) {
			*anchor = &path->nsvg;
		} else {
			paths[paths.size() - 1]->setNext(path);
		}
		paths.push_back(path);
	}
}

#ifdef NSVG_CLIP_PATHS
Clip::Clip(TOVEclipPath *clipPath) {
    std::memset(&nsvg, 0, sizeof(nsvg));
    copyFromNSVG(nullptr, &nsvg.shapes, paths, clipPath->shapes);
    nsvg.index = clipPath->index;
}

Clip::Clip(const ClipRef &source, const nsvg::Transform &transform) {
	std::memset(&nsvg, 0, sizeof(nsvg));
	copyPaths(nullptr, &nsvg.shapes, paths, source->paths);
	nsvg.index = source->nsvg.index;
	for (int i = 0; i < paths.size(); i++) {
		paths[i]->set(paths[i], transform);
	}
}

void Clip::compute(const AbstractTesselator &tess) {
    computed = tess.toClipPath(paths);
}


void ClipSet::link() {
	for (int i = 1; i < clips.size(); i++) {
		clips[i - 1]->setNext(clips[i]);
	}
	if (clips.size() > 0) {
		clips[clips.size() - 1]->setNext(nullptr);
	}
}

ClipSet::ClipSet(const ClipSet &source, const nsvg::Transform &transform) {
	const int numClips = source.clips.size();
	clips.resize(numClips);
	for (int i = 0; i < numClips; i++) {
		clips[i] = tove_make_shared<Clip>(source.clips[i], transform);
	}
	link();
}
#endif

void Graphics::setNumPaths(int n) {
 	// close all
 	newPath = true;

	if (paths.size() != n) {
		clear();
		for (int i = 0; i < n; i++) {
			_appendPath(tove_make_shared<Path>());
		}
	}
}

void Graphics::_appendPath(const PathRef &path) {
	if (paths.empty()) {
		nsvg.shapes = &path->nsvg;
	} else {
		current()->setNext(path);
	}

    path->setIndex(paths.size());
	path->clearNext();
	paths.push_back(path);

	path->addObserver(this);

	changed(CHANGED_GEOMETRY | CHANGED_COLORS);
}

PathRef Graphics::beginPath() {
	if (!newPath) {
		return current();
	}

	PathRef path = tove_make_shared<Path>();
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

	strokeColor = tove_make_shared<Color>(0.25f, 0.25f, 0.25f);
	fillColor = tove_make_shared<Color>(0.95f, 0.95f, 0.95f);

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

Graphics::Graphics(const ClipSetRef &clipSet) : changes(CHANGED_BOUNDS | CHANGED_EXACT_BOUNDS) {
	initialize(1.0, 1.0);
	this->clipSet = clipSet;
}

Graphics::Graphics(const NSVGimage *image) :
    changes(CHANGED_BOUNDS | CHANGED_EXACT_BOUNDS) {

    initialize(image->width, image->height);
    copyFromNSVG(this, &nsvg.shapes, paths, image->shapes);

#ifdef NSVG_CLIP_PATHS
    std::vector<ClipRef> clips;
    TOVEclipPath *clipPath = image->clipPaths;
    while (clipPath) {
        clips.push_back(tove_make_shared<Clip>(clipPath));
        clipPath = clipPath->next;
    }
    clipSet = tove_make_shared<ClipSet>(clips);
	nsvg.clipPaths = clipSet->getHead();
#endif
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

	clipSet = graphics->clipSet;

	for (const auto &path : graphics->paths) {
		addPath(path);
	}
}

void Graphics::clear() {
	if (paths.size() > 0) {
		for (const auto &p : paths) {
			p->removeObserver(this);
		}
		paths.clear();
		nsvg.shapes = nullptr;
		changed(CHANGED_GEOMETRY);
	}
}

SubpathRef Graphics::beginSubpath() {
	return beginPath()->beginSubpath();
}

void Graphics::closeSubpath(bool closeCurves) {
	if (!paths.empty()) {
		current()->closeSubpath(closeCurves);
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

ToveLineJoin Graphics::getLineJoin() const {
	return nsvg::toveLineJoin(
		static_cast<NSVGlineJoin>(strokeLineJoin));
}

void Graphics::setLineJoin(ToveLineJoin join) {
	strokeLineJoin = nsvg::nsvgLineJoin(join);
}

bool Graphics::areColorsSolid() const {
    for (const auto &path : paths) {
        if (!path->areColorsSolid()) {
            return false;
        }
    }
    return true;
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
	closePath();

	path->closeSubpath();
	_appendPath(path);
	newPath = true;
}

PathRef Graphics::getPathByName(const char *name) const {
    for (const PathRef &p : paths) {
        if (strcmp(p->getName(), name) == 0) {
            return p;
        }
    }
    return PathRef();
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
	const int numPaths = source->paths.size();
	setNumPaths(numPaths);
	for (int i = 0; i < numPaths; i++) {
		paths[i]->set(source->paths[i], transform);
	}

#ifdef NSVG_CLIP_PATHS
	if (source->clipSet) {
		clipSet = tove_make_shared<ClipSet>(*source->clipSet.get(), transform);
	} else {
		clipSet = ClipSetRef();
	}
	nsvg.clipPaths = clipSet ? clipSet->getHead() : nullptr;
#endif
}

ToveChangeFlags Graphics::fetchChanges(ToveChangeFlags flags) {
    flags &= ~(CHANGED_BOUNDS | CHANGED_EXACT_BOUNDS);
	const ToveChangeFlags c = changes & flags;
	changes &= ~flags;
	return c;
}

void Graphics::animate(const GraphicsRef &a, const GraphicsRef &b, float t) {
	const int n = a->paths.size();
	if (n != b->paths.size()) {
		if (tove::report::warnings()) {
			std::ostringstream message;
			message << "cannot animate over path counts " <<
				a->paths.size() << " <-> " <<
				b->paths.size() << ".";
			tove::report::warn(message.str().c_str());
		}
		return;
	}
	if (paths.size() != n) {
		clear();
		while (paths.size() < n) {
			addPath(tove_make_shared<Path>());
		}
	}
	for (int i = 0; i < n; i++) {
		paths[i]->animate(a->paths[i], b->paths[i], t);
	}
}

void Graphics::computeClipPaths(const AbstractTesselator &tess) const {
#ifdef NSVG_CLIP_PATHS
	if (clipSet) {
		for (const ClipRef &clip : clipSet->getClips()) {
			clip->compute(tess);
		}
	}
#endif
}

void Graphics::clearChanges(ToveChangeFlags flags) {
	flags &= ~(CHANGED_BOUNDS | CHANGED_EXACT_BOUNDS);
	changes &= ~flags;
}

END_TOVE_NAMESPACE
