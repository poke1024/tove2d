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

#include "nsvg.h"
#include "utils.h"

#include "../thirdparty/robin-map/include/tsl/robin_map.h"
#include "../thirdparty/tinyxml2/tinyxml2.h"

#if TOVE_DEBUG
#include <iostream>
#endif

BEGIN_TOVE_NAMESPACE

#define NANOSVG_IMPLEMENTATION
#define NANOSVG_ALL_COLOR_KEYWORDS
#include "../thirdparty/nanosvg/src/nanosvg.h"

#define NSVG__FIXSHIFT 14
#define NANOSVGRAST_IMPLEMENTATION
#include "../thirdparty/nanosvg/src/nanosvgrast.h"

namespace nsvg {

thread_local NSVGparser *_parser = nullptr;
thread_local NSVGrasterizer *rasterizer = nullptr;
thread_local ToveRasterizeSettings defaultSettings = {-1.0f, -1.0f, 0};

// scoping the locale should no longer be necessary.
#define NSVG_SCOPE_LOCALE 0

class NanoSVGEnvironment {
	const char* previousLocale;

public:
	inline NanoSVGEnvironment() {
		// setting the locale here is important! nsvg relies
		// on sscanf to parse floats and units in the svg and
		// this will break with non-en locale (same with strtof).

		// sometimes, on non-English systems, "42.5%" would get
		// parsed as "42.0", with a unit of ".5%", which messed
		// everything up. setting the locale here fixes this.

#if NSVG_SCOPE_LOCALE
	    previousLocale = setlocale(LC_NUMERIC, nullptr);
	    setlocale(LC_NUMERIC, "en_US.UTF-8");
#endif
	}

	inline ~NanoSVGEnvironment() {
#if NSVG_SCOPE_LOCALE
		if (previousLocale) {
		    setlocale(LC_NUMERIC, previousLocale);
		}
#endif
	}
};

namespace bridge {

typedef void (*StartElementCallback)(void* ud, const char* el, const char** attr);
typedef void (*EndElementCallback)(void* ud, const char* el);

struct hash_cstr {
	// a simple, fast FNV-1a hash for C-style strings
	inline size_t operator()(const char *s) const {
		uint64_t h = 0xcbf29ce484222325;
		int c;
		while ((c = *s++) != '\0') {
			h ^= c;
			h *= 0x100000001b3;
		}
		return h;
	}
};

struct equal_cstr {
	inline bool operator()(const char *a, const char *b) const {
		return strcmp(a, b) == 0;
	}
};

class NanoSVGVisitor : public tinyxml2::XMLVisitor {
	using XMLDocument = tinyxml2::XMLDocument;
	using XMLElement = tinyxml2::XMLElement;
	using XMLAttribute = tinyxml2::XMLAttribute;

	StartElementCallback mStartElement;
	EndElementCallback mEndElement;
	void *mUserData;
	const XMLDocument *mCurrentDocument;
	bool mSkipDefs;

	typedef tsl::robin_map<
		const char*,
		const XMLElement*,
		hash_cstr,
		equal_cstr,
		std::allocator<std::pair<const char*, const XMLElement*>>,
		true /* store hash */> ElementsMap;
	std::unique_ptr<ElementsMap> mElementsById;

	void gatherIds(const XMLElement *parent) {
		const XMLElement *child = parent->FirstChildElement();
		while (child) {
			gatherIds(child);

			const char *id = child->Attribute("id");
			if (id) {
				mElementsById->insert(ElementsMap::value_type(id, child));
			}
			child = child->NextSiblingElement();
		}
	}

	const XMLElement *lookupById(const char *id) {
		if (!mElementsById.get()) {
			assert(mCurrentDocument);
			mElementsById.reset(new ElementsMap);
			gatherIds(mCurrentDocument->RootElement());
		}

		const auto it = mElementsById->find(id);
		if (it != mElementsById->end()) {
			return it->second;
		} else {
			return nullptr;
		}
	}

public:
	NanoSVGVisitor(
		StartElementCallback startElement,
		EndElementCallback endElement,
		void *userdata) :

		mStartElement(startElement),
		mEndElement(endElement),
		mUserData(userdata),
		mSkipDefs(false) {

	}

    virtual bool VisitEnter(const XMLDocument &doc) {
    	mCurrentDocument = doc.ToDocument();

		// always handle <defs> tags first
		mSkipDefs = false;
		const XMLElement *defs = doc.RootElement()->FirstChildElement("defs");
		while (defs) {
			defs->Accept(this);
			defs = defs->NextSiblingElement("defs");
		}
		mSkipDefs = true;

        return true;
    }

    virtual bool VisitExit(const XMLDocument &doc) {
    	mCurrentDocument = nullptr;
        return true;
    }

    virtual bool VisitEnter(const XMLElement &element, const XMLAttribute *firstAttribute) {
    	if (mSkipDefs && strcmp(element.Name(), "defs") == 0 &&
			element.Parent() == mCurrentDocument->RootElement()) {
			
			// already handled in l VisitEnter(const XMLDocument &doc)
			return true;
		}
    	if (strcmp(element.Name(), "use") == 0) {
    		const char *href = element.Attribute("href");
			if (!href) {
				href = element.Attribute("xlink:href");
			}
			if (href) {
				const char *s = href;
				while (isspace(*s)) {
					s++;
				}
				if (*s == '#') {
					const XMLElement *referee = lookupById(s + 1);
					if (referee) {
						referee->Accept(this);
					}
				}
			}
    	} else {
			const char *attr[NSVG_XML_MAX_ATTRIBS];
			int numAttr = 0;

			const XMLAttribute *attribute = firstAttribute;
			while (attribute && numAttr < NSVG_XML_MAX_ATTRIBS - 3) {
				attr[numAttr++] = attribute->Name();
				attr[numAttr++] = attribute->Value();

				attribute = attribute->Next();
			}

			attr[numAttr++] = 0;
			attr[numAttr++] = 0;	

	    	mStartElement(mUserData, element.Name(), attr);
    	}

        return true;
    }

    virtual bool VisitExit(const XMLElement& element) {
    	mEndElement(mUserData, element.Name());
        return true;
    }
};

int parseSVG(
	char* input,
	void (*startelCb)(void* ud, const char* el, const char** attr),
	void (*endelCb)(void* ud, const char* el),
	void (*contentCb)(void* ud, const char* s),
	void* ud) {

	tinyxml2::XMLDocument doc;
	doc.Parse(input);
	NanoSVGVisitor visitor(startelCb, endelCb, ud);
	return doc.Accept(&visitor) ? 1 : 0;
}

} // bridge

NSVGimage *parseSVG(const char *svg, const char *units, float dpi) {
	const NanoSVGEnvironment env;
	// we know that our own bridge::parseSVG won't destroy the svg input
	// text, so it's safe to const_cast here.
	return nsvgParseEx(const_cast<char*>(svg), units, dpi, bridge::parseSVG);
}

static NSVGrasterizer *ensureRasterizer() {
	if (!rasterizer) {
		rasterizer = nsvgCreateRasterizer();
	}
	return rasterizer;
}

const ToveRasterizeSettings *getDefaultRasterizeSettings() {

	if (defaultSettings.tessTolerance < 0.0f) {
		rasterizer = ensureRasterizer();
		if (!rasterizer) {
			return nullptr;
		}

		defaultSettings.tessTolerance = rasterizer->tessTol;
		defaultSettings.distTolerance = rasterizer->distTol;
		defaultSettings.quality = 1; //1;
	}

	return &defaultSettings;
}

static NSVGrasterizer *getRasterizer(
	const ToveRasterizeSettings *settings) {

	rasterizer = ensureRasterizer();
	if (!rasterizer) {
		return nullptr;
	}
	// skipping nsvgDeleteRasterizer(rasterizer)

	if (!settings) {
		settings = getDefaultRasterizeSettings();
	}

	rasterizer->tessTol = settings->tessTolerance;
	rasterizer->distTol = settings->distTolerance;
	rasterizer->quality = settings->quality;

	rasterizer->nedges = 0;
	rasterizer->npoints = 0;
	rasterizer->npoints2 = 0;

	return rasterizer;
}

static NSVGparser *getNSVGparser() {
	if (!_parser) {
		_parser = nsvg__createParser();
		if (!_parser) {
			TOVE_BAD_ALLOC();
			return nullptr;
		}
	} else {
		nsvg__resetPath(_parser);
	}
	return _parser;
}

uint32_t makeColor(float r, float g, float b, float a) {
	return nsvg__RGBA(
		clamp(r, 0, 1) * 255.0,
		clamp(g, 0, 1) * 255.0,
		clamp(b, 0, 1) * 255.0,
		clamp(a, 0, 1) * 255.0);
}

uint32_t applyOpacity(uint32_t color, float opacity) {
	return nsvg__applyOpacity(color, opacity);
}

int maxDashes() {
	return NSVG_MAX_DASHES;
}

void curveBounds(float *bounds, float *curve) {
	nsvg__curveBounds(bounds, curve);
}

void Matrix3x2::setIdentity() {
	double *t = m_val;
	t[0] = 1.0f; t[1] = 0.0f;
	t[2] = 0.0f; t[3] = 1.0f;
	t[4] = 0.0f; t[5] = 0.0f;
}

Matrix3x2 Matrix3x2::inverse() const {
	Matrix3x2 result;
	double *inv = result.m_val;
	const double *t = m_val;

	// adapted from nanosvg.
	const double det = t[0] * t[3] - t[2] * t[1];
	inv[0] = (t[3] / det);
	inv[2] = (-t[2] / det);
	inv[4] = ((t[2] * t[5] - t[3] * t[4]) / det);
	inv[1] = (-t[1] / det);
	inv[3] = (t[0] / det);
	inv[5] = ((t[1] * t[4] - t[0] * t[5]) / det);

	return result;
}

void xformInverse(float *a, float *b) {
	nsvg__xformInverse(a, b);
}

void xformIdentity(float *m) {
	nsvg__xformIdentity(m);
}

NSVGimage *parsePath(const char *d) {
	const NanoSVGEnvironment env;

	NSVGparser *parser = getNSVGparser();

	const char *attr[3] = {"d", d, nullptr};
	nsvg__parsePath(parser, attr);

	NSVGimage *image = parser->image;

	parser->image = static_cast<NSVGimage*>(malloc(sizeof(NSVGimage)));
	if (!parser->image) {
		TOVE_BAD_ALLOC();
		return nullptr;
	}
	memset(parser->image, 0, sizeof(NSVGimage));

	return image;
}

float *pathArcTo(float *cpx, float *cpy, float *args, int &npts) {
	NSVGparser *parser = getNSVGparser();
	nsvg__pathArcTo(parser, cpx, cpy, args, 0);
	npts = parser->npts;
	return parser->pts;
}

void CachedPaint::init(const NSVGpaint &paint, float opacity) {
	NSVGcachedPaint cache;
	nsvg__initPaint(&cache, const_cast<NSVGpaint*>(&paint), opacity, 0, nullptr);

	type = cache.type;
	spread = cache.spread;
	for (int i = 0; i < 6; i++) {
		xform[i] = cache.xform[i];
	}

	const int n = numColors;
	uint8_t *storage = reinterpret_cast<uint8_t*>(colors);
	const int row = rowBytes;

	for (int i = 0; i < n; i++) {
		*reinterpret_cast<uint32_t*>(storage) = cache.colors[i];
		storage += row;
	}
}

bool shapeStrokeBounds(float *bounds, const NSVGshape *shape,
	float scale, const ToveRasterizeSettings *quality) {

	// computing the shape stroke bounds is not trivial as miters
	// might take varying amounts of space.

	NSVGrasterizer *rasterizer = getRasterizer(quality);
	nsvg__flattenShapeStroke(
		rasterizer, const_cast<NSVGshape*>(shape), scale);

	const int n = rasterizer->nedges;
	if (n < 1) {
		return false;
	} else {
		const NSVGedge *edges = rasterizer->edges;

		float bx0 = edges->x0, by0 = edges->y0;
		float bx1 = bx0, by1 = by0;

 		for (int i = 0; i < n; i++) {
			const float x0 = edges->x0;
			const float y0 = edges->y0;
			const float x1 = edges->x1;
			const float y1 = edges->y1;
			edges++;

			bx0 = std::min(bx0, x0);
			bx0 = std::min(bx0, x1);

			by0 = std::min(by0, y0);
			by0 = std::min(by0, y1);

			bx1 = std::max(bx1, x0);
			bx1 = std::max(bx1, x1);

			by1 = std::max(by1, y0);
			by1 = std::max(by1, y1);
		}

		bounds[0] = bx0;
		bounds[1] = by0;
		bounds[2] = bx1;
		bounds[3] = by1;
		return true;
	}
}

void rasterize(NSVGimage *image, float tx, float ty, float scale,
	uint8_t* pixels, int width, int height, int stride,
	const ToveRasterizeSettings *quality) {

	NSVGrasterizer *rasterizer = getRasterizer(quality);

	nsvgRasterize(rasterizer, image, tx, ty, scale,
			pixels, width, height, stride);
}

Transform::Transform() {
	identity = true;
	scaleLineWidth = false;

	nsvg__xformIdentity(matrix);
}

Transform::Transform(
	float a, float b, float c,
	float d, float e, float f) {

	identity = false;
	scaleLineWidth = false;

	matrix[0] = a;
	matrix[1] = d;
	matrix[2] = b;
	matrix[3] = e;
	matrix[4] = c;
	matrix[5] = f;
}

Transform::Transform(const Transform &t) {
	identity = t.identity;
	scaleLineWidth = t.scaleLineWidth;
	for (int i = 0; i < 6; i++) {
		matrix[i] = t.matrix[i];
	}
}

void Transform::multiply(const Transform &t) {
	nsvg__xformMultiply(matrix, const_cast<float*>(&t.matrix[0]));
	identity = identity && t.identity;
}

void Transform::transformGradient(NSVGgradient* grad) const {
#if TOVE_DEBUG
	std::cout << "transformGradient [original]" << std::endl;
	std::cout << tove::debug::xform<float>(grad->xform);
#endif

	nsvg__xformMultiply(grad->xform, const_cast<float*>(&matrix[0]));

#if TOVE_DEBUG
	std::cout << "transformGradient [transformed]" << std::endl;
	std::cout << tove::debug::xform<float>(grad->xform);
#endif
}

void Transform::transformPoints(float *pts, const float *srcpts, int npts) const {
	if (identity) {
		memcpy(pts, srcpts, 2 * sizeof(float) * npts);
	} else {
		const float t0 = matrix[0];
		const float t1 = matrix[1];
		const float t2 = matrix[2];
		const float t3 = matrix[3];
		const float t4 = matrix[4];
		const float t5 = matrix[5];

		for (int i = 0; i < npts; i++) {
			const float x = srcpts[2 * i + 0];
			const float y = srcpts[2 * i + 1];
			pts[2 * i + 0] = x * t0 + y * t2 + t4;
			pts[2 * i + 1] = x * t1 + y * t3 + t5;
		}
	}
}

float Transform::getScale() const {
	float x = matrix[0] + matrix[2];
	float y = matrix[1] + matrix[3];
	return std::sqrt(x * x + y * y);
}

NSVGlineJoin nsvgLineJoin(ToveLineJoin join) {
	switch (join) {
		case TOVE_LINEJOIN_MITER:
			return NSVG_JOIN_MITER;
		case TOVE_LINEJOIN_ROUND:
			return NSVG_JOIN_ROUND;
		case TOVE_LINEJOIN_BEVEL:
			return NSVG_JOIN_BEVEL;
	}
	return NSVG_JOIN_MITER;
}

ToveLineJoin toveLineJoin(NSVGlineJoin join) {
	switch (join) {
		case NSVG_JOIN_MITER:
			return TOVE_LINEJOIN_MITER;
		case NSVG_JOIN_ROUND:
			return TOVE_LINEJOIN_ROUND;
		case NSVG_JOIN_BEVEL:
			return TOVE_LINEJOIN_BEVEL;
	}
	return TOVE_LINEJOIN_MITER;
}
} // namespace nsvg

END_TOVE_NAMESPACE
