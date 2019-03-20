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
#include "observer.h"
#include "primitive.h"
#include "gpux/curve_data.h"
#include "nsvg.h"
#include "utils.h"
#include "intersect.h"

BEGIN_TOVE_NAMESPACE

class Subpath : public Observable, public std::enable_shared_from_this<Subpath> {
private:
	struct Command {
		uint8_t type; // ToveCommandType
		bool dirty;
		uint16_t index; // point index
		int8_t direction; // cw vs. ccw

		union {
            LinePrimitive line;
			RectPrimitive rect;
			EllipsePrimitive ellipse;
		};
	};

    enum {
        DIRTY_BOUNDS = 1,
        DIRTY_COMMANDS = 2,
        DIRTY_COEFFICIENTS = 4,
        DIRTY_CURVE_BOUNDS = 8
    };

	mutable std::vector<Command> commands;
    mutable std::vector<CurveData> curves;
	mutable uint8_t dirty;

	float *addPoints(int n, bool allowClosedEdit = false);

	inline void addPoint(float x, float y, bool allowClosedEdit = false) {
		float *p = addPoints(1, allowClosedEdit);
		p[0] = x;
		p[1] = y;
	}

	void setNumPoints(int npts);

	int addCommand(ToveCommandType type, int index);

	inline float *getMutablePoints(int index) const {
		return nsvg.pts + 2 * index;
	}

	void updateCommands() const;

    bool isLoop() const;

    void fixLoop();

    inline void ensureCurveData(uint8_t flags) const {
        if (dirty & flags) {
            updateCurveData(flags);
        }
    }

    void updateCurveData(uint8_t flags) const;

#if TOVE_DEBUG
	std::ostream &dump(std::ostream &os);
#endif

public:
	NSVGpath nsvg;

	Subpath();
	Subpath(const NSVGpath *path);
	Subpath(const SubpathRef &t);

	inline ~Subpath() {
		free(nsvg.pts);
	}

    inline void commit() const {
		if (dirty & DIRTY_COMMANDS) {
			updateCommands();
		}
	}

	int moveTo(float x, float y);
	int lineTo(float x, float y);
	int curveTo(float cpx1, float cpy1, float cpx2, float cpy2, float x, float y);
	int arc(float x, float y, float r, float startAngle, float endAngle, bool counterclockwise);
	int drawRect(float x, float y, float w, float h, float rx, float ry);
	int drawEllipse(float cx, float cy, float rx, float ry);

    int insertCurveAt(float t);
    void removeCurve(int curve);
    void remove(int from, int n);
    int mould(float t, float x, float y);
    void move(int k, float x, float y, ToveHandle handle);

    void makeFlat(int k, int dir);
    void makeSmooth(int k, int dir, float a = 0.5);

	inline float getCurveValue(int curve, int index) const {
        commit();
        const int npts = nsvg.npts;
        const int nc = ncurves(npts);
        if (nc < 1) {
            return 0.0f;
        }
        if (isClosed()) {
            curve = (curve % nc + nc) % nc;
        } else if (curve < 0 || curve >= nc) {
            return 0.0f;
        }
        const int i = curve * 3 * 2 + index + 2;
		if (i >= 0 && i < npts * 2) {
			return nsvg.pts[i];
		} else {
			return 0.0f;
		}
	}

	inline void setCurveValue(int curve, int index, float value) {
        commit();
        const int npts = nsvg.npts;
        const int nc = ncurves(npts);
        if (nc < 1) {
            return;
        }
        if (isClosed()) {
            curve = (curve % nc + nc) % nc;
        } else if (curve < 0 || curve >= nc) {
            return;
        }
        const int i = curve * 3 * 2 + index + 2;
        if (i >= 0 && i < npts * 2) {
			nsvg.pts[i] = value;
			changed(CHANGED_POINTS);
		}
	}

    /* for the point interface to love, we hide the last
       duplicated point on closed curves. */

    inline int getLoveNumPoints() const {
        const int n = getNumPoints();
        return isClosed() && n > 0 ? n - 1 : n;
    }

    float getPointValue(int index, int dim);
    void setPointValue(int index, int dim, float value);

	void setPoints(const float *pts, int npts, bool add_loop = true);

    bool isCollinear(int u, int v, int w) const;
    bool isLineAt(int k, int dir) const;

	inline float getCommandPoint(const Command &command, int what) {
		const float *p = nsvg.pts + 2 * command.index;
		return p[command.direction * (what / 2) * 2 + (what & 1)];
	}

	void setCommandPoint(const Command &command, int what, float value);

	float getCommandValue(int commandIndex, int what);
	void setCommandValue(int commandIndex, int what, float value);
    void setCommandDirty(int commandIndex);

	void updateBounds();

	void set(const SubpathRef &t, const nsvg::Transform &transform);

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

	void setIsClosed(bool closed);

	inline int getNumCurves(bool countClose = true) const {
		const int npts = nsvg.npts;
		int n = ncurves(npts);

		if (countClose && n > 0) {
			// always count an additional "close" curve
            // (might change due to mutable points).
			n += 1;
		}

		return n;
	}

	bool computeShaderCurveData(
		ToveShaderGeometryData *shaderData,
        int curveIndex,
		int target,
		ExCurveData &extended);

	bool animate(const SubpathRef &a, const SubpathRef &b, float t);

	inline void setNext(const SubpathRef &trajectory) {
		nsvg.next = &trajectory->nsvg;
	}

	void updateNSVG();

	void changed(ToveChangeFlags flags);

	void invert();
	void clean(float eps = 0.0);

	ToveOrientation getOrientation() const;
	void setOrientation(ToveOrientation orientation);

    void testInside(float x, float y, AbstractInsideTest &test) const;
    void intersect(const AbstractRay &ray, Intersecter &intersecter) const;

    ToveVec2 getPosition(float globalt) const;
    ToveVec2 getNormal(float globalt) const;

    ToveNearest nearest(float x, float y, float dmin, float dmax) const;
};

END_TOVE_NAMESPACE

#endif // __TOVE_TRAJECTORY
