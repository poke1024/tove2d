typedef void (*ToveWarningFunction)(const char *s);

typedef uint32_t ToveChangeFlags;

typedef uint32_t ToveMeshUpdateFlags;

typedef struct {
	ToveMeshUpdateFlags update;
} ToveMeshResult;

typedef struct {
	float *array;
	int n;
} ToveFloatArray;

typedef struct {
	uint8_t *array;
	int n;
} ToveColorArray;

typedef struct {
	uint16_t *array;
	int n;
} ToveIndex16Array;

typedef struct {
	uint8_t *pixels;
} ToveImageRecord;

EXPORT void DeleteImage(ToveImageRecord image);

typedef enum {
	PAINT_UNDEFINED = -1,
	PAINT_NONE = 0,
	PAINT_SOLID = 1,
	PAINT_LINEAR_GRADIENT = 2,
	PAINT_RADIAL_GRADIENT = 3
} TovePaintType;

typedef enum {
	FILLRULE_NON_ZERO = 0,
	FILLRULE_EVEN_ODD = 1
} ToveFillRule;

typedef enum {
	LINEJOIN_MITER,
	LINEJOIN_ROUND,
	LINEJOIN_BEVEL
} ToveLineJoin;

typedef enum {
	ORIENTATION_CW = 0,
	ORIENTATION_CCW = 1
} ToveOrientation;

typedef enum {
	HOLES_NONE,
	HOLES_CW,
	HOLES_CCW
} ToveHoles;

enum {
	CHANGED_FILL_STYLE = 1,
	CHANGED_LINE_STYLE = 2,
	CHANGED_POINTS = 4,
	CHANGED_GEOMETRY = 8,
	CHANGED_LINE_ARGS = 16,
	CHANGED_BOUNDS = 32,
	CHANGED_EXACT_BOUNDS = 64,
	CHANGED_RECREATE = 128,
	CHANGED_ANYTHING = 255
};

enum {
	UPDATE_MESH_VERTICES = 1,
	UPDATE_MESH_COLORS = 2,
	UPDATE_MESH_TRIANGLES = 4,
	UPDATE_MESH_EVERYTHING = 7,
	UPDATE_MESH_AUTO_TRIANGLES = 128
};

typedef struct {
	float x, y;
} TovePoint;

typedef struct {
	void *ptr;
} TovePaintRef;

typedef struct {
	void *ptr;
} ToveSubpathRef;

typedef struct {
	void *ptr;
} ToveCommandRef;

typedef struct {
	void *ptr;
} TovePathRef;

typedef struct {
	void *ptr;
} ToveGraphicsRef;

typedef struct {
	int recursionLimit;
	struct {
		bool valid;
		float distanceTolerance;
		float colinearityEpsilon;
		float angleEpsilon;
		float angleTolerance;
		float cuspLimit;
	} adaptive;
} ToveTesselationQuality;

typedef struct {
	float r, g, b, a;
} RGBA;

typedef struct {
	union {
		float bounds[4];
		struct {
			float x0;
			float y0;
			float x1;
			float y1;
		};
	};
} ToveBounds;

typedef struct {
	float x, y;
} ToveVec2;

typedef struct {
	float t;
	float distanceSquared;
} ToveNearest;

typedef float ToveMatrix3x3[3 * 3];

typedef struct {
	ToveMatrix3x3 *matrix;
	uint8_t *colorsTexture;
	int colorsTextureRowBytes;
} ToveShaderGradientData;

typedef struct {
	ToveShaderGradientData *data;
	int numColors;
} ToveShaderGradient;

typedef struct {
	int8_t style; // TovePaintType
	RGBA rgba;
	ToveShaderGradient gradient;
} ToveShaderColorData;

typedef struct {
	int n[2]; // number of used lookup table elements for x and y
	int bsearch; // number of needed (compound) bsearch iterations
} ToveLookupTableMeta;

typedef struct {
	int16_t curveIndex;
	int16_t numCurves;
	bool isClosed;
} ToveLineRun;

typedef struct {
	int16_t maxCurves;
	int16_t numCurves;
	int16_t numSubPaths;

	ToveBounds *bounds;
	float strokeWidth;
	float miterLimit;
	int8_t fillRule;
	bool fragmentShaderStrokes;
	ToveLineRun *lineRuns;

	float *lookupTable;
	int16_t lookupTableSize;
	ToveLookupTableMeta *lookupTableMeta;

	uint8_t *listsTexture;
	int listsTextureRowBytes;
	int listsTextureSize[2];
	const char *listsTextureFormat;

	uint16_t *curvesTexture;
	int curvesTextureRowBytes;
	int curvesTextureSize[2];
	const char *curvesTextureFormat;
} ToveShaderGeometryData;

typedef struct {
	ToveShaderColorData line;
	ToveShaderColorData fill;
} ToveShaderLineFillColorData;

typedef struct {
	ToveShaderLineFillColorData color;
	ToveShaderGeometryData geometry;
} ToveShaderData;

typedef struct {
	void *ptr;
} ToveShaderLinkRef;

typedef struct {
	void *ptr;
} ToveMeshRef;

EXPORT const char *GetVersion();
EXPORT void SetWarningFunction(ToveWarningFunction f);

EXPORT TovePaintType PaintGetType(TovePaintRef paint);
EXPORT TovePaintRef ClonePaint(TovePaintRef paint);
EXPORT TovePaintRef NewEmptyPaint();
EXPORT TovePaintRef NewColor(float r, float g, float b, float a);
EXPORT void ColorSet(TovePaintRef color, float r, float g, float b, float a);
EXPORT RGBA ColorGet(TovePaintRef color, float opacity);
EXPORT TovePaintRef NewLinearGradient(float x1, float y1, float x2, float y2);
EXPORT TovePaintRef NewRadialGradient(float cx, float cy, float fx, float fy, float r);
EXPORT void GradientAddColorStop(TovePaintRef gradient, float offset, float r, float g, float b, float a);
EXPORT void GradientSetColorStop(TovePaintRef gradient, int i, float offset, float r, float g, float b, float a);
EXPORT void ReleasePaint(TovePaintRef color);

EXPORT ToveSubpathRef NewSubpath();
EXPORT ToveSubpathRef CloneSubpath(ToveSubpathRef trajectory);
EXPORT int SubpathGetNumCurves(ToveSubpathRef trajectory);
EXPORT int SubpathGetNumPoints(ToveSubpathRef trajectory);
EXPORT bool SubpathIsClosed(ToveSubpathRef trajectory);
EXPORT const float *SubpathGetPoints(ToveSubpathRef trajectory);
EXPORT void SubpathSetPoint(ToveSubpathRef trajectory, int index, float x, float y);
EXPORT void SubpathSetPoints(ToveSubpathRef trajectory, const float *pts, int npts);
EXPORT float SubpathGetCurveValue(ToveSubpathRef trajectory, int curve, int index);
EXPORT void SubpathSetCurveValue(ToveSubpathRef trajectory, int curve, int index, float value);
EXPORT float SubpathGetPtValue(ToveSubpathRef trajectory, int index, int dim);
EXPORT void SubpathSetPtValue(ToveSubpathRef trajectory, int index, int dim, float value);
EXPORT void SubpathInvert(ToveSubpathRef trajectory);
EXPORT void SubpathSetOrientation(ToveSubpathRef trajectory, ToveOrientation orientation);
EXPORT void SubpathClean(ToveSubpathRef trajectory, float eps);
EXPORT int SubpathMoveTo(ToveSubpathRef trajectory, float x, float y);
EXPORT int SubpathLineTo(ToveSubpathRef trajectory, float x, float y);
EXPORT int SubpathCurveTo(ToveSubpathRef trajectory, float cpx1, float cpy1,
	float cpx2, float cpy2, float x, float y);
EXPORT int SubpathArc(ToveSubpathRef trajectory, float x, float y, float r,
	float startAngle, float endAngle, bool counterclockwise);
EXPORT int SubpathDrawRect(ToveSubpathRef trajectory, float x, float y,
	float w, float h, float rx, float ry);
EXPORT int SubpathDrawEllipse(ToveSubpathRef trajectory, float x, float y, float rx, float ry);
EXPORT float SubpathGetCommandValue(ToveSubpathRef trajectory, int command, int property);
EXPORT void SubpathSetCommandValue(ToveSubpathRef trajectory, int command, int property, float value);
EXPORT ToveVec2 SubpathGetPosition(ToveSubpathRef trajectory, float t);
EXPORT ToveVec2 SubpathGetNormal(ToveSubpathRef trajectory, float t);
EXPORT ToveNearest SubpathNearest(ToveSubpathRef trajectory, float x, float y, float dmin, float dmax);
EXPORT int SubpathInsertCurveAt(ToveSubpathRef trajectory, float t);
EXPORT void SubpathRemoveCurve(ToveSubpathRef trajectory, int curve);
EXPORT int SubpathMould(ToveSubpathRef trajectory, float t, float x, float y);
EXPORT bool SubpathIsEdgeAt(ToveSubpathRef trajectory, int k);
EXPORT void SubpathMove(ToveSubpathRef trajectory, int k, float x, float y);
EXPORT void ReleaseSubpath(ToveSubpathRef trajectory);

EXPORT TovePathRef NewPath(const char *d);
EXPORT TovePathRef ClonePath(TovePathRef path);
EXPORT ToveSubpathRef PathBeginSubpath(TovePathRef path);
EXPORT void PathAddSubpath(TovePathRef path, ToveSubpathRef traj);
EXPORT void PathSetFillColor(TovePathRef path, TovePaintRef color);
EXPORT void PathSetLineColor(TovePathRef path, TovePaintRef color);
EXPORT TovePaintRef PathGetFillColor(TovePathRef path);
EXPORT TovePaintRef PathGetLineColor(TovePathRef path);
EXPORT void PathSetLineWidth(TovePathRef path, float width);
EXPORT void PathSetMiterLimit(TovePathRef path, float limit);
EXPORT float PathGetMiterLimit(TovePathRef path);
EXPORT void PathSetLineDash(TovePathRef path, const float *dashes, int count);
EXPORT void PathSetLineDashOffset(TovePathRef path, float offset);
EXPORT float PathGetLineWidth(TovePathRef path);
EXPORT int PathGetNumSubpaths(TovePathRef path);
EXPORT ToveSubpathRef PathGetSubpath(TovePathRef path, int i);
EXPORT void PathAnimate(TovePathRef path, TovePathRef a, TovePathRef b, float t);
EXPORT int PathGetNumCurves(TovePathRef path);
EXPORT ToveFillRule PathGetFillRule(TovePathRef path);
EXPORT void PathSetFillRule(TovePathRef path, ToveFillRule rule);
EXPORT float PathGetOpacity(TovePathRef path);
EXPORT void PathSetOpacity(TovePathRef path, float opacity);
EXPORT void PathClearChanges(TovePathRef path);
EXPORT ToveMeshResult PathTesselate(
	TovePathRef path, ToveMeshRef fillMesh, ToveMeshRef lineMesh, float scale,
	const ToveTesselationQuality *quality, ToveHoles holes, ToveMeshUpdateFlags flags);
EXPORT ToveChangeFlags PathFetchChanges(TovePathRef path, ToveChangeFlags flags);
EXPORT void PathSetOrientation(TovePathRef path, ToveOrientation orientation);
EXPORT void PathClean(TovePathRef path, float eps);
EXPORT bool PathIsInside(TovePathRef path, float x, float y);
EXPORT void ReleasePath(TovePathRef path);

EXPORT ToveGraphicsRef NewGraphics(const char *svg, const char* units, float dpi);
EXPORT ToveGraphicsRef CloneGraphics(ToveGraphicsRef graphics);
EXPORT ToveSubpathRef GraphicsBeginSubpath(ToveGraphicsRef graphics);
EXPORT void GraphicsCloseSubpath(ToveGraphicsRef graphics);
EXPORT void GraphicsInvertSubpath(ToveGraphicsRef graphics);
EXPORT TovePathRef GraphicsGetCurrentPath(ToveGraphicsRef shape);
EXPORT void GraphicsAddPath(ToveGraphicsRef shape, TovePathRef path);
EXPORT int GraphicsGetNumPaths(ToveGraphicsRef shape);
EXPORT TovePathRef GraphicsGetPath(ToveGraphicsRef shape, int i);
EXPORT ToveChangeFlags GraphicsFetchChanges(ToveGraphicsRef shape, ToveChangeFlags flags);
EXPORT void GraphicsSetFillColor(ToveGraphicsRef shape, TovePaintRef color);
EXPORT void GraphicsSetLineColor(ToveGraphicsRef shape, TovePaintRef color);
EXPORT void GraphicsSetLineWidth(ToveGraphicsRef shape, float width);
EXPORT void GraphicsSetMiterLimit(ToveGraphicsRef shape, float limit);
EXPORT void GraphicsSetLineDash(ToveGraphicsRef shape, const float *dashes, int count);
EXPORT void GraphicsSetLineDashOffset(ToveGraphicsRef shape, float offset);
EXPORT void GraphicsFill(ToveGraphicsRef shape);
EXPORT void GraphicsStroke(ToveGraphicsRef shape);
EXPORT ToveBounds GraphicsGetBounds(ToveGraphicsRef shape, bool exact);
EXPORT void GraphicsSet(ToveGraphicsRef graphics, ToveGraphicsRef source,
	bool scaleLineWidth, float tx, float ty, float r, float sx, float sy,
	float ox, float oy, float kx, float ky);
EXPORT ToveMeshResult GraphicsTesselate(ToveGraphicsRef shape, ToveMeshRef mesh,
	float scale, const ToveTesselationQuality *quality, ToveHoles holes,
	ToveMeshUpdateFlags flags);
EXPORT ToveImageRecord GraphicsRasterize(ToveGraphicsRef shape, int width,
	int height, float tx, float ty, float scale, const ToveTesselationQuality *quality);
EXPORT void GraphicsAnimate(ToveGraphicsRef shape, ToveGraphicsRef a, ToveGraphicsRef b, float t);
EXPORT void GraphicsSetOrientation(ToveGraphicsRef shape, ToveOrientation orientation);
EXPORT void GraphicsClean(ToveGraphicsRef shape, float eps);
EXPORT TovePathRef GraphicsHit(ToveGraphicsRef graphics, float x, float y);
EXPORT void ReleaseGraphics(ToveGraphicsRef shape);

EXPORT ToveShaderLinkRef NewColorShaderLink();
EXPORT ToveShaderLinkRef NewGeometryShaderLink(TovePathRef path, bool enableFragmentShaderStrokes);
EXPORT ToveChangeFlags ShaderLinkBeginUpdate(ToveShaderLinkRef link, TovePathRef path, bool initial);
EXPORT ToveChangeFlags ShaderLinkEndUpdate(ToveShaderLinkRef link, TovePathRef path, bool initial);
EXPORT ToveShaderData *ShaderLinkGetData(ToveShaderLinkRef link);
EXPORT ToveShaderLineFillColorData *ShaderLinkGetColorData(ToveShaderLinkRef link);
EXPORT void ReleaseShaderLink(ToveShaderLinkRef link);

EXPORT ToveMeshRef NewMesh();
EXPORT ToveMeshRef NewColorMesh();
EXPORT ToveFloatArray MeshGetVertices(ToveMeshRef mesh);
EXPORT ToveColorArray MeshGetColors(ToveMeshRef mesh);
EXPORT ToveIndex16Array MeshGetIndices(ToveMeshRef mesh);
EXPORT void MeshCache(ToveMeshRef mesh, bool keyframe);
EXPORT void ReleaseMesh(ToveMeshRef mesh);
