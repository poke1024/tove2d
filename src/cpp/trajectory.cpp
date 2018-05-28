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
		switch (command.type) {
			case TOVE_DRAW_RECT: {
				float *p = getMutablePoints(command.index);
				command.rect.normalize();
				assert(nsvg.npts - command.index >= command.rect.size());
				command.rect.write(p);
			} break;
			case TOVE_DRAW_ELLIPSE: {
				float *p = getMutablePoints(command.index);
				assert(nsvg.npts - command.index >= command.ellipse.size());
				command.ellipse.write(p);
			} break;
		}
		command.dirty = false;
	}
	commandsDirty = false;
}

Trajectory::Trajectory() {
	memset(&nsvg, 0, sizeof(nsvg));
	nsvg.closed = 0;

	for (int i = 0; i < 4; i++) {
		nsvg.bounds[i] = 0.0;
	}
	boundsDirty = true;

	commandsDirty = false;
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
	boundsDirty = false;
	commandsDirty = false;
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
	boundsDirty = t->boundsDirty;
	commandsDirty = false;
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
	NSVGpath *p = &nsvg;
	const int n = p->npts;
	if (n > 0) {
		float px = p->pts[(n - 1) * 2 + 0];
		float py = p->pts[(n - 1) * 2 + 1];
		float dx = x - px;
		float dy = y - py;
		float *q = addPoints(3);
		q[0] = px + dx/3.0f; q[1] = py + dy/3.0f;
		q[2] = x - dx/3.0f; q[3] = y - dy/3.0f;
		q[4] = x; q[5] = y;
		return addCommand(TOVE_LINE_TO, n - 1);
	} else {
		return -1;
	}
}

int Trajectory::curveTo(float cpx1, float cpy1, float cpx2, float cpy2, float x, float y) {
	const int index = nsvg.npts;
	float *q = addPoints(3);
	q[0] = cpx1; q[1] = cpy1;
	q[2] = cpx2; q[3] = cpy2;
	q[4] = x; q[5] = y;
	return addCommand(TOVE_CURVE_TO, index);
}

inline float wrapAngle(float phi) {
	return phi - 360 * std::floor(phi / 360.0);
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
	const Command &command = commands[commandIndex];
	switch (command.type) {
		case TOVE_MOVE_TO: {
			if (what >= 4 && what <= 5) {
				return getCommandPoint(command, what - 4);
			}
		} break;
		case TOVE_LINE_TO: {
			if (what >= 4 && what <= 5) {
				return getCommandPoint(command, what + 2);
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
			if (what >= 4 && what <= 5) {
				assert(command.index >= 0);
				const float px = getCommandPoint(command, what - 4);
				const float dx = value - px;
				setCommandPoint(command, what - 2, px + dx / 3.0f);
				setCommandPoint(command, what, value - dx / 3.0f);
				setCommandPoint(command, what + 2, value);
				changed(CHANGED_POINTS);
				refreshCommand(commandIndex + 1);
			}
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
			commandsDirty = true;
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
			commandsDirty = true;
			changed(CHANGED_POINTS);
		} break;
	}
}

void Trajectory::refreshCommand(int commandIndex) {
	const int n = commands.size();
	if (isClosed()) {
		commandIndex = (commandIndex % n + n) % n;
	}
	if (commandIndex < 0 ||commandIndex >= n) {
		return;
	}
	Command &command = commands[commandIndex];
	switch (command.type) {
		case TOVE_LINE_TO: {
			assert(command.index >= 0);
			for (int d = 0; d < 2; d++) {
				float value = getCommandPoint(command, 6 + d);
				const float px = getCommandPoint(command, 0 + d);
				const float dx = value - px;
				setCommandPoint(command, 2 + d, px + dx / 3.0f);
				setCommandPoint(command, 4 + d, value - dx / 3.0f);
			}
			changed(CHANGED_POINTS);
		} break;
	}
}

void Trajectory::updateBounds() {
	if (!boundsDirty) {
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
	boundsDirty = false;
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

bool Trajectory::computeShaderCurveData(
	ToveShaderGeometryData *shaderData,
	const float *pts,
	int target,
	ExtendedCurveData &extended) {

	commit();

	uint16_t *curveData = reinterpret_cast<uint16_t*>(shaderData->curvesTexture) +
		shaderData->curvesTextureRowBytes * target / sizeof(uint16_t);

	float bx[4];
	float by[4];

	coeffs(pts[0], pts[2], pts[4], pts[6], bx);
	coeffs(pts[1], pts[3], pts[5], pts[7], by);

	if (fabs(bx[0]) < 1e-2 && fabs(bx[1]) < 1e-2 && fabs(bx[2]) < 1e-2 &&
		fabs(by[0]) < 1e-2 && fabs(by[1]) < 1e-2 && fabs(by[2]) < 1e-2) {
		extended.ignore = IGNORE_FILL | IGNORE_LINE;
		return false;
	}

#if 0
	printf("%d (%f, %f) (%f, %f) (%f, %f) (%f, %f)\n", target,
		pts[0], pts[1],
		pts[2], pts[3],
		pts[4], pts[5],
		pts[6], pts[7]);
#endif

	extended.set(pts, bx, by);

	// write curve data.
	for (int i = 0; i < 4; i++) {
		store_fp16(curveData[i], bx[i]);
		store_fp16(curveData[i + 4], by[i]);
	}

	uint16_t *data = reinterpret_cast<uint16_t*>(&curveData[8]);

	for (int i = 0; i < 4; i++) {
		store_fp16(data[i], extended.bounds[i]);
	}
	data += 4;

	if (shaderData->fragmentShaderStrokes) {
		extended.copyRoots(data);
		data += 4;
	}

	assert(data - curveData <= 4 * shaderData->curvesTextureSize[0]);
	extended.ignore = 0;

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
	boundsDirty = true;
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
