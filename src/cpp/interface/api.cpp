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

#include "../common.h"
#include "../references.h"
#include "../path.h"
#include "../graphics.h"
#include "../palette.h"
#include "../mesh/mesh.h"
#include "../mesh/meshifier.h"
#include "../mesh/flatten.h"
#include "../shader/feed/color_feed.h"
#include "../gpux/gpux_feed.h"
#include "../../thirdparty/bluenoise.h"
#include <sstream>
#include <vector>

#if TOVE_TARGET == TOVE_TARGET_LOVE2D

using namespace tove;

template<typename F>
static ToveMeshResult exception_safe(const F &f) {
	try {
		return ToveMeshResult{f()};
	} catch (const std::bad_alloc &e) {
		TOVE_BAD_ALLOC();
		return ToveMeshResult{0};
	} catch (const std::exception &e) {
		tove::report::err(e.what());
		return ToveMeshResult{0};
	} catch (...) {
		return ToveMeshResult{0};
	}
}

static const ToveTesselationSettings *getDefaultQuality() {
	static ToveTesselationSettings defaultQuality;

	defaultQuality.stopCriterion = TOVE_MAX_ERROR;

	defaultQuality.recursionLimit = 8;
	defaultQuality.arcTolerance = 0.0;
	defaultQuality.maximumError = 0.5;

	return &defaultQuality;
}

extern "C" {

void SetReportFunction(ToveReportFunction f) {
	tove::report::config.report = f;
}

void SetReportLevel(ToveReportLevel l) {
	tove::report::config.level = l;
}


TovePaletteRef NoPalette() {
	return TovePaletteRef{nullptr};
}

TovePaletteRef DefaultPalette(const char *name) {
	static TovePaletteRef paletteRefs[2] = {nullptr, nullptr};

	int index;
	if (strcmp(name, "pico8") == 0) {
		index = 0;
	} else if (strcmp(name, "bw") == 0) {
		index = 1;
	} else {
		index = 0;
	}

	if (paletteRefs[index].ptr == nullptr) {
		switch (index) {
			case 0: {
				// the wonderful PICO 8 palette, CC0-license
				// see https://www.lexaloffle.com/pico-8.php?page=faq

				static const uint8_t pico8_colors[] = {
					0, 0, 0, // black
					29, 43, 83, // dark blue
					126, 37, 83, // dark purple
					0, 135, 81, // dark green
					171, 82, 54, // brown
					95, 87, 79, // dark gray
					194, 195, 199, // light gray
					255, 241, 232, // white
					255, 0, 77, // red
					255, 163, 0, // orange
					255, 236, 39, // yellow
					0, 228, 54, // green
					41, 173, 255, // blue
					131, 118, 156, // indigo
					255, 119, 168, // pink
					255, 204, 170 // peach
				};

				PaletteRef palette = std::make_shared<Palette>(pico8_colors, 16);
				paletteRefs[0] = palettes.publish(palette);
			} break;

			case 1: {
				static const uint8_t bw_colors[] = {
					255, 255, 255,
					0, 0, 0
				};

				PaletteRef palette = std::make_shared<Palette>(bw_colors, 2);
				paletteRefs[1] = palettes.publish(palette);
			} break;
		}
	}

	return paletteRefs[index];
}

class Bayer {
	std::vector<float> matrix;
	const int n;

	void compute(const int n2, const int r) {
		const int n = n2 >> 1;

		if (n < 1) {
			matrix[0] = 0;
		} else {
			compute(n, r);

			float *m00 = matrix.data();
			float *m10 = matrix.data() + n * r;
			float *m01 = matrix.data() + n;
			float *m11 = matrix.data() + n + n * r;

			for (int y = 0; y < n; y++) {
				int o = y * r;
				for (int x = 0; x < n; x++, o++) {
					const int z = 4 * m00[o];
					m00[o] = z;
					m10[o] = z + 3;
					m01[o] = z + 2;
					m11[o] = z + 1;
				}
			}
		}
	}


public:
	Bayer(const int n) : n(n) {
		const int scale = n * n;
		matrix.resize(scale);
		compute(n, n);
		// note that our M already is biased by -0.5
		for (int i = 0; i < scale; i++) {
			matrix[i] = matrix[i] / scale - 0.5f;
		}
	}

	inline const float *get() const {
		return matrix.data();
	}
};

bool GenerateBlueNoise(int s, float *m) {
	if ((s & (s - 1)) != 0) { // not a power of 2?
		return false;
	}

	BlueNoise noise(s);
	std::memcpy(m, noise.get(), s * s * sizeof(float));
	return true;
}

bool SetRasterizeSettings(
	ToveRasterizeSettings *settings,
	const char *algorithm,
	TovePaletteRef palette,
	float spread,
	float noise,
	const float *noiseMatrix,
	int noiseMatrixSize) {

	static std::map<std::string, std::function<ToveDither()>> algorithms;
	if (algorithms.empty()) {
		algorithms["fast"] = [] () {
			return ToveDither{TOVE_DITHER_NONE, nullptr, 0, 0};
		};
		algorithms["floyd"] = [] () {
			static const float floyd[] = {
				0.0f,			0.0f,			7.0f / 16.0f,
				3.0f / 16.0f,	5.0f / 16.0f,	1.0f / 16.0f
			};

			return ToveDither{TOVE_DITHER_DIFFUSION, floyd, 3, 2};
		};
		algorithms["atkinson"] = [] () {
			static const float atkinson[] = {
				0.0f,		0.0f,			0.0f,			1.0f / 8.0f,	1.0f / 8.0f,
				0.0f,		1.0f / 8.0f,	1.0f / 8.0f,	1.0f / 8.0f,	0.0f,
				0.0f,		0.0f,			1.0f / 8.0f,	0.0f,			0.0f
			};

			return ToveDither{TOVE_DITHER_DIFFUSION, atkinson, 5, 3};
		};
		algorithms["jarvis"] = [] () {
			static const float jarvis[] = {
				0.0f,			0.0f,			0.0f,			7.0f / 48.0f,	5.0f / 48.0f,
				3.0f / 48.0f,	5.0f / 48.0f,	7.0f / 48.0f, 	5.0f / 48.0f, 	3.0f / 48.0f,
				1.0f / 48.0f,	3.0f / 48.0f,	5.0f / 48.0f, 	3.0f / 48.0f,	1.0f / 48.0f
			};

			return ToveDither{TOVE_DITHER_DIFFUSION, jarvis, 5, 3};
		};
		algorithms["stucki"] = [] () {
			static const float stucki[] = {
				0.0f,			0.0f,			0.0f,			8.0f / 42.0f,	4.0f / 42.0f,
				2.0f / 42.0f,	4.0f / 42.0f,	8.0f / 42.0f, 	4.0f / 42.0f, 	2.0f / 42.0f,
				1.0f / 42.0f,	2.0f / 42.0f,	4.0f / 42.0f, 	2.0f / 42.0f,	1.0f / 42.0f
			};

			return ToveDither{TOVE_DITHER_DIFFUSION, stucki, 5, 3};
		};
	}
	
	*settings = *nsvg::getDefaultRasterizeSettings();

	const auto it = algorithms.find(algorithm);
	if (it != algorithms.end()) {
		settings->quality.dither = it->second();
	} else if (strncmp(algorithm, "bayer", 5) == 0) {
		static std::map<int, Bayer*> cached;
		Bayer *bayer;

		const uint16_t n = std::atoi(&algorithm[5]);
		if ((n & (n - 1)) != 0) { // not a power of 2?
			return false;
		}

		const auto b = cached.find(n);
		if (b == cached.end()) {
			bayer = new Bayer(n);
			cached[n] = bayer;
		} else {
			bayer = b->second;
		}

		settings->quality.dither = ToveDither{
			TOVE_DITHER_ORDERED, bayer->get(), n, n};
	} else {
		return false;
	}

	if (settings->quality.dither.type == TOVE_DITHER_ORDERED) {
		if (palette.ptr) {
			settings->quality.dither.spread = spread * (256.0f / (deref(palette)->size() - 1));
		} else {
			settings->quality.dither.spread = spread;
		}
	} else {
		settings->quality.dither.spread = spread;
	}

	settings->quality.palette = palette;
	settings->quality.noise.amount = noise;
	settings->quality.noise.matrix = noiseMatrix;
	settings->quality.noise.n = noiseMatrixSize;

	return true;
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

TovePaintRef NewShaderPaint(const char* s) {
	return paints.publish(tove_make_shared<PaintShader>(s));
}

const ToveSendArgs *ShaderNextSendArgs(TovePaintRef shader) {
	return std::static_pointer_cast<PaintShader>(deref(shader))->nextSendArgs();
}

TovePaintRef NewColor(float r, float g, float b, float a) {
	return paints.publish(tove_make_shared<Color>(r, g, b, a));
}

void ColorSet(TovePaintRef color, float r, float g, float b, float a) {
	deref(color)->setRGBA(r, g, b, a);
}

ToveRGBA ColorGet(TovePaintRef color, float opacity) {
	ToveRGBA rgba;
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
	return paints.publish(tove_make_shared<LinearGradient>(x1, y1, x2, y2));
}

TovePaintRef NewRadialGradient(float cx, float cy, float fx, float fy, float r) {
	return paints.publish(tove_make_shared<RadialGradient>(cx, cy, fx, fy, r));
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

void PathRemoveSubpath(TovePathRef path, ToveSubpathRef traj) {
	deref(path)->removeSubpath(deref(traj));
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
	deref(path)->animate(deref(a), deref(b), t, 1);
}

void PathRotate(TovePathRef path, ToveElementType what, int k) {
	deref(path)->rotate(what, k);
}

bool PathHasNormalFillStrokeOrder(TovePathRef path) {
	return deref(path)->hasNormalFillStrokeOrder();
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

/*ToveMeshResult PathTesselate(
	ToveGraphicsRef graphics,
	TovePathRef path,
	ToveMeshRef fillMesh,
	ToveMeshRef lineMesh,
	const ToveTesselationSettings *quality0,
	ToveMeshUpdateFlags flags) {

	const ToveTesselationSettings *quality =
		quality0 ? quality0 : getDefaultQuality();

	AbstractMeshifier *meshifier =
		graphics->getMeshifier(path, *quality);
	return meshifier->pathToMesh(
		p,
		deref(fillMesh), deref(lineMesh),
		fillIndex, lineIndex);



	return exception_safe(
		[graphics, path, fillMesh, lineMesh, quality0, flags] () {

		ToveMeshUpdateFlags updated;
		int fillIndex = 0;
		int lineIndex = 0;

		const PathRef p = deref(path);
		const Graphics *g = deref(graphics).get();

		const ToveTesselationSettings *quality =
			quality0 ? quality0 : getDefaultQuality();

		if (quality->stopCriterion == TOVE_REC_DEPTH) {
			FixedMeshifier meshifier(g, *quality, flags);
			updated = meshifier.pathToMesh(
				p,
				deref(fillMesh), deref(lineMesh),
				fillIndex, lineIndex);
		} else {
			AdaptiveMeshifier meshifier(g, *quality);
			updated = meshifier.pathToMesh(
				p,
				deref(fillMesh), deref(lineMesh),
				fillIndex, lineIndex);
		}
		return updated;
	});
}*/

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

void PathRefine(TovePathRef path, int factor) {
	deref(path)->refine(factor);
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
	return deref(subpath)->getNumCurves(false);
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

float *SubpathGetPointsPtr(ToveSubpathRef subpath) {
	return deref(subpath)->getPoints();
}

void SubpathFixLoop(ToveSubpathRef subpath) {
	deref(subpath)->fixLoop();
}

/*void SubpathSetPoint(ToveSubpathRef subpath, int index, float x, float y) {
	deref(subpath)->setPoint(index, x, y);
}*/

void SubpathSetPoints(ToveSubpathRef subpath, const float *pts, int npts) {
	deref(subpath)->setPoints(pts, npts);
}

float SubpathGetCurveValue(ToveSubpathRef subpath, int curve, int index) {
	auto s = deref(subpath);
	if (s) {
		return s->getCurveValue(curve, index);
	} else {
		return 0.0f;
	}
}

void SubpathSetCurveValue(ToveSubpathRef subpath, int curve, int index, float value) {
	auto s = deref(subpath);
	if (s) {
		s->setCurveValue(curve, index, value);
	}
}

float SubpathGetPtValue(ToveSubpathRef subpath, int index, int dim) {
	auto s = deref(subpath);
	if (s) {
		return s->getPointValue(index, dim);
	} else {
		return 0.0f;
	}
}

void SubpathSetPtValue(ToveSubpathRef subpath, int index, int dim, float value) {
	auto s = deref(subpath);
	if (s) {
		s->setPointValue(index, dim, value);
	}
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

void SubpathMove(ToveSubpathRef subpath, int k, float x, float y, ToveHandle handle) {
	return deref(subpath)->move(k - 1, x, y, handle);
}

void SubpathSet(ToveSubpathRef subpath, ToveSubpathRef source,
	float a, float b, float c, float d, float e, float f) {

	nsvg::Transform transform(a, b, c, d, e, f);
	deref(subpath)->set(deref(source), transform);
}

ToveCurvature *SubpathSaveCurvature(ToveSubpathRef subpath) {
	return deref(subpath)->saveCurvature();
}

void SubpathRestoreCurvature(ToveSubpathRef subpath) {
	deref(subpath)->restoreCurvature();
}


ToveGraphicsRef NewGraphics(const char *svg, const char* units, float dpi) {

	return shapes.publish(Graphics::createFromSVG(svg, units, dpi));
}

ToveGraphicsRef CloneGraphics(ToveGraphicsRef graphics, bool deep) {
	return shapes.publish(tove_make_shared<Graphics>(deref(graphics).get(), deep));
}

TovePathRef GraphicsBeginPath(ToveGraphicsRef graphics) {
	return paths.publish(deref(graphics)->beginPath());
}

void GraphicsClosePath(ToveGraphicsRef graphics) {
	deref(graphics)->closePath(true);
}

ToveSubpathRef GraphicsBeginSubpath(ToveGraphicsRef graphics) {
	return trajectories.publish(deref(graphics)->beginSubpath());
}

void GraphicsCloseSubpath(ToveGraphicsRef graphics, bool close) {
	deref(graphics)->closeSubpath(close);
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

void GraphicsRemovePath(ToveGraphicsRef shape, TovePathRef path) {
	deref(shape)->removePath(deref(path));
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
	return deref(shape)->fetchChanges(flags);
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

void GraphicsGetBounds(ToveGraphicsRef shape, bool exact, ToveBounds *bounds) {
	const float *computed;

	if (exact) {
		computed = deref(shape)->getExactBounds();
	} else {
		computed  = deref(shape)->getBounds();
	}

	bounds->x0 = computed[0];
	bounds->y0 = computed[1];
	bounds->x1 = computed[2];
	bounds->y1 = computed[3];
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

/*ToveMeshResult GraphicsTesselate(
	ToveGraphicsRef graphics,
	ToveMeshRef mesh,
	const ToveTesselationSettings *quality,
	ToveMeshUpdateFlags flags) {

	return exception_safe(
		[graphics, mesh, quality, flags] () {

		return deref(graphics)->tesselate(
			deref(mesh),
			*(quality ? quality : getDefaultQuality()),
			flags);
	});
}*/

void GraphicsRasterize(
	ToveGraphicsRef shape, uint8_t *pixels, int width, int height, int stride,
	float tx, float ty, float scale, const ToveRasterizeSettings *settings) {

	deref(shape)->rasterize(
		pixels, width, height, stride, tx, ty, scale, settings);
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

ToveLineCap GraphicsGetLineCap(ToveGraphicsRef graphics) {
	return deref(graphics)->getLineCap();
}

void GraphicsSetLineCap(ToveGraphicsRef graphics, ToveLineCap cap) {
	deref(graphics)->setLineCap(cap);
}

bool GraphicsMorphify(const ToveGraphicsRef *graphics, int n) {
	std::vector<GraphicsRef> g;
	g.reserve(n);
	for (int i = 0; i < n; i++) {
		g.push_back(deref(graphics[i]));
	}
	return Graphics::morphify(g);
}

void GraphicsRotate(ToveGraphicsRef graphics, ToveElementType what, int k) {
	deref(graphics)->rotate(what, k);
}

void ReleaseGraphics(ToveGraphicsRef graphics) {
	shapes.release(graphics);
}


ToveFeedRef NewColorFeed(ToveGraphicsRef graphics, float scale) {
	return shaderLinks.publish(tove_make_shared<ColorFeed>(deref(graphics), scale));
}

ToveFeedRef NewGeometryFeed(TovePathRef path, bool enableFragmentShaderStrokes) {
	return shaderLinks.publish(tove_make_shared<GPUXFeed>(
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

void FeedBindPaintIndices(ToveFeedRef link, const ToveGradientData *data) {
	deref(link)->bindPaintIndices(*data);
}

void ReleaseFeed(ToveFeedRef link) {
	shaderLinks.release(link);
}


ToveMeshRef NewMesh(ToveNameRef name) {
	return meshes.publish(tove_make_shared<Mesh>(deref(name)));
}

ToveMeshRef NewColorMesh(ToveNameRef name) {
	return meshes.publish(tove_make_shared<ColorMesh>(deref(name)));
}

ToveMeshRef NewPaintMesh(ToveNameRef name) {
	return meshes.publish(tove_make_shared<PaintMesh>(deref(name)));
}

int MeshGetVertexCount(ToveMeshRef mesh) {
	return deref(mesh)->getVertexCount();
}

void MeshSetVertexBuffer(ToveMeshRef mesh, void *buffer, int32_t size) {
	deref(mesh)->setExternalVertexBuffer(buffer, size);
}

ToveTrianglesMode MeshGetIndexMode(ToveMeshRef mesh) {
	return deref(mesh)->getIndexMode();
}

int MeshGetIndexCount(ToveMeshRef mesh) {
	return deref(mesh)->getIndexCount();
}

void MeshCopyIndexData(ToveMeshRef mesh, void *buffer, int32_t size) {
	deref(mesh)->copyIndexData(
		static_cast<ToveVertexIndex*>(buffer),
		size / sizeof(ToveVertexIndex));
}

void MeshCacheKeyFrame(ToveMeshRef mesh) {
	deref(mesh)->cacheKeyFrame();
}

void MeshSetCacheSize(ToveMeshRef mesh, int size) {
	deref(mesh)->setCacheSize(size);
}

void ReleaseMesh(ToveMeshRef mesh) {
	meshes.release(mesh);
}

ToveTesselatorRef NewAdaptiveTesselator(float resolution, int recursionLimit) {
	return tesselators.publish(tove_make_shared<AdaptiveTesselator>(
		new AdaptiveFlattener<DefaultCurveFlattener>(
			DefaultCurveFlattener(resolution, recursionLimit))));
}

ToveTesselatorRef NewRigidTesselator(int subdivisions) {
	return tesselators.publish(tove_make_shared<RigidTesselator>(subdivisions));
}

ToveMeshUpdateFlags TesselatorTessGraphics(ToveTesselatorRef tess,
	ToveGraphicsRef graphics, ToveMeshRef mesh, ToveMeshUpdateFlags flags) {

	tove::MeshRef m = deref(mesh);
	return deref(tess)->graphicsToMesh(deref(graphics).get(), flags, m, m);
}

ToveMeshUpdateFlags TesselatorTessPath(ToveTesselatorRef tess,
	ToveGraphicsRef graphics, TovePathRef path,
	ToveMeshRef fillMesh, ToveMeshRef lineMesh, ToveMeshUpdateFlags flags) {

	const float *bounds = deref(graphics)->getBounds();
	const float extent = std::max(
		bounds[2] - bounds[0], bounds[3] - bounds[1]);

	int lineIndex = 0;
	int fillIndex = 0;

	deref(tess)->beginTesselate(deref(graphics).get(), 1.0f / extent);

	PathPaintInd empty;

	ToveMeshUpdateFlags result = deref(tess)->pathToMesh(
		flags, deref(path), 0, empty,
		deref(fillMesh), deref(lineMesh), fillIndex, lineIndex);

	deref(tess)->endTesselate();

	return result;
}

void TesselatorSetMaxSubdivisions(int subdivisions) {
	toveMaxFlattenSubdivisions = subdivisions;
}

bool TesselatorHasFixedSize(ToveTesselatorRef tess) {
	return deref(tess)->hasFixedSize();
}

void ReleaseTesselator(ToveTesselatorRef tess) {
	tesselators.release(tess);
}


TovePaletteRef NewPalette(const uint8_t *colors, int n) {
	return palettes.publish(tove_make_shared<Palette>(colors, n));
}

void ReleasePalette(TovePaletteRef palette) {
	palettes.release(palette);
}


ToveNameRef NewName(const char *s) {
	return names.publish(tove_make_shared<std::string>(s != nullptr ? s : ""));
}

void ReleaseName(ToveNameRef name) {
	names.release(name);
}

ToveNameRef CloneName(ToveNameRef name) {
	return names.publish(tove_make_shared<std::string>(*deref(name)));
}

void NameSet(ToveNameRef name, const char *s) {
	deref(name)->assign(s);
}

const char *NameCStr(ToveNameRef name) {
	if (!name.ptr) {
		return "noname";
	} else {
		return deref(name)->c_str();
	}
}


} // extern "C"

#endif // TOVE_TARGET_LOVE2D
