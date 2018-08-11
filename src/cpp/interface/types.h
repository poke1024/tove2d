typedef void (*ToveWarningFunction)(const char *s);

typedef uint32_t ToveChangeFlags;

typedef uint32_t ToveMeshUpdateFlags;

typedef struct {
	ToveMeshUpdateFlags update;
} ToveMeshResult;

typedef enum {
	TRIANGLES_LIST,
	TRIANGLES_STRIP
} ToveTrianglesMode;

typedef enum {
	TOVE_GLSL2,
	TOVE_GLSL3
} ToveShaderLanguage;

typedef uint16_t ToveVertexIndex;

typedef struct {
	ToveTrianglesMode mode;
	const ToveVertexIndex *array;
	int size;
} ToveTriangles;

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
	CHANGED_ANYTHING = 255,
	CHANGED_COLORS = CHANGED_FILL_STYLE | CHANGED_LINE_STYLE
};

enum {
	UPDATE_MESH_VERTICES = 1,
	UPDATE_MESH_COLORS = 2,
	UPDATE_MESH_TRIANGLES = 4,
	UPDATE_MESH_GEOMETRY = 8,
	UPDATE_MESH_EVERYTHING = 15,
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

typedef enum {
	TOVE_REC_DEPTH,
	TOVE_ANTIGRAIN,
	TOVE_MAX_ERROR
} ToveStopCriterion;

typedef struct {
	ToveStopCriterion stopCriterion;
	int recursionLimit;
	float arcTolerance;
	struct {
		float distanceTolerance;
		float colinearityEpsilon;
		float angleEpsilon;
		float angleTolerance;
		float cuspLimit;
	} antigrain;
	float maximumError;
} ToveTesselationQuality;

typedef struct {
	float r, g, b, a;
} RGBA;

typedef struct {
	RGBA rgba;
	float offset;
} ToveColorStop;

typedef struct {
	float values[6];
} ToveGradientParameters;

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
	float x, y, z, w;
} ToveVec4;

typedef struct {
	float t;
	float distanceSquared;
} ToveNearest;

typedef struct {
	int16_t numPaints;
	int16_t numColors;
} TovePaintColorAllocation;

typedef struct {
	int16_t numColors; // to be removed
	int8_t matrixRows;
	float *matrix;
	ToveVec4 *arguments;
	uint8_t *colorsTexture;
	int16_t colorsTextureRowBytes;
	int16_t colorTextureHeight;
} ToveGradientData;

typedef struct {
	int8_t style; // TovePaintType
	RGBA rgba;
	ToveGradientData gradient;
} TovePaintData;

typedef struct {
	TovePaintData *array;
	int16_t size;
} TovePaintDataArray;

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
	TovePaintData line;
	TovePaintData fill;
} ToveShaderLineFillColorData;

typedef struct {
	ToveShaderLineFillColorData color;
	ToveShaderGeometryData geometry;
} ToveShaderData;

typedef struct {
	void *ptr;
} ToveFeedRef;

typedef struct {
	void *ptr;
} ToveMeshRef;
