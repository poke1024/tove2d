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

// This is TÖVE's vertex line shader, which works by drawing instances
// of line segments as meshes.

uniform sampler2D curves;
uniform int max_curves;
uniform int num_curves;
uniform int draw_mode;
uniform int curve_index;
uniform int segments_per_curve;

uniform vec2 lineargs;
#define LINE_OFFSET lineargs.x
#define MITER_LIMIT lineargs.y

// varying vec2 raw_vertex_pos;

vec2 at(vec4 bx, vec4 by, float t) {
    float t2 = t * t;
    vec4 c_t = vec4(t2 * t, t2, t, 1);
    return vec2(dot(c_t, bx), dot(c_t, by));
}

/*vec2 vertex(float curveTY, float t) {
    vec4 bx = Texel(curves, vec2(0.0f / float(CURVE_DATA_SIZE), curveTY));
    vec4 by = Texel(curves, vec2(1.0f / float(CURVE_DATA_SIZE), curveTY));
    return at(bx, by, t);
}*/

vec2 normal(vec4 bx, vec4 by, float t) {
	vec3 c_dt = vec3(3 * t * t, 2 * t, 1);
	vec2 tangent = vec2(dot(bx.xyz, c_dt), dot(by.xyz, c_dt));
	if (dot(tangent, tangent) < 1e-2) {
		tangent = at(bx, by, t + 1e-2) - at(bx, by, t - 1e-2);
	}
	return normalize(vec2(-tangent.y, tangent.x));
}

vec2 drawjoin(vec4 mesh_position) {
    int curve0 = love_InstanceID;
    int curve1 = (curve0 + 1) % num_curves;
    vec2 ty = (vec2(curve0, curve1) + vec2(curve_index)) / vec2(max_curves);

    vec4 bx0 = Texel(curves,
        vec2(0.0f / float(CURVE_DATA_SIZE), ty.x));
    vec4 by0 = Texel(curves,
        vec2(1.0f / float(CURVE_DATA_SIZE), ty.x));
    vec2 n10 = normal(bx0, by0, 1.0f);

    vec4 bx1 = Texel(curves,
        vec2(0.0f / float(CURVE_DATA_SIZE), ty.y));
    vec4 by1 = Texel(curves,
        vec2(1.0f / float(CURVE_DATA_SIZE), ty.y));
    vec2 n21 = normal(bx1, by1, 0.0f);

    float wind = sign(-n21.y * n10.x - n21.x * -n10.y);

    if (mesh_position.x != 0.0f) {
        bool up = mesh_position.y < 0.0f;

        vec2 n = up ? n10 : n21;
        vec4 bx = up ? bx0 : bx1;
        vec4 by = up ? by0 : by1;
        float t = up ? 1.0f : 0.0f;

        return at(bx, by, t) + wind * LINE_OFFSET * n * mesh_position.x;
    } else {
        vec2 m = (n10 + n21) * 0.5f;
        if (dot(m, m) * MITER_LIMIT * MITER_LIMIT < 1.0f) { // use bevel?
            // emit a duplicate point, effectively removing the miter.
            return at(bx0, by0, 1.0f) + wind * LINE_OFFSET * n10;
        } else {
            float l = LINE_OFFSET / dot(m, n10);
            return at(bx1, by1, 0.0f) + l * m * wind;
        }
    }
}

vec2 drawline(vec4 mesh_position) {
    int instanceId = love_InstanceID;

    int curve = curve_index + instanceId / segments_per_curve;
    int tpos0 = instanceId % segments_per_curve;
    float tpos1 = tpos0 + mesh_position.x;
    float t = tpos1 / float(segments_per_curve);
    float curveTY = curve / float(max_curves);

    vec4 bx = Texel(curves,
        vec2(0.0f / float(CURVE_DATA_SIZE), curveTY));
    vec4 by = Texel(curves,
        vec2(1.0f / float(CURVE_DATA_SIZE), curveTY));

    vec2 n = normal(bx, by, t);
    vec2 p = at(bx, by, t);

    return p - LINE_OFFSET * n * mesh_position.y;
}

vec4 do_vertex(vec4 mesh_position) {
    vec2 q;

    switch (draw_mode) {
        case 0:
            q = drawline(mesh_position);
            break;
        case 1:
            q = drawjoin(mesh_position);
            break;
        default:
            q = mesh_position.xy;
            break;
    }

    raw_vertex_pos = q;
    return vec4(q, mesh_position.zw);
}
