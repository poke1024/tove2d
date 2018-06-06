typedef uint32_t ToveChangeFlags;

typedef uint32_t ToveMeshUpdateFlags;

typedef enum {
	ERR_NONE = 0,
	ERR_TRIANGULATION_FAILED,
	ERR_OUT_OF_MEMORY,
	ERR_CLOSED_TRAJECTORY,
	ERR_UNKNOWN
} ToveError;

typedef struct {
	ToveError err;
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

enum {
	CHANGED_FILL_STYLE = 1,
	CHANGED_LINE_STYLE = 2,
	CHANGED_POINTS = 4,
	CHANGED_GEOMETRY = 8,
	CHANGED_LINE_ARGS = 16,
	CHANGED_BOUNDS = 32,
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
} ToveTrajectoryRef;

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
	int maxCurves;
	int numCurves;

	ToveBounds *bounds;
	float strokeWidth;
	float miterLimit;
	int8_t fillRule;
	bool fragmentShaderStrokes;

	float *lookupTable;
	int lookupTableSize;
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

EXPORT ToveTrajectoryRef NewTrajectory();
EXPORT ToveTrajectoryRef CloneTrajectory(ToveTrajectoryRef trajectory);
EXPORT int TrajectoryGetNumCurves(ToveTrajectoryRef trajectory);
EXPORT int TrajectoryGetNumPoints(ToveTrajectoryRef trajectory);
EXPORT bool TrajectoryIsClosed(ToveTrajectoryRef trajectory);
EXPORT const float *TrajectoryGetPoints(ToveTrajectoryRef trajectory);
EXPORT void TrajectorySetPoint(ToveTrajectoryRef trajectory, int index, float x, float y);
EXPORT void TrajectorySetPoints(ToveTrajectoryRef trajectory, const float *pts, int npts);
EXPORT float TrajectoryGetCurveValue(ToveTrajectoryRef trajectory, int curve, int index);
EXPORT void TrajectorySetCurveValue(ToveTrajectoryRef trajectory, int curve, int index, float value);
EXPORT float TrajectoryGetPtValue(ToveTrajectoryRef trajectory, int index, int dim);
EXPORT void TrajectorySetPtValue(ToveTrajectoryRef trajectory, int index, int dim, float value);
EXPORT void TrajectoryInvert(ToveTrajectoryRef trajectory);
EXPORT void TrajectorySetOrientation(ToveTrajectoryRef trajectory, ToveOrientation orientation);
EXPORT void TrajectoryClean(ToveTrajectoryRef trajectory, float eps);
EXPORT int TrajectoryMoveTo(ToveTrajectoryRef trajectory, float x, float y);
EXPORT int TrajectoryLineTo(ToveTrajectoryRef trajectory, float x, float y);
EXPORT int TrajectoryCurveTo(ToveTrajectoryRef trajectory, float cpx1, float cpy1, float cpx2, float cpy2, float x, float y);
EXPORT int TrajectoryArc(ToveTrajectoryRef trajectory, float x, float y, float r, float startAngle, float endAngle, bool counterclockwise);
EXPORT int TrajectoryDrawRect(ToveTrajectoryRef trajectory, float x, float y, float w, float h, float rx, float ry);
EXPORT int TrajectoryDrawEllipse(ToveTrajectoryRef trajectory, float x, float y, float rx, float ry);
EXPORT float TrajectoryGetCommandValue(ToveTrajectoryRef trajectory, int command, int property);
EXPORT void TrajectorySetCommandValue(ToveTrajectoryRef trajectory, int command, int property, float value);
EXPORT ToveVec2 TrajectoryGetPosition(ToveTrajectoryRef trajectory, float t);
EXPORT ToveVec2 TrajectoryGetNormal(ToveTrajectoryRef trajectory, float t);
EXPORT float TrajectoryClosest(ToveTrajectoryRef trajectory, float x, float y, float dmin, float dmax);
EXPORT int TrajectoryInsertCurveAt(ToveTrajectoryRef trajectory, float t);
EXPORT void TrajectoryRemoveCurve(ToveTrajectoryRef trajectory, int curve);
EXPORT int TrajectoryMould(ToveTrajectoryRef trajectory, float t, float x, float y);
EXPORT void ReleaseTrajectory(ToveTrajectoryRef trajectory);

EXPORT TovePathRef NewPath(const char *d);
EXPORT TovePathRef ClonePath(TovePathRef path);
EXPORT ToveTrajectoryRef PathBeginTrajectory(TovePathRef path);
EXPORT void PathAddTrajectory(TovePathRef path, ToveTrajectoryRef traj);
EXPORT void PathSetFillColor(TovePathRef path, TovePaintRef color);
EXPORT void PathSetLineColor(TovePathRef path, TovePaintRef color);
EXPORT TovePaintRef PathGetFillColor(TovePathRef path);
EXPORT TovePaintRef PathGetLineColor(TovePathRef path);
EXPORT void PathSetLineWidth(TovePathRef path, float width);
EXPORT void PathSetMiterLimit(TovePathRef path, float limit);
EXPORT void PathSetLineDash(TovePathRef path, const float *dashes, int count);
EXPORT void PathSetLineDashOffset(TovePathRef path, float offset);
EXPORT float PathGetLineWidth(TovePathRef path);
EXPORT int PathGetNumTrajectories(TovePathRef path);
EXPORT ToveTrajectoryRef PathGetTrajectory(TovePathRef path, int i);
EXPORT void PathAnimate(TovePathRef path, TovePathRef a, TovePathRef b, float t);
EXPORT int PathGetNumCurves(TovePathRef path);
EXPORT ToveFillRule PathGetFillRule(TovePathRef path);
EXPORT void PathSetFillRule(TovePathRef path, ToveFillRule rule);
EXPORT float PathGetOpacity(TovePathRef path);
EXPORT void PathSetOpacity(TovePathRef path, float opacity);
EXPORT void PathClearChanges(TovePathRef path);
EXPORT ToveMeshResult PathTesselate(TovePathRef path, ToveMeshRef fillMesh, ToveMeshRef lineMesh, float scale, const ToveTesselationQuality *quality, ToveMeshUpdateFlags flags);
EXPORT ToveChangeFlags PathFetchChanges(TovePathRef path, ToveChangeFlags flags);
EXPORT void PathSetOrientation(TovePathRef path, ToveOrientation orientation);
EXPORT void PathClean(TovePathRef path, float eps);
EXPORT bool PathIsInside(TovePathRef path, float x, float y);
EXPORT void ReleasePath(TovePathRef path);

EXPORT ToveGraphicsRef NewGraphics(const char *svg, const char* units, float dpi);
EXPORT ToveGraphicsRef CloneGraphics(ToveGraphicsRef graphics);
EXPORT ToveTrajectoryRef GraphicsBeginTrajectory(ToveGraphicsRef graphics);
EXPORT void GraphicsCloseTrajectory(ToveGraphicsRef graphics);
EXPORT void GraphicsInvertTrajectory(ToveGraphicsRef graphics);
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
EXPORT ToveBounds GraphicsGetBounds(ToveGraphicsRef shape);
EXPORT void GraphicsSet(ToveGraphicsRef graphics, ToveGraphicsRef source, bool scaleLineWidth, float tx, float ty, float r, float sx, float sy, float ox, float oy, float kx, float ky);
EXPORT ToveMeshResult GraphicsTesselate(ToveGraphicsRef shape, ToveMeshRef mesh, float scale, const ToveTesselationQuality *quality, ToveMeshUpdateFlags flags);
EXPORT ToveImageRecord GraphicsRasterize(ToveGraphicsRef shape, int width, int height, float tx, float ty, float scale, const ToveTesselationQuality *quality);
EXPORT void GraphicsAnimate(ToveGraphicsRef shape, ToveGraphicsRef a, ToveGraphicsRef b, float t);
EXPORT void GraphicsSetOrientation(ToveGraphicsRef shape, ToveOrientation orientation);
EXPORT void GraphicsClean(ToveGraphicsRef shape, float eps);
EXPORT TovePathRef GraphicsHit(ToveGraphicsRef graphics, float x, float y);
EXPORT void ReleaseGraphics(ToveGraphicsRef shape);

EXPORT ToveShaderLinkRef NewColorShaderLink();
EXPORT ToveShaderLinkRef NewGeometryShaderLink(TovePathRef path);
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
