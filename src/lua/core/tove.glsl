// *****************************************************************
// TÖVE - Animated vector graphics for LÖVE.
// https://github.com/poke1024/tove2d
//
// Copyright (c) 2018, Bernhard Liebl
//
// Distributed under the MIT license. See LICENSE file for details.
//
// All rights reserved.
// *****************************************************************
//
// shader implementation for tove2d vector graphics rendering.
// supports cubic bezier curves and fill rules.
// stroke rendering is supported but inefficient.
//
// Credits:
//
// the basis of the cubic root calculation was worked out by:
// https://www.particleincell.com/2013/cubic-line-intersection/
//
// segmentPointDistanceSquared() is adapted from:
// https://stackoverflow.com/questions/849211/shortest-distance-between-a-point-and-a-line-segment

varying vec4 raw_vertex_pos;

#define T_EPS 0.0
#define SENTINEL_END 1.0
#define SENTINEL_STROKES (254.0 / 255.0)
#define MAX_LINE_ITERATIONS 14

#define LUT_LOG_N tablemeta.z

#define M_PI 3.1415926535897932384626433832795

uniform ivec3 constants;
#define NUM_CURVES constants.x
#define LISTS_W constants.y
#define LISTS_H constants.z

uniform ivec3 tablemeta;
uniform float lut[2 * LUT_SIZE];

uniform sampler2D lists;
uniform sampler2D curves;

#if LINE_STYLE > 0
uniform float linewidth; // half the linewidth
#endif

#if LINE_STYLE > 0
vec2 eval(vec4 bx, vec4 by, float t) {
	float t2 = t * t;
	vec4 c = vec4(t2 * t, t2, t, 1);
	return vec2(dot(c, bx), dot(c, by));
}

vec2 eval0(vec4 bx, vec4 by) {
	return vec2(bx.w, by.w);
}

vec2 eval1(vec4 bx, vec4 by) {
	return vec2(dot(vec4(1), bx), dot(vec4(1), by));
}

float distanceSquared(vec2 a, vec2 b) {
	vec2 c = a - b;
	return dot(c, c);
}

float segmentPointDistanceSquared(vec2 v, vec2 w, vec2 p) {
	// Return minimum distance between line segment vw and point p
	float l2 = distanceSquared(v, w);  // i.e. |w-v|^2 -  avoid a sqrt
	// Consider the line extending the segment, parameterized as v + t (w - v).
	// We find projection of point p onto the line.
	// It falls where t = [(p-v) . (w-v)] / |w-v|^2
	// We clamp t from [0,1] to handle points outside the segment vw.
	float t = clamp(dot(p - v, w - v) / l2, 0.0f, 1.0f);
	vec2 projection = v + t * (w - v);  // Projection falls on the segment
	return distanceSquared(p, mix(v, projection, step(1e-2, l2)));
}

vec2 curveTangent(vec4 bx, vec4 by, float t) {
	vec3 c_dt = vec3(3 * t * t, 2 * t, 1);
	vec2 tangent = vec2(dot(bx.xyz, c_dt), dot(by.xyz, c_dt));
	if (dot(tangent, tangent) < 1e-2) {
		// ugly, but necessary case; e.g. see "hearts" shape from demos
		tangent = eval(bx, by, t + 1e-2) - eval(bx, by, t - 1e-2);
	}
	return tangent;
}

float curveLineDistanceSquared(vec2 pos, vec4 bx, vec4 by, vec2 tr, float tolerance) {
	float s = 0.5 * (tr.y - tr.x);
	float t = tr.x + s;
	s *= 0.5;

	vec2 curvePoint = eval(bx, by, t);
	vec4 state = vec4(t, distanceSquared(curvePoint, pos), s, -s);

	for (int i = 0; i < MAX_LINE_ITERATIONS && state.y > tolerance; i++) {
		float tn = state.x + state.z;
		curvePoint = eval(bx, by, tn);
		float d1 = distanceSquared(curvePoint, pos);
		float s1 = state.z * 0.5;
		state = d1 < state.y ?
			vec4(tn, d1, s1, -s1) :
			vec4(state.xyw, s1 * 0.5);
	}

	vec2 w = normalize(curveTangent(bx, by, state.x)) * linewidth;
	return segmentPointDistanceSquared(curvePoint - w, curvePoint + w, pos);
}

bool onCurveLine(vec2 pos, vec4 bx, vec4 by, vec4 roots, float tolerance) {
	float t0 = 0.0;
	do {
		float t1 = roots.x;
		if (curveLineDistanceSquared(
			pos, bx, by, vec2(t0, t1), tolerance) <= tolerance) {
			return true;
		}
		roots = vec4(roots.yzw, 1.0);
		t0 = t1;
	} while (t0 < 1.0);

	return false;
}
#endif

vec3 goodT(vec3 t) {
	return step(vec3(0.0 - T_EPS), t) - step(vec3(1.0 + T_EPS), t);
}

#if FILL_RULE == 0
float clockwise(vec4 by, float t, float t2) {
	vec3 c_dt = vec3(3 * t2, 2 * t, 1);
	return sign(dot(by.xyz, c_dt));
}

int contribute(vec4 bx, vec4 by, float t, float x) {
	float t2 = t * t;
	bool ok = clamp(t, 0.0 - T_EPS, 1.0 + T_EPS) == t &&
		dot(bx, vec4(t * t2, t2, t, 1)) > x;
	return int(clockwise(by, t, t2)) * int(ok);
}

int contribute3(vec4 bx, vec4 by, vec3 t, float x) {
	vec3 t2 = t * t;
	vec3 hit = goodT(t) * step(
		vec3(x), bx.x * t * t2 + bx.y * t2 + bx.z * t + bx.w);
	vec3 cw = vec3(clockwise(by, t.x, t2.x),
		clockwise(by, t.y, t2.y),
		clockwise(by, t.z, t2.z));
	return int(dot(vec3(1), cw * hit));
}

#elif FILL_RULE == 1
int contribute(vec4 bx, vec4 by, float t, float x) {
	float t2 = t * t;
	return int(clamp(t, 0.0 - T_EPS, 1.0 + T_EPS) == t &&
		dot(bx, vec4(t * t2, t2, t, 1)) > x);
}

int contribute3(vec4 bx, vec4 by, vec3 t, float x) {
	vec3 t2 = t * t;
	vec3 hit = goodT(t) * step(
		vec3(x), bx.x * t * t2 + bx.y * t2 + bx.z * t + bx.w);
	return int(dot(hit, vec3(1)));
}
#endif

#if LINE_STYLE > 0
bool checkLine(float curveId, vec2 position, vec2 axis1) {
	vec4 bounds = texture(curves, vec2(2.0 / CURVE_DATA_SIZE, curveId));
	vec2 off = clamp(position, bounds.xy, bounds.zw) - position;
	if (abs(dot(off, axis1)) < linewidth) {
		vec4 bx = texture(curves, vec2(0.0 / CURVE_DATA_SIZE, curveId));
		vec4 by = texture(curves, vec2(1.0 / CURVE_DATA_SIZE, curveId));
		vec4 roots = texture(curves, vec2(3.0 / CURVE_DATA_SIZE, curveId));
		return onCurveLine(position, bx, by, roots, linewidth * linewidth);
	} else {
		return false;
	}
}
#endif

int advancedMagic(vec4 bx, vec4 by, vec4 P, float position, float upperBound) {
#if FILL_STYLE > 0
	if (position > upperBound) {
		return 0;
	}

	vec3 ABC = P.yzw / P.xxx;
	// note: ABC.x might be nan due to P.y == 0.

	if (abs(P.x) < 1e-2 || abs(ABC.x) > 1000.0) {
		// not cubic in y; or cubic, but so large, that our numeric
		// calculations would go havoc due to cubic powers on A.
		if (abs(P.y) < 1e-2) {
			// linear.
			float a = P.z;
			float b = P.w;

			return contribute(bx, by, -b / a, position);
		} else {
			// 	quadratic.
			float a = P.y;
			float b = P.z;
			float c = P.w;

			float D = sqrt(b * b - 4 * a * c);
			float n = 2 * a;
			int z = contribute(bx, by, (-b + D) / n, position);
			if (D > 0.0) {
				z += contribute(bx, by, (-b - D) / n, position);
			}
			return z;
		}
	} else {
		// cubic.
		float A = ABC.x;
		float B = ABC.y;
		float C = ABC.z;

		float A2 = A * A;
		float Q = (3.0 * B - A2) / 9.0;
		float R = (9.0 * A * B - 27.0 * C - 2 * A2 * A) / 54.0;

		float A_third = A / 3.0;
		float Q3 = Q * Q * Q;
		float D = Q3 + R * R;

		if (D > 0.0) { // we have complex or duplicate roots
			float sqrtD = sqrt(D);
			vec2 r = vec2(R + sqrtD, R - sqrtD);
			vec2 ST = sign(r) * pow(r, vec2(1.0 / 3.0));
			float st = ST.x + ST.y;

			if (abs(ST.x - ST.y) < 1e-2) { // complex roots?
				// real root:
				int z = contribute(bx, by, -A_third + st, position);
				// real part of complex root:
				z += contribute(bx, by, -A_third - st / 2, position) * 2;
				return z;
			} else {
				// real root:
				return contribute(bx, by, -A_third + st, position);
			}
		} else {
			vec2 tmp = sqrt(vec2(-Q3, -Q));
			vec3 phi = vec3(acos(R / tmp.x)) + vec3(0, 2 * M_PI, 4 * M_PI);
			vec3 t = 2 * tmp.y * cos(phi / 3.0) - A_third;
			return contribute3(bx, by, t, position);
		}
	}
#else
	return 0;
#endif
}

bool isInside(int z) {
#if FILL_RULE == 0
	return z != 0;
#elif FILL_RULE == 1
	return mod(z, 2) > 0.5;
#endif
}

#define X_LO lohi.x
#define X_HI lohi.y
#define X_LOHI lohi.xy
#define Y_LO lohi.z
#define Y_HI lohi.w
#define Y_LOHI lohi.zw
#define XY_LO lohi.xz
#define XY_HI lohi.yw

#define X_MID mid.x
#define Y_MID mid.y

ivec4 bsearch(ivec4 lohi, vec2 position) {
    ivec2 mid = (XY_LO + XY_HI) / 2;
	ivec2 lutIndex = mid * 2;

    bvec2 notDone = lessThan(XY_LO, XY_HI);
    bvec2 smaller = lessThan(
		vec2(lut[lutIndex.x], lut[lutIndex.y + 1]), position);

	ivec4 nextLo = ivec4(mid + ivec2(1), XY_HI);

#if 0
	// glsl2 version of the glsl3 code below.
	return ivec4(
		notDone.x ? (smaller.x ? nextLo.xz : ivec2(X_LO, X_MID)) : X_LOHI,
        notDone.y ? (smaller.y ? nextLo.yw : ivec2(Y_LO, Y_MID)) : Y_LOHI);
#endif

	return ivec4(mix(
		lohi.xyzw,
		mix(ivec4(X_LO, X_MID, Y_LO, Y_MID), nextLo.xzyw, smaller.xxyy),
		notDone.xxyy));
}

ivec2 search(vec2 position) {
	ivec4 lohi = ivec4(0, tablemeta.x, 0, tablemeta.y);
	for (int i = 0; i < LUT_LOG_N; i++) {
		lohi = bsearch(lohi, position);

	}
	return ivec2(min(lohi.xz, tablemeta.xy - ivec2(1)));
}

vec2 orient(ivec2 at, vec2 position, out bool rayOnX) {
	ivec2 index = 2 * (at - ivec2(1));

	float x0 = lut[index.x + 0];
	float x1 = lut[index.x + 2];
	float y0 = lut[index.y + 1];
	float y1 = lut[index.y + 3];

	vec2 d0 = position - vec2(x0, y0);
	vec2 d1 = vec2(x1, y1) - position;

	vec2 dxy = min(d0, d1);
	rayOnX = dxy.x < dxy.y;

	// reduce remaining numerical inaccuracies.
	vec2 p0 = vec2(x0, y0);
	vec2 p1 = vec2(x1, y1);
	vec2 center = (p0 + p1) / 2;
	vec2 margin = vec2(0.5);
	vec2 npos = mix(min(p0 + margin, center), position, step(margin, d0));
	npos = mix(max(center, p1 - margin), npos, step(margin, d1));
	return rayOnX ? vec2(position.x, npos.y) : vec2(npos.x, position.y);
}

vec4 effect(vec4 _1, Image _2, vec2 _3, vec2 _4) {
	vec2 position = raw_vertex_pos.xy;
	ivec2 at = search(position);

	bool rayOnX;
	position = orient(at, position, rayOnX);

	vec2 axis1 = vec2(int(rayOnX), 1 - int(rayOnX));
	vec2 axis2 = vec2(1) - axis1;

	float rayPos = dot(position, axis1);
	vec4 C = vec4(0.0, 0.0, 0.0, dot(position, axis2));

	int z = 0;

	vec2 blockpos = rayOnX ?
		vec2(0, (at.y - 1 + LISTS_H / 2) / float(LISTS_H)) :
		vec2(0, (at.x - 1) / float(LISTS_H));
	vec2 blockadv = vec2(1.0 / LISTS_W, 0);

	float C_ID_SCALE = 255.0 / NUM_CURVES;
	vec4 curveIds = texture(lists, blockpos) * C_ID_SCALE;
	int k = 0;

	while (curveIds.x < SENTINEL_STROKES * C_ID_SCALE) {
		float curveId = curveIds.x;
		vec4 bx = texture(curves, vec2(0.0 / CURVE_DATA_SIZE, curveId));
		vec4 by = texture(curves, vec2(1.0 / CURVE_DATA_SIZE, curveId));
		vec4 bounds = texture(curves, vec2(2.0 / CURVE_DATA_SIZE, curveId));

#if LINE_STYLE > 0
		vec2 off = clamp(position, bounds.xy, bounds.zw) - position;

		if (abs(dot(off, axis1)) < linewidth) {
			vec4 roots = texture(curves, vec2(3.0 / CURVE_DATA_SIZE, curveId));
			if (onCurveLine(position, bx, by, roots, linewidth * linewidth)) {
				return computeLineColor(position);
			}
		}

		if (dot(off, axis2) == 0.0) {
#endif

			z += advancedMagic(
				rayOnX ? bx : by,
				rayOnX ? by : bx,
				rayOnX ? -by + C : bx - C,
				rayPos,
				dot(bounds.zw, axis1));

#if LINE_STYLE > 0
		}
#endif

		if (++k < 4) {
			curveIds.xyzw = curveIds.yzwx;
		} else {
			blockpos += blockadv;
			if (blockpos.x >= 1.0) {
				// something went horribly wrong.
				return vec4(0.33, 1, 1, 1);
			}
			curveIds = texture(lists, blockpos) * C_ID_SCALE;
			k = 0;
		}
	}

#if LINE_STYLE > 0
	if (curveIds.x == SENTINEL_STROKES * C_ID_SCALE) {
		if (++k < 4) {
			curveIds.xyzw = curveIds.yzwx;
		} else {
			blockpos += blockadv;
			curveIds = texture(lists, blockpos) * C_ID_SCALE;
			k = 0;
		}
		while (curveIds.x < SENTINEL_END * C_ID_SCALE) {
			if (checkLine(curveIds.x, position, axis1)) {
				return computeLineColor(position);
			}

			if (++k < 4) {
				curveIds.xyzw = curveIds.yzwx;
			} else {
				blockpos += blockadv;
				if (blockpos.x >= 1.0) {
					// something went horribly wrong.
					return vec4(0.33, 1, 1, 1);
				}
				curveIds = texture(lists, blockpos) * C_ID_SCALE;
				k = 0;
			}
		}
	}
#endif

#if FILL_STYLE > 0
	if (isInside(z)) {
		return computeFillColor(position);
	} else {
		discard;
	}
#else
	discard;
#endif
}
