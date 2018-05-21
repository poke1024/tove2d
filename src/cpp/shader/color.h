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

#define SPECIAL_CASE_2 true

class ColorShaderLinkImpl {
protected:
	ToveShaderColorData &shaderData;

	static TovePaintType getStyle(const PaintRef &paint) {
		if (paint.get() == nullptr) {
			return PAINT_NONE;
		}
		const TovePaintType style = paint->getType();
		if (style < PAINT_LINEAR_GRADIENT) {
			return style;
		} else {
			NSVGgradient *g = paint->getNSVGgradient();
			assert(g);
	    	if (g->nstops < 1) {
	    		return PAINT_NONE;
	    	} else {
	    		return style;
	    	}
		}
	}

	void beginUpdateGradient(const PaintRef &paint, float opacity) {
		const TovePaintType style = (TovePaintType)shaderData.style;
		ToveShaderGradient &gradient = shaderData.gradient;

		if (style < PAINT_LINEAR_GRADIENT) {
			gradient.numColors = 0;
			if (style == PAINT_SOLID) {
				paint->getRGBA(&shaderData.rgba, opacity);
			}
		} else {
			NSVGgradient *g = paint->getNSVGgradient();
			assert(g && g->nstops >= 1);
	    	if (SPECIAL_CASE_2 && g->nstops == 2 &&
				g->stops[0].offset == 0.0 &&
				g->stops[1].offset == 1.0) {
	    		gradient.numColors = 2;
	    	} else {
	    		gradient.numColors = 256;
	    	}
		}
	}

	void endUpdateGradient(const PaintRef &paint, float opacity) {
		const TovePaintType style = (TovePaintType)shaderData.style;

		if (style < PAINT_LINEAR_GRADIENT) {
			return;
		}

		ToveShaderGradient &gradient = shaderData.gradient;

		assert(gradient.data);
		assert(gradient.data->colorsTexture);
		uint8_t *texture = gradient.data->colorsTexture;
		const int textureRowBytes = gradient.data->colorsTextureRowBytes;

		NSVGgradient *g = paint->getNSVGgradient();
		assert(g);
		if (SPECIAL_CASE_2 && gradient.numColors <= 2) {
			assert(g->nstops == gradient.numColors);
			for (int i = 0; i < gradient.numColors; i++) {
				*(uint32_t*)texture = nsvg::applyOpacity(
					g->stops[i].color, opacity);
				texture += textureRowBytes;
			}
		} else {
			NSVGpaint nsvgPaint;
			nsvgPaint.type = paint->getType() == PAINT_LINEAR_GRADIENT ?
				NSVG_PAINT_LINEAR_GRADIENT : NSVG_PAINT_RADIAL_GRADIENT;
			nsvgPaint.gradient = g;

			assert(gradient.numColors == 256);

			nsvg::CachedPaint cached(texture, textureRowBytes, 256);
			cached.init(nsvgPaint, opacity);
		}

		paint->getGradientMatrix(gradient.data->matrix);
	}

public:
	ColorShaderLinkImpl(ToveShaderColorData &data) : shaderData(data) {
		shaderData.style = PAINT_UNDEFINED;
	}
};

class LineColorShaderLinkImpl : public ColorShaderLinkImpl {
public:
	ToveChangeFlags beginUpdate(const PathRef &path, bool initial) {
    	ToveChangeFlags changes = path->fetchChanges(CHANGED_LINE_STYLE);

    	if (shaderData.style == PAINT_UNDEFINED) {
    		changes |= CHANGED_LINE_STYLE;
    	}

    	if (changes == 0) {
    		return 0; // done
    	}

		const PaintRef &lineColor = path->getLineColor();

        const bool hasStroke = path->getLineWidth() > 0 && lineColor.get() != nullptr;

        TovePaintType lineStyle = getStyle(hasStroke ? lineColor : PaintRef());
        if (shaderData.style != PAINT_UNDEFINED && shaderData.style != lineStyle) {
        	return CHANGED_RECREATE;
        }
        shaderData.style = lineStyle;

		if (changes & CHANGED_LINE_STYLE) {
			beginUpdateGradient(lineColor, path->getOpacity());
		}

		return changes;
    }

    int endUpdate(const PathRef &path, bool initial) {
    	// (1) compute gradient colors

    	endUpdateGradient(path->getLineColor(), path->getOpacity());

    	return 0;
    }

	LineColorShaderLinkImpl(ToveShaderColorData &data) : ColorShaderLinkImpl(data) {
	}
};

class FillColorShaderLinkImpl : public ColorShaderLinkImpl {
public:
	ToveChangeFlags beginUpdate(const PathRef &path, bool initial) {
    	ToveChangeFlags changes = path->fetchChanges(CHANGED_FILL_STYLE);

    	if (shaderData.style == PAINT_UNDEFINED) {
    		changes |= CHANGED_FILL_STYLE;
    	}

    	if (changes == 0) {
    		return 0; // done
    	}

		const PaintRef &fillColor = path->getFillColor();

        const TovePaintType style = getStyle(fillColor);
        if (shaderData.style != PAINT_UNDEFINED && shaderData.style != style) {
        	return CHANGED_RECREATE;
        }
        shaderData.style = style;

		if (changes & CHANGED_FILL_STYLE) {
			beginUpdateGradient(fillColor, path->getOpacity());
		}

		return changes;
	}

    int endUpdate(const PathRef &path, bool initial) {
    	// (1) compute gradient colors

    	endUpdateGradient(path->getFillColor(), path->getOpacity());

    	return 0;
    }

	FillColorShaderLinkImpl(ToveShaderColorData &data) : ColorShaderLinkImpl(data) {
	}
};
