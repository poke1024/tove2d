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

#include <functional>

#define SPECIAL_CASE_2 true

class AbstractPaintFeed {
protected:
	TovePaintData &paintData;
	const float scale;

protected:
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

	int determineNumColors(const PaintRef &paint) const {
		const TovePaintType style = (TovePaintType)paintData.style;
		ToveGradientData &gradient = paintData.gradient;

		if (style < PAINT_LINEAR_GRADIENT) {
			return style == PAINT_SOLID ? 1 : 0;
		} else {
			NSVGgradient *g = paint->getNSVGgradient();
			assert(g && g->nstops >= 1);
	    	if (SPECIAL_CASE_2 && g->nstops == 2 &&
				g->stops[0].offset == 0.0 &&
				g->stops[1].offset == 1.0) {
	    		return 2;
	    	} else {
	    		return 256;
	    	}
		}
	}

	bool update(const PaintRef &paint, float opacity) {
		const TovePaintType style = (TovePaintType)paintData.style;
		ToveGradientData &gradient = paintData.gradient;

		if (style < PAINT_LINEAR_GRADIENT) {
			if (style == PAINT_SOLID) {
				//RGBA old = paintData.rgba;
				paint->getRGBA(paintData.rgba, opacity);

				if (gradient.arguments) {
					if (gradient.matrix) {
						float *m = gradient.matrix;

						m[0] = 1;
						m[1] = 0;
						m[2] = 0;

						m[3] = 0;
						m[4] = 1;
						m[5] = 0;

						m[6] = 0;
						m[7] = 0;
						m[8] = 1;

						if (gradient.matrixType == MATRIX_MAT3x4) {
							m[9] = 0;
							m[10] = 0;
							m[11] = 0;
						}
					}

					if (gradient.arguments) {
						gradient.arguments->x = 0;
						gradient.arguments->y = 0;
						gradient.arguments->z = 0;
						gradient.arguments->w = 0;
					}

					*(uint32_t*)gradient.colorsTexture = nsvg::makeColor(
						paintData.rgba.r,
						paintData.rgba.g,
						paintData.rgba.b,
						paintData.rgba.a
					);
				}
			}

			return true;
		}

		assert(gradient.colorsTexture);
		uint8_t *texture = gradient.colorsTexture;
		const int textureRowBytes = gradient.colorsTextureRowBytes;

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

		assert(gradient.matrix != nullptr);
		paint->getGradientMatrix(*(ToveMatrix3x3*)gradient.matrix, scale);

		if (gradient.arguments) {
			float s = 0.5f / gradient.colorTextureHeight;

			gradient.arguments->x =
				(style == PAINT_RADIAL_GRADIENT) ? 1.0f : 0.0f;
			gradient.arguments->y = 1.0f;
			gradient.arguments->z = s;
			gradient.arguments->w = 1.0f - 2.0f * s;
		}

		return true;
	}

public:
	AbstractPaintFeed(TovePaintData &data, float scale) :
		paintData(data), scale(scale) {

		paintData.style = PAINT_UNDEFINED;
	}
};

class PaintFeedBase : public AbstractPaintFeed {
protected:
	const ToveChangeFlags CHANGED_STYLE;
	const PathRef path;

	const PaintRef &getColor() const {
		static const PaintRef none;

		switch (CHANGED_STYLE) {
			case CHANGED_LINE_STYLE:
				return path->getLineWidth() >= 0.0f ?
					path->getLineColor() : none;
			case CHANGED_FILL_STYLE:
				return path->getFillColor();
			default:
				assert(false);
		}
	}

public:
	PaintFeedBase(
		const PathRef &path,
		TovePaintData &data,
		float scale,
		ToveChangeFlags CHANGED_STYLE) :

		AbstractPaintFeed(data, scale),
		path(path),
		CHANGED_STYLE(CHANGED_STYLE) {

		std::memset(&paintData, 0, sizeof(paintData));

		const PaintRef &color = getColor();
        const TovePaintType style = getStyle(color);
		paintData.style = style;
		paintData.gradient.numColors = determineNumColors(color);
	}

	int getNumColors() const {
		return paintData.gradient.numColors;
	}

	void bind(const ToveGradientData &data, int i) {
		int size;
		switch (data.matrixType) {
			case MATRIX_MAT3x3:
				size = 3 * 3;
				break;
			case MATRIX_MAT3x4:
				size = 3 * 4;
				break;
			default:
				assert(false);
		}

		paintData.gradient.matrix = data.matrix + i * size;
		paintData.gradient.arguments = &data.arguments[i];
		paintData.gradient.colorsTexture = 	data.colorsTexture + 4 * i;
		paintData.gradient.colorsTextureRowBytes = data.colorsTextureRowBytes;
		paintData.gradient.colorTextureHeight = data.colorTextureHeight;

		update(getColor(), path->getOpacity());
	}

	inline ToveChangeFlags beginUpdate() {
		if (path->fetchChanges(CHANGED_STYLE)) {
			const PaintRef &color = getColor();
	        const TovePaintType style = getStyle(color);

	        if (paintData.style != style ||
				determineNumColors(color) != paintData.gradient.numColors) {

 				return CHANGED_RECREATE;
			}

			return CHANGED_STYLE;
		} else {
			return 0;
		}
    }

	inline ToveChangeFlags endUpdate() {
    	return update(getColor(), path->getOpacity()) ? CHANGED_STYLE : 0;
    }
};

class PaintFeed : public PaintFeedBase {
private:
	TovePaintData _data;

public:
	PaintFeed(const PaintFeed &feed) :
		PaintFeedBase(feed.path, _data, feed.scale, feed.CHANGED_STYLE) {
	}

	PaintFeed(const PathRef &path, float scale, ToveChangeFlags changed) :
		PaintFeedBase(path, _data, scale, changed) {
	}
};

class LinePaintFeed : public PaintFeedBase {
public:
	LinePaintFeed(const PathRef &path, TovePaintData &data, float scale) :
		PaintFeedBase(path, data, scale, CHANGED_LINE_STYLE) {
	}
};

class FillPaintFeed : public PaintFeedBase {
public:
	FillPaintFeed(const PathRef &path, TovePaintData &data, float scale) :
		PaintFeedBase(path, data, scale, CHANGED_FILL_STYLE) {
	}
};
