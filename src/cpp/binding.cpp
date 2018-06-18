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
	deref(color)->getRGBA(&rgba, opacity);
	return rgba;
}

TovePaintRef NewLinearGradient(float x1, float y1, float x2, float y2) {
	return paints.publish(std::make_shared<LinearGradient>(x1, y1, x2, y2));
}

TovePaintRef NewRadialGradient(float cx, float cy, float fx, float fy, float r) {
	return paints.publish(std::make_shared<RadialGradient>(cx, cy, fx, fy, r));
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

		if (quality && !quality->adaptive.valid) {
			FixedMeshifier meshifier(scale, quality, holes, flags);
			updated = meshifier(deref(path), deref(fillMesh), deref(lineMesh));
		} else {
			deref(fillMesh)->clear();
			deref(lineMesh)->clear();

			AdaptiveMeshifier meshifier(scale, quality);
			updated = meshifier(deref(path), deref(fillMesh), deref(lineMesh));
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

void ReleasePath(TovePathRef path) {
	paths.release(path);
}


ToveSubpathRef NewSubpath() {
	return trajectories.make();
}

ToveSubpathRef CloneSubpath(ToveSubpathRef trajectory) {
	return trajectories.make(deref(trajectory));
}

void ReleaseSubpath(ToveSubpathRef trajectory) {
	trajectories.release(trajectory);
}

int SubpathGetNumCurves(ToveSubpathRef trajectory) {
	return deref(trajectory)->getNumCurves();
}

int SubpathGetNumPoints(ToveSubpathRef trajectory) {
	return deref(trajectory)->getLoveNumPoints();
}

bool SubpathIsClosed(ToveSubpathRef trajectory) {
	return deref(trajectory)->isClosed();
}

const float *SubpathGetPoints(ToveSubpathRef trajectory) {
	return deref(trajectory)->getPoints();
}

/*void SubpathSetPoint(ToveSubpathRef trajectory, int index, float x, float y) {
	deref(trajectory)->setPoint(index, x, y);
}*/

void SubpathSetPoints(ToveSubpathRef trajectory, const float *pts, int npts) {
	deref(trajectory)->setLovePoints(pts, npts);
}

float SubpathGetCurveValue(ToveSubpathRef trajectory, int curve, int index) {
	return deref(trajectory)->getCurveValue(curve, index);
}

void SubpathSetCurveValue(ToveSubpathRef trajectory, int curve, int index, float value) {
	deref(trajectory)->setCurveValue(curve, index, value);
}

float SubpathGetPtValue(ToveSubpathRef trajectory, int index, int dim) {
	return deref(trajectory)->getLovePointValue(index, dim);
}

void SubpathSetPtValue(ToveSubpathRef trajectory, int index, int dim, float value) {
	deref(trajectory)->setLovePointValue(index, dim, value);
}

void SubpathInvert(ToveSubpathRef trajectory) {
	deref(trajectory)->invert();
}

void SubpathClean(ToveSubpathRef trajectory, float eps) {
	deref(trajectory)->clean(eps);
}

void SubpathSetOrientation(ToveSubpathRef trajectory, ToveOrientation orientation) {
	deref(trajectory)->setOrientation(orientation);
}

int SubpathMoveTo(ToveSubpathRef trajectory, float x, float y) {
	return deref(trajectory)->moveTo(x, y);
}

int SubpathLineTo(ToveSubpathRef trajectory, float x, float y) {
	return deref(trajectory)->lineTo(x, y);
}

int SubpathCurveTo(ToveSubpathRef trajectory, float cpx1, float cpy1, float cpx2, float cpy2, float x, float y) {
	return deref(trajectory)->curveTo(cpx1, cpy1, cpx2, cpy2, x, y);
}

int SubpathArc(ToveSubpathRef trajectory, float x, float y, float r, float startAngle, float endAngle, bool counterclockwise) {
	return deref(trajectory)->arc(x, y, r, startAngle, endAngle, counterclockwise);
}

int SubpathDrawRect(ToveSubpathRef trajectory, float x, float y, float w, float h, float rx, float ry) {
	return deref(trajectory)->drawRect(x, y, w, h, rx, ry);
}

int SubpathDrawEllipse(ToveSubpathRef trajectory, float x, float y, float rx, float ry) {
	return deref(trajectory)->drawEllipse(x, y, rx, ry);
}

float SubpathGetCommandValue(ToveSubpathRef trajectory, int command, int property) {
	return deref(trajectory)->getCommandValue(command, property);
}

void SubpathSetCommandValue(ToveSubpathRef trajectory,
	int command, int property, float value) {
	deref(trajectory)->setCommandValue(command, property, value);
}

ToveVec2 SubpathGetPosition(ToveSubpathRef trajectory, float t) {
	return deref(trajectory)->getPosition(t);
}

ToveVec2 SubpathGetNormal(ToveSubpathRef trajectory, float t) {
	return deref(trajectory)->getNormal(t);
}

ToveNearest SubpathNearest(ToveSubpathRef trajectory,
	float x, float y, float dmin, float dmax) {
	return deref(trajectory)->nearest(x, y, dmin, dmax);
}

int SubpathInsertCurveAt(ToveSubpathRef trajectory, float t) {
	return deref(trajectory)->insertCurveAt(t);
}

void SubpathRemoveCurve(ToveSubpathRef trajectory, int curve) {
	deref(trajectory)->removeCurve(curve);
}

int SubpathMould(ToveSubpathRef trajectory, float t, float x, float y) {
	return deref(trajectory)->mould(t, x, y);
}

bool SubpathIsEdgeAt(ToveSubpathRef trajectory, int k) {
	return deref(trajectory)->isEdgeAt(k - 1);
}

void SubpathMove(ToveSubpathRef trajectory, int k, float x, float y) {
	return deref(trajectory)->move(k - 1, x, y);
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

void GraphicsSet(ToveGraphicsRef graphics, ToveGraphicsRef source,
	bool scaleLineWidth, float tx, float ty, float r, float sx, float sy,
	float ox, float oy, float kx, float ky) {

	nsvg::Transform transform(tx, ty, r, sx, sy, ox, oy, kx, ky);
	transform.setWantsScaleLineWidth(scaleLineWidth);
	deref(graphics)->set(deref(source), transform);
}

ToveMeshResult GraphicsTesselate(
	ToveGraphicsRef shape,
	ToveMeshRef mesh,
	float scale,
	const ToveTesselationQuality *quality,
	ToveHoles holes,
	ToveMeshUpdateFlags flags) {

	return exception_safe([shape, mesh, scale, quality, holes, flags] () {
		const GraphicsRef graphics = deref(shape);
		const int n = graphics->getNumPaths();

		ToveMeshUpdateFlags updated;
		if (quality && !quality->adaptive.valid) {
			FixedMeshifier meshifier(scale, quality, holes, flags);
			updated = meshifier.graphics(graphics, deref(mesh), deref(mesh));
		} else {
			deref(mesh)->clear();
			AdaptiveMeshifier meshifier(scale, quality);
			updated = meshifier.graphics(graphics, deref(mesh), deref(mesh));
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

void GraphicsAnimate(ToveGraphicsRef shape, ToveGraphicsRef a, ToveGraphicsRef b, float t) {
	deref(shape)->animate(deref(a), deref(b), t);
}

void GraphicsSetOrientation(ToveGraphicsRef shape, ToveOrientation orientation) {
	deref(shape)->setOrientation(orientation);
}

void GraphicsClean(ToveGraphicsRef shape, float eps) {
	deref(shape)->clean(eps);
}

TovePathRef GraphicsHit(ToveGraphicsRef graphics, float x, float y) {
	return paths.publishOrNil(deref(graphics)->hit(x, y));
}

void ReleaseGraphics(ToveGraphicsRef shape) {
	shapes.release(shape);
}


ToveShaderLinkRef NewColorShaderLink() {
	return shaderLinks.publish(std::make_shared<ColorShaderLink>());
}

ToveShaderLinkRef NewGeometryShaderLink(TovePathRef path, bool enableFragmentShaderStrokes) {
	return shaderLinks.publish(std::make_shared<GeometryShaderLink>(
		deref(path), enableFragmentShaderStrokes));
}

ToveChangeFlags ShaderLinkBeginUpdate(ToveShaderLinkRef link, TovePathRef path, bool initial) {
	return deref(link)->beginUpdate(deref(path), initial);
}

ToveChangeFlags ShaderLinkEndUpdate(ToveShaderLinkRef link, TovePathRef path, bool initial) {
	return deref(link)->endUpdate(deref(path), initial);
}

ToveShaderData *ShaderLinkGetData(ToveShaderLinkRef link) {
	return deref(link)->getData();
}

ToveShaderLineFillColorData *ShaderLinkGetColorData(ToveShaderLinkRef link) {
	return deref(link)->getColorData();
}

void ReleaseShaderLink(ToveShaderLinkRef link) {
	shaderLinks.release(link);
}


ToveMeshRef NewMesh() {
	return meshes.publish(std::make_shared<Mesh>());
}

ToveMeshRef NewColorMesh() {
	return meshes.publish(std::make_shared<ColorMesh>());
}

int MeshGetVertexCount(ToveMeshRef mesh) {
	return deref(mesh)->getVertexCount();
}

void MeshCopyPositions(ToveMeshRef mesh, void *buffer, uint32_t size) {
	deref(mesh)->copyPositions(buffer, size);
}

void MeshCopyPositionsAndColors(ToveMeshRef mesh, void *buffer, uint32_t size) {
	deref(mesh)->copyPositionsAndColors(buffer, size);
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
