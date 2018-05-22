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

 #ifndef __TOVE_TRAJECTORY
 #define __TOVE_TRAJECTORY 1

#include "common.h"
#include "claimable.h"
#include "primitive.h"
#include "shader/curvedata.h"
#include "nsvg.h"
#include "utils.h"

class Trajectory : public Claimable<Path>, public std::enable_shared_from_this<Trajectory> {
private:
	struct Command {
		uint8_t type; // ToveCommandType
		bool dirty;
		uint16_t index; // point index
		int8_t direction; // cw vs. ccw

		union {
			RectPrimitive rect;
			EllipsePrimitive ellipse;
		};
	};

	mutable std::vector<Command> commands;
	mutable bool commandsDirty;
	bool boundsDirty;

	float *addPoints(int n);

	inline void addPoint(float x, float y) {
		float *p = addPoints(1);
		p[0] = x;
		p[1] = y;
	}

	void setNumPoints(int npts);

	int addCommand(ToveCommandType type, int index);

	inline float *getMutablePoints(int index) const {
		return nsvg.pts + 2 * index;
	}

	inline void commit() const {
		if (commandsDirty) {
			updateCommands();
		}
	}

	void updateCommands() const;

public:
	NSVGpath nsvg;

	Trajectory();
	Trajectory(const NSVGpath *path);
	Trajectory(const TrajectoryRef &t);

	inline ~Trajectory() {
		free(nsvg.pts);
	}

	int moveTo(float x, float y);
	int lineTo(float x, float y);
	int curveTo(float cpx1, float cpy1, float cpx2, float cpy2, float x, float y);
	int arc(float x, float y, float r, float startAngle, float endAngle, bool counterclockwise);
	int drawRect(float x, float y, float w, float h, float rx, float ry);
	int drawEllipse(float cx, float cy, float rx, float ry);

	inline float getValue(int index) const {
		if (index >= 0 && index < nsvg.npts * 2) {
			commit();
			return nsvg.pts[index];
		} else {
			return 0.0;
		}
	}

	inline void setValue(int index, float value) {
		if (index >= 0 && index < nsvg.npts * 2) {
			commit();
			nsvg.pts[index] = value;
			changed(CHANGED_POINTS);
		}
	}

	inline void setPoint(int index, float x, float y) {
		if (index >= 0 && index < nsvg.npts) {
			commit();
			float *p = &nsvg.pts[index * 2 + 0];
			p[0] = x;
			p[1] = y;
			changed(CHANGED_POINTS);
		}
	}

	void setPoints(const float *pts, int npts);

	inline float getCommandPoint(const Command &command, int what) {
		const float *p = nsvg.pts + 2 * command.index;
		return p[command.direction * (what / 2) * 2 + (what & 1)];
	}

	inline void setCommandPoint(const Command &command, int what, float value) {
		float *p = nsvg.pts + 2 * command.index;
		p[command.direction * (what / 2) * 2 + (what & 1)] = value;
	}

	float getCommandValue(int commandIndex, int what);
	void setCommandValue(int commandIndex, int what, float value);

	void updateBounds();

	void transform(float sx, float sy, float tx, float ty);

	inline int getNumPoints() const {
		return nsvg.npts;
	}

	inline const float *getPoints() const {
		commit();
		return nsvg.pts;
	}

	inline bool isClosed() const {
		return nsvg.closed;
	}

	inline void setIsClosed(bool closed) {
		nsvg.closed = closed;
	}

	/*bool startEqualsEnd() const {
		commit();
		const int n = nsvg.npts;
		return n > 0 && nsvg.pts[0] == nsvg.pts[n * 2 - 2] && nsvg.pts[1] == nsvg.pts[n * 2 - 1];
	}*/

	inline int getNumCurves(bool countClose = true) const {
		const int npts = nsvg.npts;
		int n = ncurves(npts);

		if (countClose && n > 0) {
			// always count an additional "close" curve (might change due to mutable points).
			n += 1;
		}

		return n;
	}

	void computeShaderCloseCurveData(
		ToveShaderGeometryData *shaderData,
		int target,
		ExtendedCurveData &extended);

	inline void computeShaderCurveData(
		ToveShaderGeometryData *shaderData,
		int curveIndex,
		int target,
		ExtendedCurveData &extended) {

		computeShaderCurveData(
			shaderData, &nsvg.pts[curveIndex * 3 * 2], target, extended);
	}

	void computeShaderCurveData(
		ToveShaderGeometryData *shaderData,
		const float *pts,
		int target,
		ExtendedCurveData &extended);

	void animate(const TrajectoryRef &a, const TrajectoryRef &b, float t);

	inline void setNext(const TrajectoryRef &trajectory) {
		nsvg.next = &trajectory->nsvg;
	}

	void updateNSVG();

	inline int fetchChanges() {
		// always emits a change; could be optimized. currently
		// only used in the curves renderer GeometryShaderLinkImpl.
		return CHANGED_POINTS;
	}

	void changed(ToveChangeFlags flags);

	inline void clearChanges(ToveChangeFlags flags) {
	}

	void invert();
	void clean(float eps = 0.0);
	ToveOrientation getOrientation() const;
	void setOrientation(ToveOrientation orientation);
};

#endif // __TOVE_TRAJECTORY
