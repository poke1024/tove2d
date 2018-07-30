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
#include "references.h"
#include "path.h"
#include "graphics.h"
#include "mesh/mesh.h"
#include "mesh/meshifier.h"
#include "shader/link.h"

template<typename F>
ToveMeshResult exception_safe(const F &f) {
	try {
		return ToveMeshResult{f()};
	} catch (const std::bad_alloc &e) {
		TOVE_WARN("Out of memory.");
		return ToveMeshResult{0};
	} catch (const std::exception &e) {
		TOVE_WARN(e.what());
		return ToveMeshResult{0};
	} catch (...) {
		return ToveMeshResult{0};
	}
}

extern "C" {

void SetWarningFunction(ToveWarningFunction f) {
	tove_warn_func = f;
}


TovePaintType PaintGetType(TovePaintRef paint) {
	return deref(paint)->getType();
}

TovePaintRef ClonePaint(TovePaintRef paint) {
	return paints.publish(deref(paint)->clone());
}

TovePaintRef NewEmptyPaint() {
	return paints.publishEmpty();
}

TovePaintRef NewColor(float r, float g, float b, float a) {
	return paints.publish(std::make_shared<Color>(r, g, b, a));
}

void ColorSet(TovePaintRef color, float r, float g, float b, float a) {
	deref(color)->setRGBA(r, g, b, a);
}

RGBA ColorGet(TovePaintRef color, float opacity) {
	RGBA rgba;
	deref(color)->getRGBA(rgba, opacity);
	return rgba;
}

int PaintGetNumColors(TovePaintRef paint) {
	return deref(paint)->getNumColorStops();
}

ToveColorStop PaintGetColorStop(TovePaintRef paint, int i, float opacity) {
	ToveColorStop stop;
	stop.offset = deref(paint)->getColorStop(i, stop.rgba, opacity);
	return stop;
}

TovePaintRef NewLinearGradient(float x1, float y1, float x2, float y2) {
	return paints.publish(std::make_shared<LinearGradient>(x1, y1, x2, y2));
}

TovePaintRef NewRadialGradient(float cx, float cy, float fx, float fy, float r) {
	return paints.publish(std::make_shared<RadialGradient>(cx, cy, fx, fy, r));
}

ToveGradientParameters GradientGetParameters(TovePaintRef gradient) {
	ToveGradientParameters parameters;
	deref(gradient)->getGradientParameters(parameters);
	return parameters;
}

void GradientAddColorStop(TovePaintRef gradient, float offset, float r, float g, float b, float a) {
	deref(gradient)->addColorStop(offset, r, g, b, a);
}

void GradientSetColorStop(TovePaintRef gradient, int i, float offset, float r, float g, float b, float a) {
	deref(gradient)->setColorStop(i, offset, r, g, b, a);
}

void ReleasePaint(TovePaintRef paint) {
	paints.release(paint);
}


TovePathRef NewPath(const char *d) {
	if (d) {
		return paths.make(d);
	} else {
		return paths.make();
	}
}

TovePathRef ClonePath(TovePathRef path) {
	return paths.make(deref(path).get());
}

ToveSubpathRef PathBeginSubpath(TovePathRef path) {
	return trajectories.publish(deref(path)->beginSubpath());
}

void PathAddSubpath(TovePathRef path, ToveSubpathRef traj) {
	deref(path)->addSubpath(deref(traj));
}

void PathSetFillColor(TovePathRef path, TovePaintRef color) {
	deref(path)->setFillColor(deref(color));
}

void PathSetLineColor(TovePathRef path, TovePaintRef color) {
	deref(path)->setLineColor(deref(color));
}

TovePaintRef PathGetFillColor(TovePathRef path) {
	return paints.publishOrNil(deref(path)->getFillColor());
}

TovePaintRef PathGetLineColor(TovePathRef path) {
	return paints.publishOrNil(deref(path)->getLineColor());
}

void PathSetLineDash(TovePathRef path, const float *dashes, int count) {
	deref(path)->setLineDash(dashes, count);
}

void PathSetLineDashOffset(TovePathRef path, float offset) {
	deref(path)->setLineDashOffset(offset);
}

float PathGetLineWidth(TovePathRef path) {
	return deref(path)->getLineWidth();
}

void PathSetLineWidth(TovePathRef path, float width) {
	deref(path)->setLineWidth(width);
}

void PathSetMiterLimit(TovePathRef path, float limit) {
	deref(path)->setMiterLimit(limit);
}

float PathGetMiterLimit(TovePathRef path) {
	return deref(path)->getMiterLimit();
}

int PathGetNumSubpaths(TovePathRef path) {
	return deref(path)->getNumSubpaths();
}

ToveSubpathRef PathGetSubpath(TovePathRef path, int i) {
	if (i >= 1 && i <= deref(path)->getNumSubpaths()) {
		return trajectories.publish(deref(path)->getSubpath(i - 1));
	} else {
		return trajectories.publishEmpty();
	}
}

void PathAnimate(TovePathRef path, TovePathRef a, TovePathRef b, float t) {
	deref(path)->animate(deref(a), deref(b), t);
}

int PathGetNumCurves(TovePathRef path) {
	return deref(path)->getNumCurves();
}

ToveFillRule PathGetFillRule(TovePathRef path) {
	return deref(path)->getFillRule();
}

void PathSetFillRule(TovePathRef path, ToveFillRule rule) {
	deref(path)->setFillRule(rule);
}

float PathGetOpacity(TovePathRef path) {
	return deref(path)->getOpacity();
}

void PathSetOpacity(TovePathRef path, float opacity) {
	deref(path)->setOpacity(opacity);
}

void PathClearChanges(TovePathRef path) {
	deref(path)->clearChanges(-1);
}

ToveMeshResult PathTesselate(
	TovePathRef path,
	ToveMeshRef fillMesh,
	ToveMeshRef lineMesh,
	float scale,
	const ToveTesselationQuality *quality,
	ToveHoles holes,
	ToveMeshUpdateFlags flags) {

	return exception_safe([path, fillMesh, lineMesh, scale, quality, holes, flags] () {
		ToveMeshUpdateFlags updated;
		int fillIndex = 0;
		int lineIndex = 0;

		const PathRef p = deref(path);
		Graphics *g = dynamic_cast<Graphics*>(p->getClaimer());
		assert(g);

		if (quality && !quality->adaptive.valid) {
			FixedMeshifier meshifier(scale, quality, holes, flags);
			updated = meshifier.pathToMesh(
				g,
				p,
				deref(fillMesh), deref(lineMesh),
				fillIndex, lineIndex);
		} else {
			AdaptiveMeshifier meshifier(scale, quality);
			updated = meshifier.pathToMesh(
				g,
				p,
				deref(fillMesh), deref(lineMesh),
				fillIndex, lineIndex);
		}
		return updated;
	});
}

ToveChangeFlags PathFetchChanges(TovePathRef path, ToveChangeFlags flags) {
	const ToveChangeFlags changes = deref(path)->fetchChanges(flags);
	deref(path)->clearChanges(flags);
	return changes;
}

void PathSetOrientation(TovePathRef path, ToveOrientation orientation) {
	deref(path)->setOrientation(orientation);
}

void PathClean(TovePathRef path, float eps) {
	deref(path)->clean(eps);
}

bool PathIsInside(TovePathRef path, float x, float y) {
	return deref(path)->isInside(x, y);
}

void PathSet(
	TovePathRef path,
	TovePathRef source,
	bool scaleLineWidth,
	float a, float b, float c,
	float d, float e, float f) {

	nsvg::Transform transform(a, b, c, d, e, f);
	transform.setWantsScaleLineWidth(scaleLineWidth);
	deref(path)->set(deref(source), transform);
}

ToveLineJoin PathGetLineJoin(TovePathRef path) {
	return deref(path)->getLineJoin();
}

void PathSetLineJoin(TovePathRef path, ToveLineJoin join) {
	deref(path)->setLineJoin(join);
}

const char *PathGetId(TovePathRef path) {
	return deref(path)->nsvg.id;
}

void ReleasePath(TovePathRef path) {
	paths.release(path);
}


ToveSubpathRef NewSubpath() {
	return trajectories.make();
}

ToveSubpathRef CloneSubpath(ToveSubpathRef subpath) {
	return trajectories.make(deref(subpath));
}

EXPORT void SubpathCommit(ToveSubpathRef subpath) {
	deref(subpath)->commit();
}

void ReleaseSubpath(ToveSubpathRef subpath) {
	trajectories.release(subpath);
}

int SubpathGetNumCurves(ToveSubpathRef subpath) {
	return deref(subpath)->getNumCurves();
}

int SubpathGetNumPoints(ToveSubpathRef subpath) {
	return deref(subpath)->getLoveNumPoints();
}

bool SubpathIsClosed(ToveSubpathRef subpath) {
	return deref(subpath)->isClosed();
}

void SubpathSetIsClosed(ToveSubpathRef subpath, bool closed) {
	deref(subpath)->setIsClosed(closed);
}

const float *SubpathGetPointsPtr(ToveSubpathRef subpath) {
	return deref(subpath)->getPoints();
}

/*void SubpathSetPoint(ToveSubpathRef subpath, int index, float x, float y) {
	deref(subpath)->setPoint(index, x, y);
}*/

void SubpathSetPoints(ToveSubpathRef subpath, const float *pts, int npts) {
	deref(subpath)->setLovePoints(pts, npts);
}

float SubpathGetCurveValue(ToveSubpathRef subpath, int curve, int index) {
	return deref(subpath)->getCurveValue(curve, index);
}

void SubpathSetCurveValue(ToveSubpathRef subpath, int curve, int index, float value) {
	deref(subpath)->setCurveValue(curve, index, value);
}

float SubpathGetPtValue(ToveSubpathRef subpath, int index, int dim) {
	return deref(subpath)->getLovePointValue(index, dim);
}

void SubpathSetPtValue(ToveSubpathRef subpath, int index, int dim, float value) {
	deref(subpath)->setLovePointValue(index, dim, value);
}

void SubpathInvert(ToveSubpathRef subpath) {
	deref(subpath)->invert();
}

void SubpathClean(ToveSubpathRef subpath, float eps) {
	deref(subpath)->clean(eps);
}

void SubpathSetOrientation(ToveSubpathRef subpath, ToveOrientation orientation) {
	deref(subpath)->setOrientation(orientation);
}

int SubpathMoveTo(ToveSubpathRef subpath, float x, float y) {
	return deref(subpath)->moveTo(x, y);
}

int SubpathLineTo(ToveSubpathRef subpath, float x, float y) {
	return deref(subpath)->lineTo(x, y);
}

int SubpathCurveTo(ToveSubpathRef subpath, float cpx1, float cpy1, float cpx2, float cpy2, float x, float y) {
	return deref(subpath)->curveTo(cpx1, cpy1, cpx2, cpy2, x, y);
}

int SubpathArc(ToveSubpathRef subpath, float x, float y, float r, float startAngle, float endAngle, bool counterclockwise) {
	return deref(subpath)->arc(x, y, r, startAngle, endAngle, counterclockwise);
}

int SubpathDrawRect(ToveSubpathRef subpath, float x, float y, float w, float h, float rx, float ry) {
	return deref(subpath)->drawRect(x, y, w, h, rx, ry);
}

int SubpathDrawEllipse(ToveSubpathRef subpath, float x, float y, float rx, float ry) {
	return deref(subpath)->drawEllipse(x, y, rx, ry);
}

float SubpathGetCommandValue(ToveSubpathRef subpath, int command, int property) {
	return deref(subpath)->getCommandValue(command, property);
}

void SubpathSetCommandValue(ToveSubpathRef subpath,
	int command, int property, float value) {
	deref(subpath)->setCommandValue(command, property, value);
}

ToveVec2 SubpathGetPosition(ToveSubpathRef subpath, float t) {
	return deref(subpath)->getPosition(t);
}

ToveVec2 SubpathGetNormal(ToveSubpathRef subpath, float t) {
	return deref(subpath)->getNormal(t);
}

ToveNearest SubpathNearest(ToveSubpathRef subpath,
	float x, float y, float dmin, float dmax) {
	return deref(subpath)->nearest(x, y, dmin, dmax);
}

int SubpathInsertCurveAt(ToveSubpathRef subpath, float t) {
	return deref(subpath)->insertCurveAt(t);
}

void SubpathRemoveCurve(ToveSubpathRef subpath, int curve) {
	deref(subpath)->removeCurve(curve);
}

int SubpathMould(ToveSubpathRef subpath, float t, float x, float y) {
	return deref(subpath)->mould(t, x, y);
}

bool SubpathIsLineAt(ToveSubpathRef subpath, int k, int dir) {
	return deref(subpath)->isLineAt(k - 1, dir);
}

void SubpathMakeFlat(ToveSubpathRef subpath, int k, int dir) {
	deref(subpath)->makeFlat(k - 1, dir);
}

void SubpathMakeSmooth(ToveSubpathRef subpath, int k, int dir, float a) {
	deref(subpath)->makeSmooth(k - 1, dir, a);
}

void SubpathMove(ToveSubpathRef subpath, int k, float x, float y) {
	return deref(subpath)->move(k - 1, x, y);
}

void SubpathSet(ToveSubpathRef subpath, ToveSubpathRef source,
	float a, float b, float c, float d, float e, float f) {

	nsvg::Transform transform(a, b, c, d, e, f);
	deref(subpath)->set(deref(source), transform);
}


ToveGraphicsRef NewGraphics(const char *svg, const char* units, float dpi) {
	GraphicsRef shape;

	if (svg) {
		char *mutableSVG = strdup(svg);
		NSVGimage *image = nsvgParse(mutableSVG, units, dpi);
		free(mutableSVG);

		shape = std::make_shared<Graphics>(image);
		nsvgDelete(image);
	} else {
		shape = std::make_shared<Graphics>();
	}

	return shapes.publish(shape);
}

ToveGraphicsRef CloneGraphics(ToveGraphicsRef graphics) {
	return shapes.publish(std::make_shared<Graphics>(deref(graphics)));
}

ToveSubpathRef GraphicsBeginSubpath(ToveGraphicsRef graphics) {
	return trajectories.publish(deref(graphics)->beginSubpath());
}

void GraphicsCloseSubpath(ToveGraphicsRef graphics) {
	deref(graphics)->closeSubpath();
}

void GraphicsInvertSubpath(ToveGraphicsRef graphics) {
	deref(graphics)->invertSubpath();
}

TovePathRef GraphicsGetCurrentPath(ToveGraphicsRef shape) {
	return paths.publish(deref(shape)->getCurrentPath());
}

void GraphicsAddPath(ToveGraphicsRef shape, TovePathRef path) {
	deref(shape)->addPath(deref(path));
}

int GraphicsGetNumPaths(ToveGraphicsRef shape) {
	return deref(shape)->getNumPaths();
}

TovePathRef GraphicsGetPath(ToveGraphicsRef shape, int i) {
	if (i >= 1 && i <= deref(shape)->getNumPaths()) {
		return paths.publish(deref(shape)->getPath(i - 1));
	} else {
		return paths.publishEmpty();
	}
}

EXPORT TovePathRef GraphicsGetPathByName(ToveGraphicsRef shape, const char *name) {
	const PathRef p = deref(shape)->getPathByName(name);
	return p ? paths.publish(p) : paths.publishEmpty();
}

ToveChangeFlags GraphicsFetchChanges(ToveGraphicsRef shape, ToveChangeFlags flags) {
	return deref(shape)->fetchChanges(flags, true);
}

void GraphicsSetFillColor(ToveGraphicsRef shape, TovePaintRef color) {
	deref(shape)->setFillColor(deref(color));
}

void GraphicsSetLineWidth(ToveGraphicsRef shape, float width) {
	deref(shape)->setLineWidth(width);
}

void GraphicsSetMiterLimit(ToveGraphicsRef shape, float limit) {
	deref(shape)->setMiterLimit(limit);
}

void GraphicsSetLineDash(ToveGraphicsRef shape, const float *dashes, int count) {
	deref(shape)->setLineDash(dashes, count);
}

void GraphicsSetLineDashOffset(ToveGraphicsRef shape, float offset) {
	deref(shape)->setLineDashOffset(offset);
}

void GraphicsSetLineColor(ToveGraphicsRef shape, TovePaintRef color) {
	deref(shape)->setLineColor(deref(color));
}

void GraphicsFill(ToveGraphicsRef shape) {
	deref(shape)->fill();
}

void GraphicsStroke(ToveGraphicsRef shape) {
	deref(shape)->stroke();
}

ToveBounds GraphicsGetBounds(ToveGraphicsRef shape, bool exact) {
	const float *computed;

	if (exact) {
		computed = deref(shape)->getExactBounds();
	} else {
		computed  = deref(shape)->getBounds();
	}

	ToveBounds bounds;
	bounds.x0 = computed[0];
	bounds.y0 = computed[1];
	bounds.x1 = computed[2];
	bounds.y1 = computed[3];
	return bounds;
}

void GraphicsSet(
	ToveGraphicsRef graphics,
	ToveGraphicsRef source,
	bool scaleLineWidth,
	float a, float b, float c,
	float d, float e, float f) {

	nsvg::Transform transform(a, b, c, d, e, f);
	transform.setWantsScaleLineWidth(scaleLineWidth);
	deref(graphics)->set(deref(source), transform);
}

ToveMeshResult GraphicsTesselate(
	ToveGraphicsRef graphicsRef,
	ToveMeshRef mesh,
	float scale,
	const ToveTesselationQuality *quality,
	ToveHoles holes,
	ToveMeshUpdateFlags flags) {

	return exception_safe([graphicsRef, mesh, scale, quality, holes, flags] () {
		const GraphicsRef graphics = deref(graphicsRef);
		const int n = graphics->getNumPaths();

		ToveMeshUpdateFlags updated;
		if (quality && !quality->adaptive.valid) {
			FixedMeshifier meshifier(scale, quality, holes, flags);
			updated = meshifier.graphicsToMesh(
				graphics, deref(mesh), deref(mesh));
		} else {
			AdaptiveMeshifier meshifier(scale, quality);

			// note: only this case supports clip paths.
			graphics->computeClipPaths(meshifier);

			updated = meshifier.graphicsToMesh(
				graphics, deref(mesh), deref(mesh));
		}
		return updated;
	});
}

ToveImageRecord GraphicsRasterize(ToveGraphicsRef shape, int width, int height,
	float tx, float ty, float scale, const ToveTesselationQuality *quality) {
	ToveImageRecord image;

	NSVGimage *nsvg = deref(shape)->getImage();
	image.pixels = nsvg::rasterize(nsvg, tx, ty, scale, width, height, quality);

	return image;
}

void GraphicsAnimate(
	ToveGraphicsRef graphics,
	ToveGraphicsRef a,
	ToveGraphicsRef b,
	float t) {
	deref(graphics)->animate(deref(a), deref(b), t);
}

void GraphicsSetOrientation(
	ToveGraphicsRef graphics,
	ToveOrientation orientation) {
	deref(graphics)->setOrientation(orientation);
}

void GraphicsClean(ToveGraphicsRef graphics, float eps) {
	deref(graphics)->clean(eps);
}

TovePathRef GraphicsHit(ToveGraphicsRef graphics, float x, float y) {
	return paths.publishOrNil(deref(graphics)->hit(x, y));
}

void GraphicsClear(ToveGraphicsRef graphics) {
	deref(graphics)->clear();
}

bool GraphicsAreColorsSolid(ToveGraphicsRef graphics) {
	return deref(graphics)->areColorsSolid();
}

void GraphicsClearChanges(ToveGraphicsRef graphics) {
	deref(graphics)->clearChanges(-1);
}

ToveLineJoin GraphicsGetLineJoin(ToveGraphicsRef graphics) {
	return deref(graphics)->getLineJoin();
}

void GraphicsSetLineJoin(ToveGraphicsRef graphics, ToveLineJoin join) {
	deref(graphics)->setLineJoin(join);
}

void ReleaseGraphics(ToveGraphicsRef graphics) {
	shapes.release(graphics);
}


ToveFeedRef NewColorFeed(ToveGraphicsRef graphics, float scale) {
	return shaderLinks.publish(std::make_shared<ColorFeed>(deref(graphics), scale));
}

ToveFeedRef NewGeometryFeed(TovePathRef path, bool enableFragmentShaderStrokes) {
	return shaderLinks.publish(std::make_shared<GeometryFeed>(
		deref(path), enableFragmentShaderStrokes));
}

ToveChangeFlags FeedBeginUpdate(ToveFeedRef link) {
	return deref(link)->beginUpdate();
}

ToveChangeFlags FeedEndUpdate(ToveFeedRef link) {
	return deref(link)->endUpdate();
}

ToveShaderData *FeedGetData(ToveFeedRef link) {
	return deref(link)->getData();
}

TovePaintColorAllocation FeedGetColorAllocation(ToveFeedRef link) {
	return deref(link)->getColorAllocation();
}

void FeedBind(ToveFeedRef link, const ToveGradientData *data) {
	deref(link)->bind(*data);
}

void ReleaseFeed(ToveFeedRef link) {
	shaderLinks.release(link);
}


ToveMeshRef NewMesh() {
	return meshes.publish(std::make_shared<Mesh>());
}

ToveMeshRef NewColorMesh() {
	return meshes.publish(std::make_shared<ColorMesh>());
}

ToveMeshRef NewPaintMesh() {
	return meshes.publish(std::make_shared<PaintMesh>());
}

int MeshGetVertexCount(ToveMeshRef mesh) {
	return deref(mesh)->getVertexCount();
}

void MeshCopyVertexData(ToveMeshRef mesh, void *buffer, uint32_t size) {
	deref(mesh)->copyVertexData(buffer, size);
}

ToveTriangles MeshGetTriangles(ToveMeshRef mesh) {
	return deref(mesh)->getTriangles();
}

void MeshCache(ToveMeshRef mesh, bool keyframe) {
	deref(mesh)->cache(keyframe);
}

void ReleaseMesh(ToveMeshRef mesh) {
	meshes.release(mesh);
}

void DeleteImage(ToveImageRecord image) {
	free(image.pixels);
}

} // extern "C"
