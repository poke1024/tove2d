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
#include <sstream>

BEGIN_TOVE_NAMESPACE

#define TOVE_LOVE2D 1
#define TOVE_GODOT 2

#define TOVE_TARGET TOVE_LOVE2D

class ShaderWriter {
private:
	std::ostringstream out;
	static thread_local std::string sSource;

	static ToveShaderLanguage sLanguage;
	static bool sAvoidMat3;

public:
	static void configure(ToveShaderLanguage language, bool useMat3x4);

	void prolog();

	void beginVertexShader();
	void endVertexShader();

	void beginFragmentShader();
	void endFragmentShader();

	void computeLineColor(int lineStyle);
	void computeFillColor(int fillStyle);

	ShaderWriter &operator<<(int i) {
		out << i;
		return *this;
	}

	ShaderWriter &operator<<(const char *s) {
		out << s;
		return *this;
	}

	const char *getSourcePtr();
};

thread_local std::string ShaderWriter::sSource;
ToveShaderLanguage ShaderWriter::sLanguage = TOVE_GLSL2;
bool ShaderWriter::sAvoidMat3 = false;

void ShaderWriter::configure(ToveShaderLanguage language, bool avoidMat3) {
	sLanguage = language;
	sAvoidMat3 = avoidMat3;
}

void ShaderWriter::prolog() {
#if TOVE_TARGET == TOVE_LOVE2D
	if (sLanguage == TOVE_GLSL3) {
		out << "#pragma language glsl3\n#define GLSL3 1\n";
	}
#else
	out << "shader_type canvas_item;\nrender_mode skip_vertex_transform\n";
#endif

	out << "#define MAT3 " << (sAvoidMat3 ? "mat3x4" : "mat3") << "\n";

#if TOVE_TARGET == TOVE_LOVE2D
	out << "#define TEXEL Texel\n";
#else
	out << "#define TEXEL texture\n";
#endif
}

void ShaderWriter::beginVertexShader() {
#if TOVE_TARGET == TOVE_LOVE2D
	out << R"GLSL(
#ifdef VERTEX
)GLSL";
#endif
}

void ShaderWriter::endVertexShader() {
#if TOVE_TARGET == TOVE_LOVE2D
	out << R"GLSL(
vec4 position(mat4 transform_projection, vec4 vertex_pos) {
	return transform_projection * do_vertex(vertex_pos);
}
#endif // VERTEX
	)GLSL";
#elif TOVE_TARGET == TOVE_GODOT
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
#if TOVE_TARGET == TOVE_LOVE2D
	out << R"GLSL(
#ifdef PIXEL
)GLSL";
#endif
}

void ShaderWriter::endFragmentShader() {
#if TOVE_TARGET == TOVE_LOVE2D
	out << R"GLSL(
vec4 effect(vec4 _1, Image _2, vec2 _3, vec2 _4) {
	return do_color();
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
uniform MAT3 linematrix;
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
uniform MAT3 fillmatrix;
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
	sSource = out.str();
	//fprintf(stderr, "%s", sSource.c_str());
	return sSource.c_str();
}

END_TOVE_NAMESPACE

void ConfigureShaderCode(ToveShaderLanguage language, bool avoidMat3) {
	tove::ShaderWriter::configure(language, avoidMat3);
}

const char *GetPaintShaderCode(int numPaints) {
	tove::ShaderWriter w;

	w.prolog();

	w << "#define NUM_PAINTS " << numPaints << "\n";

	w << R"GLSL(
varying vec2 gradient_pos;
varying vec3 gradient_scale;
varying vec2 texture_pos;
)GLSL";

	w.beginVertexShader();

	w << R"GLSL(
attribute float VertexPaint;

uniform MAT3 matrix[NUM_PAINTS];
uniform vec4 arguments[NUM_PAINTS];

vec4 do_vertex(vec4 vertex_pos) {
	int i = int(VertexPaint);
	vec4 vertex_arg = arguments[i];
	gradient_pos = (matrix[i] * vec3(vertex_pos.xy, 1)).xy;
	gradient_scale = vertex_arg.zwx;

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

vec4 do_color() {
	float y = mix(gradient_pos.y, length(gradient_pos), gradient_scale.z);
	y = gradient_scale.x + gradient_scale.y * y;

	vec2 texture_pos_exact = vec2(texture_pos.x, y);
	// ensure texture prefetch via texture_pos; for solid colors and
	// many linear gradients, texture_pos == texture_pos_exact.
	vec4 c = TEXEL(colors, texture_pos);

	return texture_pos_exact == texture_pos ?
		c : TEXEL(colors, texture_pos_exact);
}
)GLSL";

	w.endFragmentShader();

	return w.getSourcePtr();
}

const char *GetImplicitFillShaderCode(const ToveShaderData *data, bool lines) {
	tove::ShaderWriter w;

	w.prolog();

	w << R"GLSL(
varying vec4 raw_vertex_pos;
)GLSL";

	w.beginVertexShader();
	w << R"GLSL(
uniform vec4 bounds;

vec4 do_vertex(vec4 vertex_pos) {
	raw_vertex_pos = vec4(mix(bounds.xy, bounds.zw, vertex_pos.xy), vertex_pos.zw);
	return raw_vertex_pos;
}
)GLSL";
	w.endVertexShader();

	w.beginFragmentShader();

	// encourage shader caching by trying to reduce code changing states.
	const int lutN = tove::nextpow2(data->geometry.lookupTableSize);

	w << "#define LUT_SIZE "<< lutN << "\n";
	w << "#define FILL_RULE "<< data->geometry.fillRule << "\n";
	w << "#define CURVE_DATA_SIZE "<<
		data->geometry.curvesTextureSize[0] << "\n";

	w.computeLineColor(lines ? data->color.line.style : 0);
	w.computeFillColor(data->color.fill.style);

	#include "../glsl/fill.inc"

	w.endFragmentShader();

	return w.getSourcePtr();
}

const char *GetImplicitLineShaderCode(const ToveShaderData *data) {
	tove::ShaderWriter w;

	w.prolog();

	w << R"GLSL(
varying vec2 raw_vertex_pos;
)GLSL";

	w << "#define CURVE_DATA_SIZE "<<
		data->geometry.curvesTextureSize[0] << "\n";

	w.beginVertexShader();
	#include "../glsl/line.inc"
	w.endVertexShader();

	w.beginFragmentShader();
	w.computeLineColor(data->color.line.style);
	w << R"GLSL(
vec4 do_color() {
	return computeLineColor(raw_vertex_pos);
}
)GLSL";
	w.endFragmentShader();

	return w.getSourcePtr();
}
