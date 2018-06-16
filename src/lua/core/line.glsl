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

uniform sampler2D curves;
uniform int max_curves;
uniform int num_curves;
uniform int curve_index;
uniform int segments_per_curve;
uniform int is_closed;

uniform vec2 lineargs;
#define LINE_OFFSET lineargs.x
#define MITER_LIMIT lineargs.y

varying vec2 raw_vertex_pos;

vec2 at(vec4 bx, vec4 by, float t) {
    float t2 = t * t;
    vec4 c_t = vec4(t2 * t, t2, t, 1);
    return vec2(dot(c_t, bx), dot(c_t, by));
}

vec2 vertex(float curveTY, float t) {
    vec4 bx = Texel(curves, vec2(0.0f / float(CURVE_DATA_SIZE), curveTY));
    vec4 by = Texel(curves, vec2(1.0f / float(CURVE_DATA_SIZE), curveTY));
    return at(bx, by, t);
}

vec4 position(mat4 transform_projection, vec4 mesh_position) {
    int mx = int(mesh_position.x);

    int instanceBaseId = curve_index + love_InstanceID;
    int instanceId = instanceBaseId + mx / 2;

    int numInstances = num_curves * segments_per_curve;
    ivec3 instanceIds = ivec3(instanceId) + ivec3(-1, 0, 1);

    if (is_closed != 0) {
        instanceIds = (instanceIds + ivec3(numInstances)) % ivec3(numInstances);
    } else {
        instanceIds = clamp(instanceIds, ivec3(0), ivec3(numInstances));
    }

    vec3 curveIds;
    vec3 ts = modf(vec3(instanceIds) / vec3(segments_per_curve), curveIds);

#ifdef GLSL3
    vec3 maxCurveIds = vec3(num_curves - 1);
    bvec3 beyondEnd = greaterThan(curveIds, maxCurveIds);
    curveIds = mix(curveIds, maxCurveIds, beyondEnd);
    ts = mix(ts, vec3(1.0f), beyondEnd);
#else
    if (curveIds.y >= num_curves) {
        curveIds.y = num_curves - 1;
        ts.y = 1.0f;
    }
    if (curveIds.z >= num_curves) {
        curveIds.z = num_curves - 1;
        ts.z = 1.0f;
    }
#endif

    vec3 curveTYs = curveIds / vec3(max_curves);
    vec2 p0, p1, p2;

    if (curveIds == ivec3(curveIds.x)) {
        vec4 bx = Texel(curves,
            vec2(0.0f / float(CURVE_DATA_SIZE), curveTYs.x));
        vec4 by = Texel(curves,
            vec2(1.0f / float(CURVE_DATA_SIZE), curveTYs.x));

        p0 = at(bx, by, ts.x);
        p1 = at(bx, by, ts.y);
        p2 = at(bx, by, ts.z);
    } else {
        p0 = vertex(curveTYs.x, ts.x);
        p1 = vertex(curveTYs.y, ts.y);
        p2 = vertex(curveTYs.z, ts.z);
    }

    ivec2 c = (ivec2(instanceBaseId) + ivec2(-1, 1)) /
        ivec2(segments_per_curve);
    bool use_bevel = false;

    if (is_closed != 0) {
        c = (c + ivec2(num_curves)) % ivec2(num_curves);
    } else {
        c = clamp(c, ivec2(0), ivec2(num_curves - 1));
    }

    if (c.x != c.y) { // check miter limit for bevel case
        vec2 pp0, pp1, pp2;
        vec2 cTY = vec2(c) / vec2(max_curves);

#if 0
        // slower but much simpler version.
        pp0 = vertex(cTY.x, 1.0f - 1.0f / segments_per_curve);
        pp1 = vertex(cTY.y, 0.0f);
        pp2 = vertex(cTY.y, 1.0f / segments_per_curve);
#else
        if (mx / 2 == 0) {
            if (curveIds.x != curveIds.y) { // i.e. ts.y == 0.0f
                pp0 = p0;
                pp1 = p1;
                pp2 = p2;
            } else { // i.e. ts.z == 0.0f
                pp0 = p1;
                pp1 = p2;
                pp2 = vertex(curveTYs.z, 1.0f / segments_per_curve);
            }
        } else {
            if (curveIds == ivec3(curveIds.x)) { // i.e. ts.x == 0.0f
                pp0 = vertex(cTY.x, 1.0f - 1.0f / segments_per_curve);
                pp1 = p0;
                pp2 = p1;
            } else { // i.e. ts.y == 0.0f
                pp0 = p0;
                pp1 = p1;
                pp2 = p2;
            }
        }
#endif

        vec2 dd10 = normalize(pp1 - pp0);
        vec2 dd21 = normalize(pp2 - pp1);

        vec2 dm = (dd10 + dd21) * 0.5f;
        use_bevel = dot(dm, dm) * MITER_LIMIT * MITER_LIMIT < 1.0f;
    }

    vec2 d10 = p1 - p0;
    vec2 d21 = p2 - p1;

    d10 = normalize(dot(d10, d10) > 1e-5 ? d10 : d21);
    d21 = normalize(dot(d21, d21) > 1e-5 ? d21 : d10);

    vec2 n10 = vec2(d10.y, -d10.x);

    vec2 q;
    if (use_bevel) {
        vec2 n21 = vec2(d21.y, -d21.x);
        q = p1 + (mx % 2 == 0 ? n10 : n21) * LINE_OFFSET * mesh_position.y;

    } else {
        vec2 t = normalize(d10 + d21);
        vec2 m = vec2(-t.y, t.x);
        float l = LINE_OFFSET / dot(m, n10);
        q = vec2(p1 - l * m * mesh_position.y);
    }

    raw_vertex_pos = q;
    return transform_projection * vec4(q, mesh_position.zw);
}
