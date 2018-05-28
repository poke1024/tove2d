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

#include "trajectory.h"
#include "utils.h"
#include "path.h"
#include "intersect.h"

float *Trajectory::addPoints(int n) {
	if (isClosed()) {
		throw std::runtime_error(
			"cannot edit closed trajectory");
	}
	const int cpts = nextpow2(nsvg.npts + n);
	nsvg.pts = static_cast<float*>(
		realloc(nsvg.pts, cpts * 2 * sizeof(float)));
	if (!nsvg.pts) {
		throw std::bad_alloc();
	}
	float *p = &nsvg.pts[nsvg.npts * 2];
	nsvg.npts += n;
	changed(CHANGED_GEOMETRY);
	return p;
}

void Trajectory::setNumPoints(int npts) {
	if (isClosed()) {
		throw std::runtime_error(
			"cannot edit closed trajectory");
	}
	if (npts > nsvg.npts) {
		addPoints(npts - nsvg.npts);
	} else if (npts < nsvg.npts) {
		changed(CHANGED_GEOMETRY);
	} else {
		changed(CHANGED_POINTS);
	}
	nsvg.npts = npts;
}

int Trajectory::addCommand(ToveCommandType type, int index) {
	commands.push_back(
		Command{uint8_t(type), false, uint16_t(index), 1});
	return commands.size() - 1;
}

void Trajectory::updateCommands() const {
	const int numCommands = commands.size();
	for (int i = 0; i < numCommands; i++) {
		Command &command = commands[i];
		if (!command.dirty) {
			continue;
		}
		float *p = getMutablePoints(command.index);
		switch (command.type) {
			case TOVE_LINE_TO: {
				assert(nsvg.npts - command.index >= command.line.size());
				command.line.write(p);
			} break;
			case TOVE_DRAW_RECT: {
				command.rect.normalize();
				assert(nsvg.npts - command.index >= command.rect.size());
				command.rect.write(p);
			} break;
			case TOVE_DRAW_ELLIPSE: {
				assert(nsvg.npts - command.index >= command.ellipse.size());
				command.ellipse.write(p);
			} break;
		}
		command.dirty = false;
	}
	dirty &= ~DIRTY_COMMANDS;
}

Trajectory::Trajectory() {
	memset(&nsvg, 0, sizeof(nsvg));
	nsvg.closed = 0;

	for (int i = 0; i < 4; i++) {
		nsvg.bounds[i] = 0.0;
	}
	dirty = DIRTY_BOUNDS;
}

Trajectory::Trajectory(const NSVGpath *path) {
	memset(&nsvg, 0, sizeof(nsvg));
	nsvg.closed = path->closed;
	nsvg.npts = path->npts;
	const size_t size = path->npts * 2 * sizeof(float);
	nsvg.pts = static_cast<float*>(malloc(size));
	if (!nsvg.pts) {
		throw std::bad_alloc();
	}
	std::memcpy(nsvg.pts, path->pts, size);
	for (int i = 0; i < 4; i++) {
		nsvg.bounds[i] = path->bounds[i];
	}
	dirty = DIRTY_COEFFICIENTS | DIRTY_CURVE_BOUNDS;
}

Trajectory::Trajectory(const TrajectoryRef &t) {
	memset(&nsvg, 0, sizeof(nsvg));
	nsvg.closed = t->nsvg.closed;
	nsvg.npts = t->nsvg.npts;
	const size_t size = nsvg.npts * 2 * sizeof(float);
	nsvg.pts = static_cast<float*>(malloc(size));
	if (!nsvg.pts) {
		throw std::bad_alloc();
	}
	std::memcpy(nsvg.pts, t->nsvg.pts, size);
	for (int i = 0; i < 4; i++) {
		nsvg.bounds[i] = t->nsvg.bounds[i];
	}
	dirty = t->dirty;
}

int Trajectory::moveTo(float x, float y) {
	NSVGpath *p = &nsvg;
	const int index = nsvg.npts;
	if (p->npts > 0) {
		p->pts[(p->npts-1)*2+0] = x;
		p->pts[(p->npts-1)*2+1] = y;
	} else {
		addPoint(x, y);
	}
	return addCommand(TOVE_MOVE_TO, nsvg.npts - 1);
}

int Trajectory::lineTo(float x, float y) {
	const int index = nsvg.npts;
	if (index < 1) {
		return -1;
	}

	LinePrimitive line(x, y);
	float *p = addPoints(line.size());
	line.write(p);

	const int commandIndex = addCommand(TOVE_LINE_TO, index);
	commands[commandIndex].line = line;
	return commandIndex;
}

int Trajectory::curveTo(float cpx1, float cpy1, float cpx2, float cpy2, float x, float y) {
	const int index = nsvg.npts;
	float *q = addPoints(3);
	q[0] = cpx1; q[1] = cpy1;
	q[2] = cpx2; q[3] = cpy2;
	q[4] = x; q[5] = y;
	return addCommand(TOVE_CURVE_TO, index);
}

int Trajectory::arc(float x, float y, float r, float startAngle, float endAngle, bool counterclockwise) {
	const float deltaAngle = wrapAngle(endAngle - startAngle);

	if (std::abs(deltaAngle - 360.0f) < 0.01) {
		// the code below will produce nans for full circles, so guard against that here.
		return drawEllipse(x, y, r, r);
	}

	// https://stackoverflow.com/questions/5736398/how-to-calculate-the-svg-path-for-an-arc-of-a-circle
	const float largeArc = deltaAngle <= 180.0 ? 0.0 : 1.0;
	float args[7] = {r, r, 0, largeArc, counterclockwise ? 1.0f : 0.0f, 0, 0};
	float cpx, cpy; // start

	polarToCartesian(x, y, r, endAngle, &cpx, &cpy);
	polarToCartesian(x, y, r, startAngle, &args[5], &args[6]);

	const float x0 = cpx;
	const float y0 = cpy;

	int n = 0;
	const float *pts = nsvg::pathArcTo(&cpx, &cpy, args, n);

	float *p = addPoints(n + 2);
	p[0] = x0;
	p[1] = y0;
	std::memcpy(&p[2], pts, n * sizeof(float) * 2);
	p += (1 + n) * 2;
	p[0] = cpx;
	p[1] = cpy;

	return -1;
}

int Trajectory::drawRect(float x, float y, float w, float h, float rx, float ry) {
	RectPrimitive rect(x, y, w, h, rx, ry);
	const int index = nsvg.npts;
	float *p = addPoints(rect.size());
	rect.write(p);

	const int commandIndex = addCommand(TOVE_DRAW_RECT, index);
	commands[commandIndex].rect = rect;
	return commandIndex;
}

int Trajectory::drawEllipse(float cx, float cy, float rx, float ry) {
	const int index = nsvg.npts;
	EllipsePrimitive ellipse(cx, cy, rx, ry);
	float *p = addPoints(ellipse.size());
	ellipse.write(p);

	const int commandIndex = addCommand(TOVE_DRAW_ELLIPSE, index);
	commands[commandIndex].ellipse = ellipse;
	return commandIndex;
}

void Trajectory::setLovePoints(const float *pts, int npts) {
	bool loop = isClosed() && npts > 0;
	const int n1 = npts + (loop ? 1 : 0);
	setNumPoints(n1);
	std::memcpy(nsvg.pts, pts, npts * sizeof(float) * 2);
	if (loop) {
		nsvg.pts[n1 * 2 - 2] = nsvg.pts[0];
		nsvg.pts[n1 * 2 - 1] = nsvg.pts[1];
	}
	commands.clear();
}

float Trajectory::getCommandValue(int commandIndex, int what) {
	if (commandIndex < 0 ||commandIndex >= commands.size()) {
		return 0.0;
	}

	commit();

	const Command &command = commands[commandIndex];
	switch (command.type) {
		case TOVE_MOVE_TO: {
			if (what >= 4 && what <= 5) {
				return getCommandPoint(command, what - 4);
			}
		} break;
		case TOVE_LINE_TO: {
			switch (what) {
				case 4: return command.line.x;
				case 5: return command.line.y;
			}
		} break;
		case TOVE_CURVE_TO: {
			if (what >= 0 && what <= 5) {
				return getCommandPoint(command, what);
			}
		} break;
		case TOVE_DRAW_RECT: {
			switch (what) {
				case 4: return command.rect.x;
				case 5: return command.rect.y;
				case 6: return command.rect.w;
				case 7: return command.rect.h;
				case 102: return command.rect.rx;
				case 103: return command.rect.ry;
			}
		} break;
		case TOVE_DRAW_ELLIPSE: {
			switch (what) {
				case 100: return command.ellipse.cx;
				case 101: return command.ellipse.cy;
				case 102: return command.ellipse.rx;
				case 103: return command.ellipse.ry;
			}
		} break;
	}
	return 0.0;
}

void Trajectory::setCommandValue(int commandIndex, int what, float value) {
	if (commandIndex < 0 ||commandIndex >= commands.size()) {
		return;
	}

	Command &command = commands[commandIndex];
	switch (command.type) {
		case TOVE_MOVE_TO: {
			if (what >= 4 && what <= 5) {
				setCommandPoint(command, what - 4, value);
				changed(CHANGED_POINTS);
			}
		} break;
		case TOVE_LINE_TO: {
			switch (what) {
				case 4: command.line.x = value; break;
				case 5: command.line.y = value; break;
			}
			command.dirty = true;
			dirty |= DIRTY_COMMANDS;
			changed(CHANGED_POINTS);
		} break;
		case TOVE_CURVE_TO: {
			if (what >= 0 && what <= 5) {
				setCommandPoint(command, what, value);
				changed(CHANGED_POINTS);
			}
		} break;
		case TOVE_DRAW_RECT: {
			switch (what) {
				case 4: command.rect.x = value; break;
				case 5: command.rect.y = value; break;
				case 6: command.rect.w = value; break;
				case 7: command.rect.h = value; break;
				case 102: command.rect.rx = value; break;
				case 103: command.rect.ry = value; break;
			}
			command.dirty = true;
			dirty |= DIRTY_COMMANDS;
			changed(CHANGED_POINTS);
		} break;
		case TOVE_DRAW_ELLIPSE: {
			switch (what) {
				case 100: command.ellipse.cx = value; break;
				case 101: command.ellipse.cy = value; break;
				case 102: command.ellipse.rx = value; break;
				case 103: command.ellipse.ry = value; break;
			}
			command.dirty = true;
			dirty |= DIRTY_COMMANDS;
			changed(CHANGED_POINTS);
		} break;
	}

	// need to refresh some follow-up commands here as well
	// (e.g. lines that connect to the previous point).
	setCommandDirty(commandIndex + 1);
}

void Trajectory::setCommandDirty(int commandIndex) {
	const int n = commands.size();
	if (isClosed()) {
		commandIndex = (commandIndex % n + n) % n;
	}
	if (commandIndex < 0 ||commandIndex >= n) {
		return;
	}
	commands[commandIndex].dirty = true;
	dirty |= DIRTY_COMMANDS;
}

void Trajectory::updateBounds() {
	if ((dirty & DIRTY_BOUNDS) == 0) {
		return;
	}
	commit();
	for (int i = 0; i + 4 <= nsvg.npts; i += 3) {
		float *curve = &nsvg.pts[i * 2];

		float bounds[4];
		nsvg::curveBounds(bounds, curve);

		if (i == 0) {
			nsvg.bounds[0] = bounds[0];
			nsvg.bounds[1] = bounds[1];
			nsvg.bounds[2] = bounds[2];
			nsvg.bounds[3] = bounds[3];
		} else {
			nsvg.bounds[0] = std::min(nsvg.bounds[0], bounds[0]);
			nsvg.bounds[1] = std::min(nsvg.bounds[1], bounds[1]);
			nsvg.bounds[2] = std::max(nsvg.bounds[2], bounds[2]);
			nsvg.bounds[3] = std::max(nsvg.bounds[3], bounds[3]);
		}
	}
	dirty &= ~DIRTY_BOUNDS;
}

void Trajectory::transform(float sx, float sy, float tx, float ty) {
	commit();
	updateBounds();
	nsvg.bounds[0] = (nsvg.bounds[0] + tx) * sx;
	nsvg.bounds[1] = (nsvg.bounds[1] + ty) * sy;
	nsvg.bounds[2] = (nsvg.bounds[2] + tx) * sx;
	nsvg.bounds[3] = (nsvg.bounds[3] + ty) * sy;
	for (int i = 0; i < nsvg.npts; i++) {
		float *pt = &nsvg.pts[i * 2];
		pt[0] = (pt[0] + tx) * sx;
		pt[1] = (pt[1] + ty) * sy;
	}
}

bool Trajectory::isLoop() const {
	const int n = nsvg.npts;
	return n > 0 &&
		nsvg.pts[0] == nsvg.pts[n * 2 - 2] &&
		nsvg.pts[1] == nsvg.pts[n * 2 - 1];
}

void Trajectory::setIsClosed(bool closed) {
	if (nsvg.closed == closed) {
		return;
	}

	commit();

	if (nsvg.npts > 0 && closed && !isLoop()) {
		lineTo(nsvg.pts[0], nsvg.pts[1]);
	}

	nsvg.closed = closed;
}

#if 0
bool Trajectory::computeShaderCloseCurveData(
	ToveShaderGeometryData *shaderData,
	int target,
	ExtendedCurveData &extended) {

	commit();

	const int n = nsvg.npts;
	assert(n > 0);

	float p[8];
	const float *pts = nsvg.pts;

	float x = pts[0];
	float y = pts[1];
	float px = pts[n * 2 - 2];
	float py = pts[n * 2 - 1];

	float dx = x - px;
	float dy = y - py;

	p[0] = px; p[1] = py;
	p[2] = px + dx/3.0f; p[3] = py + dy/3.0f;
	p[4] = x - dx/3.0f; p[5] = y - dy/3.0f;
	p[6] = x; p[7] = y;

	computeShaderCurveData(shaderData, p, target, extended);

	if (!isClosed()) {
		extended.ignore |= IGNORE_LINE;
	}

	return extended.ignore != (IGNORE_FILL | IGNORE_LINE);
}
#endif

bool Trajectory::computeShaderCurveData(
	ToveShaderGeometryData *shaderData,
	int curveIndex,
	int target,
	ExCurveData &extended) {

	commit();

	uint16_t *curveTexturesDataBegin =
		reinterpret_cast<uint16_t*>(shaderData->curvesTexture) +
			shaderData->curvesTextureRowBytes * target / sizeof(uint16_t);

	uint16_t *curveTexturesData = curveTexturesDataBegin;

	ensureCurveData(DIRTY_COEFFICIENTS | DIRTY_CURVE_BOUNDS);
	const CurveData &curveData = curves[curveIndex];
	extended.bounds = &curveData.bounds;

	const coeff *bx = curveData.bx;
	const coeff *by = curveData.by;

	if (fabs(bx[0]) < 1e-2 && fabs(bx[1]) < 1e-2 && fabs(bx[2]) < 1e-2 &&
		fabs(by[0]) < 1e-2 && fabs(by[1]) < 1e-2 && fabs(by[2]) < 1e-2) {
		return false;
	}

	const float *pts = &nsvg.pts[curveIndex * 3 * 2];

	extended.endpoints.p1[0] = pts[0];
	extended.endpoints.p1[1] = pts[1];
	extended.endpoints.p2[0] = pts[6];
	extended.endpoints.p2[1] = pts[7];

#if 0
	printf("%d (%f, %f) (%f, %f) (%f, %f) (%f, %f)\n", target,
		pts[0], pts[1],
		pts[2], pts[3],
		pts[4], pts[5],
		pts[6], pts[7]);
#endif

	// write curve data.
	for (int i = 0; i < 4; i++) {
		store_fp16(curveTexturesData[i], bx[i]);
		store_fp16(curveTexturesData[i + 4], by[i]);
	}
	curveTexturesData += 8;

	for (int i = 0; i < 4; i++) {
		store_fp16(curveTexturesData[i], curveData.bounds.bounds[i]);
	}
	curveTexturesData += 4;

	if (shaderData->fragmentShaderStrokes) {
		curveData.storeRoots(curveTexturesData);
		curveTexturesData += 4;
	}

	assert(curveTexturesData - curveTexturesDataBegin <=
		4 * shaderData->curvesTextureSize[0]);

	return true;
}

void Trajectory::animate(const TrajectoryRef &a, const TrajectoryRef &b, float t) {
	const int n = a->nsvg.npts;
	if (b->nsvg.npts != n) {
		return;
	}

	commands.clear();
	if (nsvg.npts != n) {
		setNumPoints(n);
	}

	for (int i = 0; i < n * 2; i++) {
		nsvg.pts[i] = lerp(a->nsvg.pts[i], b->nsvg.pts[i], t);
	}

	nsvg.closed = a->nsvg.closed;
	changed(CHANGED_POINTS);
}

void Trajectory::updateNSVG() {
	// NanoSVG will crash if we give it incomplete curves. so we duplicate points
	// to make complete curves here.
	if (nsvg.npts > 0) {
		while (nsvg.npts != 3 * ncurves(nsvg.npts) + 1) {
			addPoint(nsvg.pts[2 * (nsvg.npts - 1) + 0],
				nsvg.pts[2 * (nsvg.npts - 1) + 1]);
		}
	}

	/*for (int i = 0; i < nsvg.npts; i++) {
		printf("%d %f %f\n", i, nsvg.pts[2 * i + 0], nsvg.pts[2 * i + 1]);
	}*/

	updateBounds();
}

void Trajectory::changed(ToveChangeFlags flags) {
	dirty |= DIRTY_BOUNDS | DIRTY_COEFFICIENTS | DIRTY_CURVE_BOUNDS;
	if (claimer) {
		claimer->changed(flags);
	}
}

void Trajectory::invert() {
	commit();
	const int n = nsvg.npts;
	const size_t size = n * 2 * sizeof(float);
	float *pts = static_cast<float*>(malloc(size));
	if (!pts) {
		throw std::bad_alloc();
	}
	for (int i = 0; i < n; i++) {
		const int j = n - 1 - i;
		pts[i * 2 + 0] = nsvg.pts[j * 2 + 0];
		pts[i * 2 + 1] = nsvg.pts[j * 2 + 1];
	}
	free(nsvg.pts);
	nsvg.pts = pts;

	for (int i = 0; i < commands.size(); i++) {
		commands[i].index = n - 1 - commands[i].index;
		commands[i].direction = -commands[i].direction;
	}
	changed(CHANGED_POINTS);
}

void Trajectory::clean(float eps) {
	commit();
	const int n = nsvg.npts;

	std::vector<float> cleaned;
	cleaned.reserve(n * 2);

	int copied = 0;
	for (int i = 0; i + 4 <= n; i += 3) {
		const float *pts = &nsvg.pts[i * 2];

		const float dx = pts[0] - pts[6];
		const float dy = pts[1] - pts[7];

		if (dx * dx + dy * dy > eps) {
			cleaned.insert(cleaned.end(), pts, pts + 6);
		}
		copied = i + 3;
	}
	cleaned.insert(cleaned.end(), &nsvg.pts[copied * 2], &nsvg.pts[n * 2]);

	if (cleaned.size() < nsvg.npts * 2) {
		nsvg.npts = cleaned.size() / 2;
		std::memcpy(nsvg.pts, &cleaned[0], sizeof(float) * cleaned.size());

		commands.clear();
		changed(CHANGED_GEOMETRY);
	}
}

ToveOrientation Trajectory::getOrientation() const {
	commit();
	const int n = getNumPoints();
	float area = 0;
	for (int i1 = 0; i1 < n; i1++) {
		const float *p1 = &nsvg.pts[i1 * 2];
		const int i2 = (i1 + 1) % n;
		const float *p2 = &nsvg.pts[i2 * 2];
		area += p1[0] * p2[1] - p1[1] * p2[0];
	}
	return area < 0 ? ORIENTATION_CW : ORIENTATION_CCW;
}

void Trajectory::setOrientation(ToveOrientation orientation) {
	if (getOrientation() != orientation) {
		invert();
	}
}

float Trajectory::getLovePointValue(int index, int dim) {
	int n = nsvg.npts;
	if (isClosed()) {
		n -= 1;
		index = (index % n + n) % n;
	}
	if (index >= 0 && index < n && (dim & 1) == dim) {
		commit();
		return nsvg.pts[2 * index + dim];
	} else {
		return 0.0f;
	}
}

void Trajectory::setLovePointValue(int index, int dim, float value) {
	int n = nsvg.npts;
	if (isClosed()) {
		n -= 1;
		index = (index % n + n) % n;
	}
	if (index >= 0 && index < n && (dim & 1) == dim) {
		commit();
		nsvg.pts[2 * index + dim] = value;
		if (isClosed()) {
			if (index == 0) {
				nsvg.pts[2 * n + dim] = value;
			}
		}
		changed(CHANGED_POINTS);
	}
}

void Trajectory::setCommandPoint(const Command &command, int what, float value) {
	float *p = nsvg.pts;
	int index = command.index + command.direction * (what / 2);
	p[2 * index + (what & 1)] = value;
	if (isClosed() && nsvg.npts > 0) {
		if (index == 0) {
			p[(nsvg.npts - 1) * 2 + (what & 1)] = value;
		} else if (index == nsvg.npts - 1) {
			p[0 + (what & 1)] = value;
		}
	}
}

void Trajectory::updateCurveData(uint8_t flags) const {
	commit();

	flags &= dirty;

	const int npts = nsvg.npts;
	const float *pts = nsvg.pts;
	const int nc = ncurves(npts);
	curves.resize(nc);

	for (int curve = 0; curve < nc; curve++) {
		const float *p = &pts[curve * 2 * 3];
		CurveData &c = curves[curve];

		if (flags & DIRTY_COEFFICIENTS) {
			c.updatePCs(p);
		}
		if (flags & DIRTY_CURVE_BOUNDS) {
			c.updateBounds(p);
		}
	}

	dirty &= ~flags;
}

void Trajectory::testInside(float x, float y, AbstractInsideTest &test) const {
	ensureCurveData(DIRTY_COEFFICIENTS);
	const int nc = ncurves(nsvg.npts);
	for (int i = 0; i < nc; i++) {
		test.add(curves[i].bx, curves[i].by, x, y);
	}
}

void Trajectory::intersect(const AbstractRay &ray, Intersecter &intersecter) const {
	ensureCurveData(DIRTY_COEFFICIENTS);
	const int nc = ncurves(nsvg.npts);
	for (int i = 0; i < nc; i++) {
		intersecter.intersect(curves[i].bx, curves[i].by, ray);
	}
}

ToveVec2 Trajectory::getPosition(float globalt) const {
	ensureCurveData(DIRTY_COEFFICIENTS);
	const int nc = ncurves(nsvg.npts);

	float curveflt;
	float t = modf(globalt, &curveflt);
	int curve = int(curveflt);
	if (curve < nc) {
		float t2 = t * t;
		float t3 = t2 * t;

		return ToveVec2{
			float(dot4(curves[curve].bx, t3, t2, t, 1)),
			float(dot4(curves[curve].by, t3, t2, t, 1))};
	} else {
		return ToveVec2{0.0f, 0.0f};
	}
}

ToveVec2 Trajectory::getNormal(float globalt) const {
	ensureCurveData(DIRTY_COEFFICIENTS);
	const int nc = ncurves(nsvg.npts);

	float curveflt;
	float t = modf(globalt, &curveflt);
	int curve = int(curveflt);

	if (curve < nc) {
		float t2 = t * t;

		double nx = dot3(curves[curve].by, 3 * t2, 2 * t, 1);
		double ny = -dot3(curves[curve].bx, 3 * t2, 2 * t, 1);

		double len = sqrt(nx * nx + ny * ny);
		return ToveVec2{
			float(nx / len),
			float(ny / len)};
	} else {
		return ToveVec2{0.0f, 0.0f};
	}
}

inline float distance(const coeff *bx, const coeff *by, float t, float x, float y) {
	float t2 = t * t;
	float t3 = t2 * t;
	float dx = dot4(bx, t3, t2, t, 1) - x;
	float dy = dot4(by, t3, t2, t, 1) - y;
	return dx * dx + dy * dy;
}

struct Nearest {
	float t;
	float distanceSquared;
};

void bisect(
	const coeff *bx, const coeff *by,
	float t0, float t1,
	float x, float y,
	float curveIndex,
	Nearest &nearest,
	float eps2) {

	float s = (t1 - t0) * 0.5f;
	float t = t0 + s;

	float bestT = curveIndex + t;
	float bestDistance = distance(bx, by, t, x, y);

	for (int i = 0; i < 10 && bestDistance > eps2; i++) {
		s *= 0.5f;

		float d0 = distance(bx, by, t - s, x, y);
		float d1 = distance(bx, by, t + s, x, y);

		if (d0 < d1) {
			if (d0 < bestDistance) {
				t -= s;
				bestDistance = d0;
				bestT = curveIndex + t;
			}
		} else {
			if (d1 < bestDistance) {
				t += s;
				bestDistance = d1;
				bestT = curveIndex + t;
			}
		}
	}

	if (bestDistance < nearest.distanceSquared) {
		nearest.t = bestT;
		nearest.distanceSquared = bestDistance;
	}
}

float Trajectory::closest(float x, float y, float dmin, float dmax) const {
	ensureCurveData(DIRTY_COEFFICIENTS | DIRTY_CURVE_BOUNDS);
	const int nc = ncurves(nsvg.npts);
	const float eps2 = dmin * dmin;

	Nearest nearest;
	nearest.distanceSquared = 1e50;
	nearest.t = -1;

	for (int curve = 0; curve < nc; curve++) {
		const CurveData &c = curves[curve];

		if (x < c.bounds.bounds[0] - dmax ||
			x > c.bounds.bounds[2] + dmax ||
			y < c.bounds.bounds[1] - dmax ||
			y > c.bounds.bounds[3] + dmax) {
			continue;
		}

		const float *roots = c.bounds.sroots;
		float t0 = 0.0f;
		for (int j = 0; j < 5; j++) {
			float t1 = j < 4 ? roots[j] : 1.0f;
			bisect(c.bx, c.by, t0, t1, x, y, curve, nearest, eps2);
			if (nearest.distanceSquared < eps2) {
				return nearest.t;
			}
			t0 = t1;
			if (t0 >= 1.0f) {
				break;
			}
		}

		dmax = std::min(dmax, std::sqrt(nearest.distanceSquared));
	}

	return nearest.t;
}
