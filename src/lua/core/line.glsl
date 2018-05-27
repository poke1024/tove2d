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
uniform int segments_per_curve;

uniform float linewidth;
uniform float miter_limit;

vec2 at(vec4 bx, vec4 by, float t) {
    float t2 = t * t;
    vec4 c_t = vec4(t2 * t, t2, t, 1);
    return vec2(dot(c_t, bx), dot(c_t, by));
}

vec2 vertex(float curveTY, float t) {
    vec4 bx = texture(curves, vec2(0.0f / float(CURVE_DATA_SIZE), curveTY));
    vec4 by = texture(curves, vec2(1.0f / float(CURVE_DATA_SIZE), curveTY));
    return at(bx, by, t);
}

vec4 position(mat4 transform_projection, vec4 mesh_position) {
    int instanceId = love_InstanceID + int(mesh_position.x);

    int numInstances = num_curves * segments_per_curve;

    ivec3 instanceIds = (ivec3(instanceId) + ivec3(-1, 0, 1) +
        ivec3(numInstances)) % ivec3(numInstances);

    vec3 curveIds;
    vec3 ts = modf(vec3(instanceIds) / vec3(segments_per_curve), curveIds);

    vec3 curveTYs = curveIds / vec3(max_curves);
    vec2 p0, p1, p2;

    if (curveIds == ivec3(curveIds.x)) {
        vec4 bx = texture(curves, vec2(0.0f / float(CURVE_DATA_SIZE), curveTYs.x));
        vec4 by = texture(curves, vec2(1.0f / float(CURVE_DATA_SIZE), curveTYs.x));

        p0 = at(bx, by, ts.x);
        p1 = at(bx, by, ts.y);
        p2 = at(bx, by, ts.z);
    } else {
        p0 = vertex(curveTYs.x, ts.x);
        p1 = vertex(curveTYs.y, ts.y);
        p2 = vertex(curveTYs.z, ts.z);
    }

    vec2 d10 = normalize(p1 - p0);
    vec2 d21 = normalize(p2 - p1);

    vec2 t = normalize(d10 + d21);

    vec2 m = vec2(-t.y, t.x);
    vec2 n = vec2(-d10.y, d10.x);

    float l = linewidth / dot(m, n);

    // non-bevel (hacky) miter limit.
    l = min(l, miter_limit);

    vec2 q = vec2(p1 + l * m * mesh_position.y);

    return transform_projection * vec4(q, mesh_position.zw);
}
