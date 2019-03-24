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

#include "../common.h"
#include "gen.h"

BEGIN_TOVE_NAMESPACE

thread_local std::string ShaderWriter::sSource;
ToveShaderLanguage ShaderWriter::sLanguage = TOVE_GLSL2;
int ShaderWriter::sMatrixRows = 2;

void ShaderWriter::configure(ToveShaderLanguage language, int matrixRows) {
	sLanguage = language;
	sMatrixRows = matrixRows;
}

void ShaderWriter::define(const std::string &key, const std::string &value) {
	defines[key] = value;
}

void ShaderWriter::define(const std::string &key, int value) {
	define(key, std::to_string(value));
}

ShaderWriter::ShaderWriter() {
#if TOVE_TARGET == TOVE_TARGET_LOVE2D
	if (sLanguage == TOVE_GLSL3) {
		out << "#pragma language glsl3\n#define GLSL3 1\n";
	}
#else
	out << R"GLSL(
shader_type canvas_item;
render_mode skip_vertex_transform;
)GLSL";
#endif

	switch (sMatrixRows) {
		case 2:
			define("MATRIX", "mat3x2");
			break;
		case 3:
			define("MATRIX", "mat3");
			break;
		case 4:
			define("MATRIX", "mat3x4");
			break;
	}

#if TOVE_TARGET == TOVE_TARGET_LOVE2D
	define("TEXEL", "Texel");
#else
	define("TEXEL", "texture");
#endif
}

void ShaderWriter::beginVertexShader() {
#if TOVE_TARGET == TOVE_TARGET_LOVE2D
	out << R"GLSL(
#ifdef VERTEX
)GLSL";
#endif
}

void ShaderWriter::endVertexShader() {
#if TOVE_TARGET == TOVE_TARGET_LOVE2D
	out << R"GLSL(
vec4 position(mat4 transform_projection, vec4 vertex_pos) {
	return transform_projection * do_vertex(vertex_pos);
}
#endif // VERTEX
)GLSL";
#elif TOVE_TARGET == TOVE_TARGET_GODOT
	out << R"GLSL(
void vertex() {
	vec4 v = vec4(VERTEX, 0.0, 1.0);
	v = do_vertex(v);
	VERTEX = (EXTRA_MATRIX * (WORLD_MATRIX * v)).xy;
}
)GLSL";
#endif
}

void ShaderWriter::beginFragmentShader() {
#if TOVE_TARGET == TOVE_TARGET_LOVE2D
	out << R"GLSL(
#ifdef PIXEL
)GLSL";
#endif
}

void ShaderWriter::endFragmentShader() {
#if TOVE_TARGET == TOVE_TARGET_LOVE2D
	out << R"GLSL(
vec4 effect(vec4 _1, Image _2, vec2 texture_coords, vec2 _4) {
	return do_color(texture_coords);
}
#endif // PIXEL
)GLSL";
#endif
}

void ShaderWriter::computeLineColor(int lineStyle) {
	out << "#define LINE_STYLE "<< lineStyle << "\n";
	out << R"GLSL(
#if LINE_STYLE == 1
uniform vec4 linecolor;
#elif LINE_STYLE >= 2
uniform sampler2D linecolors;
uniform MATRIX linematrix;
uniform vec2 linecscale;
#endif

#if LINE_STYLE > 0
vec4 computeLineColor(vec2 pos) {
#if LINE_STYLE == 1
	return linecolor;
#elif LINE_STYLE == 2
	float y = (linematrix * vec3(pos, 1)).y;
	y = linecscale.x + linecscale.y * y;
	return TEXEL(linecolors, vec2(0.5, y));
#elif LINE_STYLE == 3
	float y = length((linematrix * vec3(pos, 1)).xy);
	y = linecscale.x + linecscale.y * y;
	return TEXEL(linecolors, vec2(0.5, y));
#endif
}
#endif
)GLSL";
}

void ShaderWriter::computeFillColor(int fillStyle) {
	out << "#define FILL_STYLE "<< fillStyle << "\n";
	out << R"GLSL(
#if FILL_STYLE == 1
uniform vec4 fillcolor;
#elif FILL_STYLE >= 2
uniform sampler2D fillcolors;
uniform MATRIX fillmatrix;
uniform vec2 fillcscale;
#endif

#if FILL_STYLE > 0
vec4 computeFillColor(vec2 pos) {
#if FILL_STYLE == 1
	return fillcolor;
#elif FILL_STYLE == 2
	float y = (fillmatrix * vec3(pos, 1)).y;
	y = fillcscale.x + fillcscale.y * y;
	return TEXEL(fillcolors, vec2(0.5, y));
#elif FILL_STYLE == 3
	float y = length((fillmatrix * vec3(pos, 1)).xy);
	y = fillcscale.x + fillcscale.y * y;
	return TEXEL(fillcolors, vec2(0.5, y));
#endif
}
#endif
)GLSL";
}

const char *ShaderWriter::getSourcePtr() {
	std::string source = out.str();

	for (auto i : defines) {
		size_t pos = 0;
	    while ((pos = source.find(i.first, pos)) != std::string::npos) {
	         source.replace(pos, i.first.length(), i.second);
	         pos += i.second.length();
	    }
	}

	//fprintf(stderr, "%s", source.c_str());

	sSource = source;
	return sSource.c_str();
}

END_TOVE_NAMESPACE

void ConfigureShaderCode(ToveShaderLanguage language, int matrixRows) {
	tove::ShaderWriter::configure(language, matrixRows);
}

const char *GetPaintShaderCode(int numPaints) {
	tove::ShaderWriter w;

	w.define("NUM_PAINTS", numPaints);

	w << R"GLSL(
varying vec2 gradient_pos;
flat varying vec3 gradient_scale;
flat varying vec2 texture_pos;
)GLSL";

	w.beginVertexShader();

#if TOVE_TARGET == TOVE_TARGET_LOVE2D
	w << "attribute float VertexPaint;\n";
#elif TOVE_TARGET == TOVE_TARGET_GODOT
	w.define("VertexPaint", "UV.x");
#endif

	w << R"GLSL(
uniform MATRIX matrix[NUM_PAINTS];
uniform float arguments[NUM_PAINTS];
uniform float cstep;

vec4 do_vertex(vec4 vertex_pos) {
	int i = int(VertexPaint);

	gradient_pos = (matrix[i] * vec3(vertex_pos.xy, 1)).xy;
	gradient_scale = vec3(cstep, 1.0f - 2.0f * cstep, arguments[i]);

	float y = mix(gradient_pos.y, length(gradient_pos), gradient_scale.z);
	y = gradient_scale.x + gradient_scale.y * y;

	texture_pos = vec2((i + 0.5) / NUM_PAINTS, y);
	return vertex_pos;
}
)GLSL";

	w.endVertexShader();

	w.beginFragmentShader();

	w << R"GLSL(
uniform sampler2D colors;

vec4 do_color(vec2 _1) {
	// encourage texture prefetch via texture_pos; for solid colors and
	// many linear gradients, texture_pos == texture_pos_exact.
	vec4 c = TEXEL(colors, texture_pos);

	// now for the adjust coordinate.
	float y = mix(gradient_pos.y, length(gradient_pos), gradient_scale.z);
	y = gradient_scale.x + gradient_scale.y * y;

	vec2 texture_pos_exact = vec2(texture_pos.x, y);
	return TEXEL(colors, texture_pos_exact);
}
)GLSL";

	w.endFragmentShader();

	return w.getSourcePtr();
}

const char *GetImplicitFillShaderCode(
	const ToveShaderData *data, bool fragLine, bool meshBand, bool debug) {
	
	tove::ShaderWriter w;

	w << R"GLSL(
varying vec4 raw_vertex_pos;
)GLSL";

	w.beginVertexShader();
	if (!meshBand) {
		w << R"GLSL(
uniform vec4 bounds;

vec4 do_vertex(vec4 vertex_pos) {
	raw_vertex_pos = vec4(mix(bounds.xy, bounds.zw, vertex_pos.xy), vertex_pos.zw);
	return raw_vertex_pos;
}
)GLSL";
	} else {
		w << R"GLSL(
vec4 do_vertex(vec4 vertex_pos) {
	raw_vertex_pos = vertex_pos;
	return vertex_pos;
}
)GLSL";
	}
	w.endVertexShader();

	w.beginFragmentShader();

	if (!meshBand) {
		// encourage shader caching by trying to reduce code changing states.
		const int lutN = tove::nextpow2(data->geometry.lookupTableSize);

		w << "#define LUT_SIZE "<< lutN << "\n";
		w << "#define LUT_BANDS 1\n";
	} else {
		w << "#define MESH_BANDS 1\n";
	}

	if (debug) {
		w << "#define GPUX_DEBUG 1\n";
	}

	w << "#define FILL_RULE "<< data->geometry.fillRule << "\n";
	w << "#define CURVE_DATA_SIZE "<<
		data->geometry.curvesTextureSize[0] << "\n";

	w.computeLineColor(fragLine ? data->color.line.style : 0);
	w.computeFillColor(data->color.fill.style);

	#include "../../glsl/fill.frag.inc"

	w.endFragmentShader();

	return w.getSourcePtr();
}

const char *GetImplicitLineShaderCode(const ToveShaderData *data) {
	tove::ShaderWriter w;

	w << R"GLSL(
varying vec2 raw_vertex_pos;
)GLSL";

	w << "#define CURVE_DATA_SIZE "<<
		data->geometry.curvesTextureSize[0] << "\n";

	w.beginVertexShader();
	#include "../../glsl/line.vert.inc"
	w.endVertexShader();

	w.beginFragmentShader();
	w.computeLineColor(data->color.line.style);
	w << R"GLSL(
vec4 do_color(vec2 _1) {
	return computeLineColor(raw_vertex_pos);
}
)GLSL";
	w.endFragmentShader();

	return w.getSourcePtr();
}
