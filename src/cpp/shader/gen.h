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

#ifndef __TOVE_SHADER_GEN
#define __TOVE_SHADER_GEN 1

#include <sstream>
#include <map>

BEGIN_TOVE_NAMESPACE

class ShaderWriter {
private:
	std::ostringstream out;
	std::map<std::string, std::string> defines;

	static thread_local std::string sSource;

	static ToveShaderLanguage sLanguage;
	static int sMatrixRows;

public:
	static void configure(ToveShaderLanguage language, int matrixRows);

	static inline int getMatrixRows() {
		return sMatrixRows;
	}

	ShaderWriter();

	void beginVertexShader();
	void endVertexShader();

	void beginFragmentShader();
	void endFragmentShader();

	void computeLineColor(int lineStyle);
	void computeFillColor(int fillStyle);

	void define(const std::string &key, const std::string &value);
	void define(const std::string &key, int value);

	inline ShaderWriter &operator<<(int i) {
		out << i;
		return *this;
	}

	inline ShaderWriter &operator<<(const char *s) {
		out << s;
		return *this;
	}

	const char *getSourcePtr();
};

END_TOVE_NAMESPACE

#endif // __TOVE_SHADER_GEN
