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

vec2 vertex(int instanceId) {
    int numInstances = num_curves * segments_per_curve;
    instanceId = (instanceId + numInstances) % numInstances;

    int curve = instanceId / segments_per_curve;
    int segment = instanceId % segments_per_curve;

    float curveId = curve / float(max_curves);
    vec4 bx = texture(curves, vec2(0.0f / float(CURVE_DATA_SIZE), curveId));
    vec4 by = texture(curves, vec2(1.0f / float(CURVE_DATA_SIZE), curveId));

    float t = segment / float(segments_per_curve);
    return at(bx, by, t);
}

vec4 position(mat4 transform_projection, vec4 mesh_position) {
    int instanceId = love_InstanceID + int(mesh_position.x);

    vec2 p0 = vertex(instanceId - 1);
    vec2 p1 = vertex(instanceId);
    vec2 p2 = vertex(instanceId + 1);

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
