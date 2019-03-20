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

#include "subpath.h"
#include "utils.h"
#include "path.h"
#include "intersect.h"

BEGIN_TOVE_NAMESPACE

inline float length(float x, float y) {
	return std::sqrt(x * x + y * y);
}

float *Subpath::addPoints(int n, bool allowClosedEdit) {
	if (!allowClosedEdit && isClosed()) {
		tove::report::warn("editing closed trajectory.");
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

void Subpath::setNumPoints(int npts) {
	if (npts == nsvg.npts) {
		return;
	}
	if (npts > nsvg.npts) {
		addPoints(npts - nsvg.npts, true);
	} else if (npts < nsvg.npts) {
		changed(CHANGED_GEOMETRY);
	} else {
		changed(CHANGED_POINTS);
	}
	nsvg.npts = npts;
}

int Subpath::addCommand(ToveCommandType type, int index) {
	commands.push_back(
		Command{uint8_t(type), false, uint16_t(index), 1});
	return commands.size() - 1;
}

void Subpath::updateCommands() const {
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

Subpath::Subpath() {
	memset(&nsvg, 0, sizeof(nsvg));
	nsvg.closed = 0;

	for (int i = 0; i < 4; i++) {
		nsvg.bounds[i] = 0.0;
	}
	dirty = DIRTY_BOUNDS;
}

Subpath::Subpath(const NSVGpath *path) {
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

Subpath::Subpath(const SubpathRef &t) {
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
	commands = t->commands;
	dirty = t->dirty | DIRTY_COEFFICIENTS | DIRTY_CURVE_BOUNDS;
}

int Subpath::moveTo(float x, float y) {
	NSVGpath *p = &nsvg;
	// const int index = nsvg.npts;
	if (p->npts > 0) {
		p->pts[(p->npts-1)*2+0] = x;
		p->pts[(p->npts-1)*2+1] = y;
	} else {
		addPoint(x, y);
	}
	return addCommand(TOVE_MOVE_TO, nsvg.npts - 1);
}

int Subpath::lineTo(float x, float y) {
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

int Subpath::curveTo(float cpx1, float cpy1, float cpx2, float cpy2, float x, float y) {
	const int index = nsvg.npts;
	float *q = addPoints(3);
	q[0] = cpx1; q[1] = cpy1;
	q[2] = cpx2; q[3] = cpy2;
	q[4] = x; q[5] = y;
	return addCommand(TOVE_CURVE_TO, index);
}

int Subpath::arc(float x, float y, float r, float startAngle, float endAngle, bool counterclockwise) {
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

int Subpath::drawRect(float x, float y, float w, float h, float rx, float ry) {
	RectPrimitive rect(x, y, w, h, rx, ry);
	const int index = nsvg.npts;
	float *p = addPoints(rect.size());
	rect.write(p);

	const int commandIndex = addCommand(TOVE_DRAW_RECT, index);
	commands[commandIndex].rect = rect;
	return commandIndex;
}

int Subpath::drawEllipse(float cx, float cy, float rx, float ry) {
	const int index = nsvg.npts;
	EllipsePrimitive ellipse(cx, cy, rx, ry);
	float *p = addPoints(ellipse.size());
	ellipse.write(p);

	const int commandIndex = addCommand(TOVE_DRAW_ELLIPSE, index);
	commands[commandIndex].ellipse = ellipse;
	return commandIndex;
}

int Subpath::insertCurveAt(float globalt) {
	const int npts0 = nsvg.npts;

	if (npts0 < 4) {
		return 0;
	}

	int curve;
	float t;
	if (globalt >= ncurves(npts0)) {
		curve = ncurves(npts0) - 1;
		t = 1.0f;
	} else {
		globalt = std::max(globalt, 0.0f);
		float curveflt;
		t = modff(globalt, &curveflt);
		curve = int(curveflt);
	}

	const int i = std::min(std::max(curve * 3, 0), npts0 - 4);

	addPoints(3, true);
	float *pts = nsvg.pts;

	std::memmove(
		&pts[2 * (i + 7)],
		&pts[2 * (i + 4)],
		(nsvg.npts - (i + 7)) * sizeof(float) * 2);

	const float s = 1 - t;

	for (int j = 0; j < 2; j++) {
		const float p0 = pts[2 * (i + 0) + j];
		const float p1 = pts[2 * (i + 1) + j];
		const float p2 = pts[2 * (i + 2) + j];
		const float p3 = pts[2 * (i + 3) + j];

		const float p0_1 = s * p0 + t * p1;
		const float p1_2 = s * p1 + t * p2;
		const float p2_3 = s * p2 + t * p3;

		const float p01_12 = s * p0_1 + t * p1_2;
		const float p12_23 = s * p1_2 + t * p2_3;

		const float p0112_1223 = s * p01_12 + t * p12_23;

		pts[2 * (i + 0) + j] = p0;
		pts[2 * (i + 1) + j] = p0_1;
		pts[2 * (i + 2) + j] = p01_12;

		pts[2 * (i + 3) + j] = p0112_1223;

		pts[2 * (i + 4) + j] = p12_23;
		pts[2 * (i + 5) + j] = p2_3;
		pts[2 * (i + 6) + j] = p3;
	}

	fixLoop();

	return i + 4;
}

void Subpath::removeCurve(int curve) {
	const int npts = nsvg.npts - (isClosed() ? 1 : 0);

	if (npts < 7) {
		return;
	}

	const int nc = ncurves(nsvg.npts);
	curve -= 1;
	curve = (curve % nc + nc) % nc;

	const int i = std::max(curve * 3, 0);
	float *pts = nsvg.pts;

	if (isLineAt(i + 3, 0)) {
		const int k = i + 3;
		if (k > 0 && k + 3 < npts) {
			float *p0 = &pts[2 * (k - 3)];
			float *p1 = &pts[2 * (k + 3)];
			for (int j = 0; j < 2; j++) {
				pts[2 * (k - 2) + j] = (p0[j] * 2 + p1[j]) / 3.0;
				pts[2 * (k + 2) + j] = (p0[j] + p1[j] * 2) / 3.0;
			}
			remove(k - 1, 3);
		}
		return;
	}

	float t = 0.0f;

	for (int j = 0; j < 2; j++) {
		const float p01_12 = pts[2 * ((i + 2) % npts) + j];
		const float p0112_1223 = pts[2 * ((i + 3) % npts) + j];
		const float p12_23 = pts[2 * ((i + 4) % npts) + j];

		if (std::abs(p01_12 - p12_23) < 1e-4) {
			t += 0.5f;
		} else {
			t += (p01_12 - p0112_1223) / (p01_12 - p12_23);
		}
	}

	t /= 2.0f;
	float s = 1.0f - t;

	for (int j = 0; j < 2; j++) {
		const float p0 = pts[2 * ((i + 0) % npts) + j];
		const float p0_1 = pts[2 * ((i + 1) % npts) + j];
		const float p2_3 = pts[2 * ((i + 5) % npts) + j];
		const float p3 = pts[2 * ((i + 6) % npts) + j];

		pts[2 * ((i + 1) % npts) + j] = (p0_1 - s * p0) / t; // p1
		pts[2 * ((i + 2) % npts) + j] = (p2_3 - t * p3) / s; // p2
		pts[2 * ((i + 3) % npts) + j] = p3;
	}

	remove((i + 4) % npts, 3);
}

void Subpath::remove(int from, int n) {
	assert(from < nsvg.npts);
	assert(n < nsvg.npts);

	if (!isClosed()) {
		n = std::min(n, nsvg.npts - from);
	}

	int clipAtStart, clipAtEnd;

	if (from + n <= nsvg.npts) {
		clipAtEnd = n;
		clipAtStart = 0;
	} else {
		clipAtEnd = nsvg.npts - from;
		clipAtStart = n - clipAtEnd;
	}

	float *pts = nsvg.pts;

	std::memmove(
		&pts[2 * from],
		&pts[2 * (from + clipAtEnd)],
		(nsvg.npts - (from + clipAtEnd)) * sizeof(float) * 2);

	std::memmove(
		pts,
		&pts[2 * clipAtStart],
		(nsvg.npts - n) * sizeof(float) * 2);

	setNumPoints(nsvg.npts - n);

	fixLoop();
}

int Subpath::mould(float globalt, float x, float y) {
	// adapted from https://pomax.github.io/bezierinfo/#moulding

	const int nc = ncurves(nsvg.npts);

	float curveflt;
	float t = modff(globalt, &curveflt);
	int curve = int(curveflt);
	if (curve < 0 || curve >= nc) {
		return -1;
	}

	const float t2 = t * t;
	const float t3 = t2 * t;

	const float B[2] = {x, y};

	const float s = 1.0f - t;
	const float s3 = s * s * s;

	const float u = s3 / (t3 + s3);
	const float v = 1.0f - u;
	const float ratio = std::abs((t3 + s3 - 1.0f) / (t3 + s3));

	float * const pts = nsvg.pts;

	const int i = curve * 3;
	float * const S = &pts[i * 2 + 0];
	float * const E = &pts[i * 2 + 6];

	float * const CP1 = &pts[i * 2 + 2];
	float * const CP2 = &pts[i * 2 + 4];

	const float C[2] = {
		u * S[0] + v * E[0],
		u * S[1] + v * E[1]
	};

	float e1[2], e2[2];
	for (int j = 0; j < 2; j++) {
		// de Casteljau, stage 1
		float q1 = S[j] * s + CP1[j] * t;
		float q2 = CP1[j] * s + CP2[j] * t;
		float q3 = CP2[j] * s + E[j] * t;

		// de Casteljau, stage 2
		float r1 = q1 * s + q2 * t;
		float r2 = q2 * s + q3 * t;

		// de Casteljau, stage 3
		float B_old = r1 * s + r2 * t;

		e1[j] = B[j] + (r1 - B_old);
		e2[j] = B[j] + (r2 - B_old);
	}

	for (int j = 0; j < 2; j++) {
		float A = B[j] + (B[j] - C[j]) / ratio;

		float v1 = A + (e1[j] - A) / s;
		float v2 = A + (e2[j] - A) / t;

		float cp1 = S[j] + (v1 - S[j]) / t;
		float cp2 = E[j] + (v2 - E[j]) / s;

		CP1[j] = cp1;
		CP2[j] = cp2;
	}

	changed(CHANGED_POINTS);

	return curve;
}

inline float polar(
	float x, float y,
	float ox, float oy,
	float *phi = nullptr) {

	const float dx = x - ox;
	const float dy = y - oy;
	if (phi) {
		*phi = std::atan2(dy, dx);
	}
	return std::sqrt(dx * dx + dy * dy);
}

static void alignHandle(
	float px, float py,
	float p0x, float p0y,
	float *cp1) {

	float newphi;
	const float newmag = polar(px, py, p0x, p0y, &newphi);

	const float cp1mag = polar(cp1[0], cp1[1], p0x, p0y);

	const float alignedPhi = newphi + M_PI;
	cp1[0] = p0x + std::cos(alignedPhi) * cp1mag;
	cp1[1] = p0y + std::sin(alignedPhi) * cp1mag;
}

void Subpath::makeFlat(int k, int dir) {
	if (k % 3 != 0) {
		return; // not a knot point
	}

	const bool closed = isClosed();
	const int n = nsvg.npts - (closed ? 1 : 0);

	if (n < 3 || k < 0 || k >= n) {
		return;
	}

	commit();
	float *pts = nsvg.pts;

	const float x = pts[2 * k + 0];
	const float y = pts[2 * k + 1];

	if (dir <= 0 && (k - 3 >= 0 || closed)) {
		const int u = (k + n - 3) % n;
		const int w = (k + n - 1) % n;

		const float dx = pts[2 * u + 0] - x;
		const float dy = pts[2 * u + 1] - y;
		pts[2 * w + 0] = x + dx / 3.0;
		pts[2 * w + 1] = y + dy / 3.0;
	}

	if (dir >= 0 && (k + 3 < n || closed)) {
		const int u = (k + 3) % n;
		const int w = (k + 1) % n;

		const float dx = pts[2 * u + 0] - x;
		const float dy = pts[2 * u + 1] - y;
		pts[2 * w + 0] = x + dx / 3.0;
		pts[2 * w + 1] = y + dy / 3.0;
	}

	fixLoop();
	changed(CHANGED_POINTS);
}


inline float distance(float x1, float y1, float x2, float y2) {
	const float dx = x1 - x2;
	const float dy = y1 - y2;
	return std::sqrt(dx * dx + dy * dy);
}

void Subpath::makeSmooth(int k, int dir, float a) {
	// Catmull-Rom smoothing, adapted from paper.js's
	// Segment.smooth.

	if (k % 3 != 0) {
		return; // not a knot point
	}

	const bool closed = isClosed();
	const int n = nsvg.npts - (closed ? 1 : 0);

	if (closed) {
		k = (k % n + n) % n;
	}

	if (n < 3 || k < 0 || k >= n) {
		return;
	}

	commit();
	float *pts = nsvg.pts;

	const float p1x = pts[2 * k + 0];
	const float p1y = pts[2 * k + 1];

	const bool prev = closed || k - 3 >= 0;
	const bool next = closed || k + 3 < n;

	float p0x, p0y, p2x, p2y;

	if (prev) {
		const int u = (k + n - 3) % n;
		p0x = pts[2 * u + 0];
		p0y = pts[2 * u + 1];
	} else {
		p0x = p1x;
		p0y = p1y;
	}

	if (next) {
		const int u = (k + 3) % n;
		p2x = pts[2 * u + 0];
		p2y = pts[2 * u + 1];
	} else {
		p2x = p1x;
		p2y = p1y;
	}

	// hacky: to provide smoothing for start and end
	// of non-closed paths, we mirror the adjacent
	// knot's control point.
	if (!prev && next) {
		const int u = (k + 2) % n;
		p0x = p1x - (pts[2 * u + 0] - p1x);
		p0y = p1y - (pts[2 * u + 1] - p1y);
	} else if (prev && !next) {
		const int u = (k + n - 2) % n;
		p2x = p1x - (pts[2 * u + 0] - p1x);
		p2y = p1y - (pts[2 * u + 1] - p1y);
	}

	const float d1 = distance(p0x, p0y, p1x, p1y);
	const float d2 = distance(p1x, p1y, p2x, p2y);

	const float d1_a = std::pow(d1, a);
    const float d1_2a = d1_a * d1_a;
    const float d2_a = std::pow(d2, a);
    const float d2_2a = d2_a * d2_a;

	if (dir <= 0 && prev) {
		const float A = 2 * d2_2a + 3 * d2_a * d1_a + d1_2a;
		const float N = 3 * d2_a * (d2_a + d1_a);
		if (std::abs(N) > 1e-6) {
			const int w = (k + n - 1) % n;
			pts[w * 2 + 0] =
				(d2_2a * p0x + A * p1x - d1_2a * p2x) / N;
			pts[w * 2 + 1] =
				(d2_2a * p0y + A * p1y - d1_2a * p2y) / N;
		} else {
			makeFlat(k, -1);
		}
	}

	if (dir >= 0 && next) {
		const float A = 2 * d1_2a + 3 * d1_a * d2_a + d2_2a;
		const float N = 3 * d1_a * (d1_a + d2_a);
		if (std::abs(N) > 1e-6) {
			const int w = (k + 1) % n;
			pts[w * 2 + 0] =
				(d1_2a * p2x + A * p1x - d2_2a * p0x) / N;
			pts[w * 2 + 1] =
				(d1_2a * p2y + A * p1y - d2_2a * p0y) / N;
		} else {
			makeFlat(k, 1);
		}
	}

	fixLoop();
	changed(CHANGED_POINTS);
}

void Subpath::move(int k, float x, float y, ToveHandle handle) {
	const bool closed = isClosed();
	const int n = nsvg.npts - (closed ? 1 : 0);

	if (closed) {
		k = (k % n + n) % n;
	}

	if (n < 3 || k < 0 || k >= n) {
		return;
	}

	commit();
	float *pts = nsvg.pts;

	const int t = k % 3;
	if (t == 0) {
		// not a control point.
		const int k0 = (k + n - 3) % n;
		const int k1 = k;
		const int k2 = (k + 3) % n;

		const bool f0 = isLineAt(k, -1);
		const bool f1 = f0 && (closed || k - 3 >= 0) && isLineAt(k0, 1);

		const bool f2 = isLineAt(k, 1);
		const bool f3 = f2 && (closed || k + 3 < n) && isLineAt(k2, -1);

		const float qx = pts[2 * k + 0];
		const float qy = pts[2 * k + 1];

		pts[2 * k + 0] = x;
		pts[2 * k + 1] = y;

		// fix edge lines.
		if (f0) {
			makeFlat(k1, -1);
			if (f1) {
				makeFlat(k0, 1);
			}
		}
		if (f2) {
			makeFlat(k1, 1);
			if (f3) {
				makeFlat(k2, -1);
			}
		}

		// move adjacent control points.
		const float dx = x - qx;
		const float dy = y - qy;

		if (!f0 && (closed || k - 1 >= 0)) {
			const int u = (k + n - 1) % n;
			pts[2 * u + 0] += dx;
			pts[2 * u + 1] += dy;
		}
		if (!f2 && (closed || k + 1 < n)) {
			const int u = (k + 1) % n;
			pts[2 * u + 0] += dx;
			pts[2 * u + 1] += dy;
		}
	} else {
		// moving a control point.
		pts[2 * k + 0] = x;
		pts[2 * k + 1] = y;

		if (handle == TOVE_HANDLE_ALIGNED) {
			const int knot = (t == 1) ? k - 1 : k + 1;
			const int opposite = (t == 1) ? k - 2 : k + 2;

			if (closed || (opposite >= 0 && opposite < n)) {
				float *p0 = &pts[2 * ((knot + n) % n)];
				float *cp1 = &pts[2 * ((opposite + n) % n)];
				alignHandle(x, y, p0[0], p0[1], cp1);
			}
		}
	}

	fixLoop();
	changed(CHANGED_POINTS);
}

void Subpath::setPoints(const float *pts, int npts, bool add_loop) {
	const bool loop = add_loop && isClosed() && npts > 0;
	const int n1 = npts + (loop ? 1 : 0);
	setNumPoints(n1);
	std::memcpy(nsvg.pts, pts, npts * sizeof(float) * 2);
	if (loop) {
		nsvg.pts[n1 * 2 - 2] = nsvg.pts[0];
		nsvg.pts[n1 * 2 - 1] = nsvg.pts[1];
	}
	commands.clear();
	changed(CHANGED_POINTS);
}

bool Subpath::isCollinear(int u, int v, int w) const {
	const int n = nsvg.npts - (isClosed() ? 1 : 0);
	if (n < 1) {
		return false;
	}
	const float *pts = nsvg.pts;

	u = (u % n + n) % n;
	v = (v % n + n) % n;
	w = (w % n + n) % n;

	const float Ax = pts[2 * u + 0];
	const float Ay = pts[2 * u + 1];
	const float Bx = pts[2 * v + 0];
	const float By = pts[2 * v + 1];
	const float Cx = pts[2 * w + 0];
	const float Cy = pts[2 * w + 1];

	return std::abs(Ax * (By - Cy) + Bx * (Cy - Ay) + Cx * (Ay - By)) < 0.1;
}

bool Subpath::isLineAt(int k, int dir) const {
	if (k % 3 != 0) {
		return false; // not a knot point
	}

	const int n = nsvg.npts;
	if (n <= 3) {
		return true;
	}

	if (!isClosed()) {
		if (k < 0 || k >= n) {
			return true;
		} else if (k < 3) {
			return dir >= 0 ? isCollinear(k, k + 1, k + 3) : true;
		} else if (k + 3 >= n) {
			return dir <= 0 ? isCollinear(k - 3, k - 1, k) : true;
		}
	}

	bool r = false;
	if (dir <= 0) {
		r = r || isCollinear(k - 3, k - 1, k);
	}
	if (dir >= 0) {
		r = r || isCollinear(k, k + 1, k + 3);
	}

	return r;
}

float Subpath::getCommandValue(int commandIndex, int what) {
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

void Subpath::setCommandValue(int commandIndex, int what, float value) {
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

void Subpath::setCommandDirty(int commandIndex) {
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

void Subpath::updateBounds() {
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

void Subpath::set(const SubpathRef &t, const nsvg::Transform &transform) {
	commit();
	t->commit();
	const int npts = t->nsvg.npts;
	setNumPoints(npts);
	transform.transformPoints(nsvg.pts, t->nsvg.pts, npts);
	nsvg.closed = t->nsvg.closed;
	changed(CHANGED_POINTS);
}

bool Subpath::isLoop() const {
	const int n = nsvg.npts;
	return n > 0 &&
		nsvg.pts[0] == nsvg.pts[n * 2 - 2] &&
		nsvg.pts[1] == nsvg.pts[n * 2 - 1];
}

void Subpath::fixLoop() {
	const int npts = nsvg.npts;
	if (npts > 0 && isClosed()) {
		nsvg.pts[npts * 2 - 2] = nsvg.pts[0];
		nsvg.pts[npts * 2 - 1] = nsvg.pts[1];
	}
}

void Subpath::setIsClosed(bool closed) {
	if (nsvg.closed == closed) {
		return;
	}

	commit();

	if (nsvg.npts > 0 && closed && !isLoop()) {
		lineTo(nsvg.pts[0], nsvg.pts[1]);
	}

	nsvg.closed = closed;
}

bool Subpath::computeShaderCurveData(
	ToveShaderGeometryData *shaderData,
	int curveIndex,
	int target,
	ExCurveData &extended) {

	commit();

	gpu_float_t *curveTexturesDataBegin =
		reinterpret_cast<gpu_float_t*>(shaderData->curvesTexture) +
			shaderData->curvesTextureRowBytes * target / sizeof(gpu_float_t);

	gpu_float_t *curveTexturesData = curveTexturesDataBegin;

	ensureCurveData(DIRTY_COEFFICIENTS | DIRTY_CURVE_BOUNDS);
	assert(curves.size() > 0);

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
		store_gpu_float(curveTexturesData[i], bx[i]);
		store_gpu_float(curveTexturesData[i + 4], by[i]);
	}
	curveTexturesData += 8;

	for (int i = 0; i < 4; i++) {
		store_gpu_float(curveTexturesData[i], curveData.bounds.bounds[i]);
	}
	curveTexturesData += 4;

	if (shaderData->fragmentShaderLine) {
		curveData.storeRoots(curveTexturesData);
		curveTexturesData += 4;
	}

	assert(curveTexturesData - curveTexturesDataBegin <=
		4 * shaderData->curvesTextureSize[0]);

	return true;
}

#if TOVE_DEBUG
std::ostream &Subpath::dump(std::ostream &os) {
	for (int i = 0; i < nsvg.npts; i++) {
		if (i > 0) {
			os << ", ";
		}
		os << "(" << nsvg.pts[2 * i + 0] <<
			", " <<  nsvg.pts[2 * i + 1] << ")";
	}
	os << std::endl;
	return os;
}
#endif

bool Subpath::animate(const SubpathRef &a, const SubpathRef &b, float t) {
	commands.clear();

	const int nptsA = a->nsvg.npts;
	const int nptsB = b->nsvg.npts;

	if (nptsA != nptsB) {
		if (t < 0.5) {
			setNumPoints(nptsA);
			for (int i = 0; i < nptsA * 2; i++) {
				nsvg.pts[i] = a->nsvg.pts[i];
			}
			nsvg.closed = a->nsvg.closed;
		} else {
			setNumPoints(nptsB);
			for (int i = 0; i < nptsB * 2; i++) {
				nsvg.pts[i] = b->nsvg.pts[i];
			}
			nsvg.closed = b->nsvg.closed;
		}

		changed(CHANGED_POINTS);
		return false;
	} else {
		const int n = nptsA;
		if (nsvg.npts != n) {
			setNumPoints(n);
		}

		for (int i = 0; i < n * 2; i++) {
			nsvg.pts[i] = lerp(a->nsvg.pts[i], b->nsvg.pts[i], t);
		}

		nsvg.closed = t < 0.5 ? a->nsvg.closed : b->nsvg.closed;
	}

	changed(CHANGED_POINTS);
	return true;
}

void Subpath::updateNSVG() {
	// NanoSVG will crash if we give it incomplete curves. so we duplicate points
	// to make complete curves here.
	if (nsvg.npts > 0) {
		while (nsvg.npts != 3 * ncurves(nsvg.npts) + 1) {
			addPoint(
				nsvg.pts[2 * (nsvg.npts - 1) + 0],
				nsvg.pts[2 * (nsvg.npts - 1) + 1],
				true);
		}
	}

	/*for (int i = 0; i < nsvg.npts; i++) {
		printf("%d %f %f\n", i, nsvg.pts[2 * i + 0], nsvg.pts[2 * i + 1]);
	}*/

	updateBounds();
}

void Subpath::changed(ToveChangeFlags flags) {
	dirty |= DIRTY_BOUNDS | DIRTY_COEFFICIENTS | DIRTY_CURVE_BOUNDS;
	broadcastChange(flags);
}

void Subpath::invert() {
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

void Subpath::clean(float eps) {
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

ToveOrientation Subpath::getOrientation() const {
	commit();
	const int n = getNumPoints();
	float area = 0;
	for (int i1 = 0; i1 < n; i1++) {
		const float *p1 = &nsvg.pts[i1 * 2];
		const int i2 = (i1 + 1) % n;
		const float *p2 = &nsvg.pts[i2 * 2];
		area += p1[0] * p2[1] - p1[1] * p2[0];
	}
	return area < 0 ? TOVE_ORIENTATION_CW : TOVE_ORIENTATION_CCW;
}

void Subpath::setOrientation(ToveOrientation orientation) {
	if (getOrientation() != orientation) {
		invert();
	}
}

float Subpath::getPointValue(int index, int dim) {
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

void Subpath::setPointValue(int index, int dim, float value) {
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

void Subpath::setCommandPoint(const Command &command, int what, float value) {
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

void Subpath::updateCurveData(uint8_t flags) const {
	commit();

	flags &= dirty & (DIRTY_COEFFICIENTS | DIRTY_CURVE_BOUNDS);

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

void Subpath::testInside(float x, float y, AbstractInsideTest &test) const {
	ensureCurveData(DIRTY_COEFFICIENTS);
	const int nc = ncurves(nsvg.npts);
	for (int i = 0; i < nc; i++) {
		test.add(curves[i].bx, curves[i].by, x, y);
	}
}

void Subpath::intersect(const AbstractRay &ray, Intersecter &intersecter) const {
	ensureCurveData(DIRTY_COEFFICIENTS);
	const int nc = ncurves(nsvg.npts);
	for (int i = 0; i < nc; i++) {
		intersecter.intersect(curves[i].bx, curves[i].by, ray);
	}
}

ToveVec2 Subpath::getPosition(float globalt) const {
	ensureCurveData(DIRTY_COEFFICIENTS);
	const int nc = ncurves(nsvg.npts);

	float curveflt;
	float t = modff(globalt, &curveflt);
	int curve = int(curveflt);
	if (curve >= 0 && curve < nc) {
		float t2 = t * t;
		float t3 = t2 * t;

		return ToveVec2{
			float(dot4(curves[curve].bx, t3, t2, t, 1)),
			float(dot4(curves[curve].by, t3, t2, t, 1))};
	} else {
		return ToveVec2{0.0f, 0.0f};
	}
}

ToveVec2 Subpath::getNormal(float globalt) const {
	ensureCurveData(DIRTY_COEFFICIENTS);
	const int nc = ncurves(nsvg.npts);

	float curveflt;
	float t = modff(globalt, &curveflt);
	int curve = int(curveflt);

	if (curve >= 0 && curve < nc) {
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

inline float distance(
	const coeff *bx, const coeff *by, float t,
	float x, float y) {

	float t2 = t * t;
	float t3 = t2 * t;
	float dx = dot4(bx, t3, t2, t, 1) - x;
	float dy = dot4(by, t3, t2, t, 1) - y;
	return dx * dx + dy * dy;
}

void bisect(
	const coeff *bx, const coeff *by,
	float t0, float t1,
	float x, float y,
	float curveIndex,
	ToveNearest &nearest,
	float eps2) {

	float s = (t1 - t0) * 0.5f;
	float t = t0 + s;

	float bestT = curveIndex + t;
	float bestDistance = distance(bx, by, t, x, y);

	for (int i = 0; i < 16 && bestDistance > eps2; i++) {
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

ToveNearest Subpath::nearest(
	float x, float y, float dmin, float dmax) const {

	ensureCurveData(DIRTY_COEFFICIENTS | DIRTY_CURVE_BOUNDS);
	const int nc = ncurves(nsvg.npts);
	const float eps2 = dmin * dmin;
	const float maxDistance = dmax;

	ToveNearest nearest;
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
				return nearest;
			}
			t0 = t1;
			if (t0 >= 1.0f) {
				break;
			}
		}

		dmax = std::min(dmax, std::sqrt(nearest.distanceSquared));
	}

	if (std::sqrt(nearest.distanceSquared) > maxDistance) {
		nearest.t = -1.0f;
		return nearest;
	} else {
		return nearest;
	}
}

END_TOVE_NAMESPACE
