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

#ifndef __TOVE_SHADER_LINK
#define __TOVE_SHADER_LINK 1

#include "color.h"
#include "geometry.h"

class AbstractShaderLink {
public:
	virtual ~AbstractShaderLink() {
	}

	virtual ToveChangeFlags beginUpdate(const PathRef &path, bool initial) = 0;
	virtual ToveChangeFlags endUpdate(const PathRef &path, bool initial) = 0;

    virtual ToveShaderData *getData() {
    	return nullptr;
    }

    virtual ToveShaderLineFillColorData *getColorData() {
    	return nullptr;
    }
};

class ColorShaderLink : public AbstractShaderLink {
private:
	const float scale;
	ToveShaderLineFillColorData data;
	LineColorShaderLinkImpl lineColor;
	FillColorShaderLinkImpl fillColor;

public:
	ColorShaderLink(float scale) :
		scale(scale),
		lineColor(data.line, scale),
		fillColor(data.fill, scale) {
	}

	virtual ToveChangeFlags beginUpdate(const PathRef &path, bool initial) {
		ToveChangeFlags changes = lineColor.beginUpdate(path, initial) |
			fillColor.beginUpdate(path, initial);
		return changes;
	}

    virtual ToveChangeFlags endUpdate(const PathRef &path, bool initial) {
		return lineColor.endUpdate(path, initial) |
			fillColor.endUpdate(path, initial);
    }

    virtual ToveShaderLineFillColorData *getColorData() {
    	return &data;
    }
};

class GeometryShaderLink : public AbstractShaderLink {
private:
	ToveShaderData data;
	LineColorShaderLinkImpl lineColor;
	FillColorShaderLinkImpl fillColor;
	GeometryShaderLinkImpl geometry;

public:
	GeometryShaderLink(const PathRef &path, bool enableFragmentShaderStrokes) :
		lineColor(data.color.line, 1),
		fillColor(data.color.fill, 1),
		geometry(data.geometry, data.color.line, path, enableFragmentShaderStrokes) {
	}

	virtual ToveChangeFlags beginUpdate(const PathRef &path, bool initial) {
		return lineColor.beginUpdate(path, initial) |
			fillColor.beginUpdate(path, initial) |
			geometry.beginUpdate(path, initial);
	}

    virtual ToveChangeFlags endUpdate(const PathRef &path, bool initial) {
		return lineColor.endUpdate(path, initial) |
			fillColor.endUpdate(path, initial) |
			geometry.endUpdate(path, initial);
    }

    virtual ToveShaderData *getData() {
    	return &data;
    }
};

#endif // __TOVE_SHADER_LINK
