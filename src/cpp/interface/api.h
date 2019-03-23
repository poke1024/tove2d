EXPORT const char *GetVersion();
EXPORT void SetReportFunction(ToveReportFunction f);
EXPORT void SetReportLevel(ToveReportLevel l);

EXPORT void DefaultRasterizeSettings(ToveRasterizeSettings *settings);

EXPORT TovePaintType PaintGetType(TovePaintRef paint);
EXPORT TovePaintRef ClonePaint(TovePaintRef paint);
EXPORT TovePaintRef NewEmptyPaint();
EXPORT int PaintGetNumColors(TovePaintRef color);
EXPORT ToveColorStop PaintGetColorStop(TovePaintRef color, int i, float opacity);
EXPORT TovePaintRef NewColor(float r, float g, float b, float a);
EXPORT void ColorSet(TovePaintRef color, float r, float g, float b, float a);
EXPORT ToveRGBA ColorGet(TovePaintRef color, float opacity);
EXPORT TovePaintRef NewLinearGradient(float x1, float y1, float x2, float y2);
EXPORT TovePaintRef NewRadialGradient(float cx, float cy, float fx, float fy, float r);
EXPORT ToveGradientParameters GradientGetParameters(TovePaintRef gradient);
EXPORT void GradientAddColorStop(TovePaintRef gradient, float offset, float r, float g, float b, float a);
EXPORT void GradientSetColorStop(TovePaintRef gradient, int i, float offset, float r, float g, float b, float a);
EXPORT void ReleasePaint(TovePaintRef color);

EXPORT ToveSubpathRef NewSubpath();
EXPORT ToveSubpathRef CloneSubpath(ToveSubpathRef subpath);
EXPORT int SubpathGetNumCurves(ToveSubpathRef subpath);
EXPORT int SubpathGetNumPoints(ToveSubpathRef subpath);
EXPORT bool SubpathIsClosed(ToveSubpathRef subpath);
EXPORT void SubpathSetIsClosed(ToveSubpathRef subpath, bool closed);
EXPORT const float *SubpathGetPointsPtr(ToveSubpathRef subpath);
EXPORT void SubpathSetPoint(ToveSubpathRef subpath, int index, float x, float y);
EXPORT void SubpathSetPoints(ToveSubpathRef subpath, const float *pts, int npts);
EXPORT float SubpathGetCurveValue(ToveSubpathRef subpath, int curve, int index);
EXPORT void SubpathSetCurveValue(ToveSubpathRef subpath, int curve, int index, float value);
EXPORT float SubpathGetPtValue(ToveSubpathRef subpath, int index, int dim);
EXPORT void SubpathSetPtValue(ToveSubpathRef subpath, int index, int dim, float value);
EXPORT void SubpathInvert(ToveSubpathRef subpath);
EXPORT void SubpathSetOrientation(ToveSubpathRef subpath, ToveOrientation orientation);
EXPORT void SubpathClean(ToveSubpathRef subpath, float eps);
EXPORT int SubpathMoveTo(ToveSubpathRef subpath, float x, float y);
EXPORT int SubpathLineTo(ToveSubpathRef subpath, float x, float y);
EXPORT int SubpathCurveTo(ToveSubpathRef subpath, float cpx1, float cpy1,
	float cpx2, float cpy2, float x, float y);
EXPORT int SubpathArc(ToveSubpathRef subpath, float x, float y, float r,
	float startAngle, float endAngle, bool counterclockwise);
EXPORT int SubpathDrawRect(ToveSubpathRef subpath, float x, float y,
	float w, float h, float rx, float ry);
EXPORT int SubpathDrawEllipse(ToveSubpathRef subpath, float x, float y, float rx, float ry);
EXPORT float SubpathGetCommandValue(ToveSubpathRef subpath, int command, int property);
EXPORT void SubpathSetCommandValue(ToveSubpathRef subpath, int command, int property, float value);
EXPORT ToveVec2 SubpathGetPosition(ToveSubpathRef subpath, float t);
EXPORT ToveVec2 SubpathGetNormal(ToveSubpathRef subpath, float t);
EXPORT ToveNearest SubpathNearest(ToveSubpathRef subpath, float x, float y, float dmin, float dmax);
EXPORT int SubpathInsertCurveAt(ToveSubpathRef subpath, float t);
EXPORT void SubpathRemoveCurve(ToveSubpathRef subpath, int curve);
EXPORT int SubpathMould(ToveSubpathRef subpath, float t, float x, float y);
EXPORT bool SubpathIsLineAt(ToveSubpathRef subpath, int k, int dir);
EXPORT void SubpathMakeFlat(ToveSubpathRef subpath, int k, int dir);
EXPORT void SubpathMakeSmooth(ToveSubpathRef subpath, int k, int dir, float a);
EXPORT void SubpathMove(ToveSubpathRef subpath, int k, float x, float y, ToveHandle handle);
EXPORT void SubpathCommit(ToveSubpathRef subpath);
EXPORT void SubpathSet(ToveSubpathRef subpath, ToveSubpathRef source,
	float a, float b, float c, float d, float e, float f);
EXPORT void ReleaseSubpath(ToveSubpathRef subpath);

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
EXPORT void PathSetOrientation(TovePathRef path, ToveOrientation orientation);
EXPORT void PathClean(TovePathRef path, float eps);
EXPORT bool PathIsInside(TovePathRef path, float x, float y);
EXPORT void PathSet(TovePathRef path, TovePathRef source,
	bool scaleLineWidth, float a, float b, float c, float d, float e, float f);
EXPORT ToveLineJoin PathGetLineJoin(TovePathRef path);
EXPORT void PathSetLineJoin(TovePathRef path, ToveLineJoin join);
EXPORT const char *PathGetId(TovePathRef path);
EXPORT void ReleasePath(TovePathRef path);

EXPORT ToveGraphicsRef NewGraphics(const char *svg, const char* units, float dpi);
EXPORT ToveGraphicsRef CloneGraphics(ToveGraphicsRef graphics);
EXPORT TovePathRef GraphicsBeginPath(ToveGraphicsRef graphics);
EXPORT void GraphicsClosePath(ToveGraphicsRef graphics);
EXPORT ToveSubpathRef GraphicsBeginSubpath(ToveGraphicsRef graphics);
EXPORT void GraphicsCloseSubpath(ToveGraphicsRef graphics, bool close);
EXPORT void GraphicsInvertSubpath(ToveGraphicsRef graphics);
EXPORT TovePathRef GraphicsGetCurrentPath(ToveGraphicsRef shape);
EXPORT void GraphicsAddPath(ToveGraphicsRef shape, TovePathRef path);
EXPORT int GraphicsGetNumPaths(ToveGraphicsRef shape);
EXPORT TovePathRef GraphicsGetPath(ToveGraphicsRef shape, int i);
EXPORT TovePathRef GraphicsGetPathByName(ToveGraphicsRef shape, const char *name);
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
	bool scaleLineWidth, float a, float b, float c, float d, float e, float f);
EXPORT void GraphicsRasterize(ToveGraphicsRef shape, uint8_t *pixels,
	int width, int height, int stride, float tx, float ty, float scale,
	const ToveRasterizeSettings *settings);
EXPORT void GraphicsAnimate(ToveGraphicsRef shape, ToveGraphicsRef a, ToveGraphicsRef b, float t);
EXPORT void GraphicsSetOrientation(ToveGraphicsRef shape, ToveOrientation orientation);
EXPORT void GraphicsClean(ToveGraphicsRef shape, float eps);
EXPORT TovePathRef GraphicsHit(ToveGraphicsRef graphics, float x, float y);
EXPORT void GraphicsClear(ToveGraphicsRef graphics);
EXPORT bool GraphicsAreColorsSolid(ToveGraphicsRef shape);
EXPORT void GraphicsClearChanges(ToveGraphicsRef shape);
EXPORT ToveLineJoin GraphicsGetLineJoin(ToveGraphicsRef shape);
EXPORT void GraphicsSetLineJoin(ToveGraphicsRef shape, ToveLineJoin join);
EXPORT void ReleaseGraphics(ToveGraphicsRef shape);

EXPORT ToveFeedRef NewColorFeed(ToveGraphicsRef graphics, float scale);
EXPORT ToveFeedRef NewGeometryFeed(TovePathRef path, bool enableFragmentShaderStrokes);
EXPORT ToveChangeFlags FeedBeginUpdate(ToveFeedRef link);
EXPORT ToveChangeFlags FeedEndUpdate(ToveFeedRef link);
EXPORT ToveShaderData *FeedGetData(ToveFeedRef link);
EXPORT TovePaintColorAllocation FeedGetColorAllocation(ToveFeedRef link);
EXPORT void FeedBind(ToveFeedRef link, const ToveGradientData *data);
EXPORT void ReleaseFeed(ToveFeedRef link);

EXPORT ToveMeshRef NewMesh();
EXPORT ToveMeshRef NewColorMesh();
EXPORT ToveMeshRef NewPaintMesh();
EXPORT int MeshGetVertexCount(ToveMeshRef mesh);
EXPORT void MeshCopyVertexData(
	ToveMeshRef mesh, void *buffer, int32_t size);
EXPORT ToveTrianglesMode MeshGetIndexMode(ToveMeshRef mesh);
EXPORT int MeshGetIndexCount(ToveMeshRef mesh);
EXPORT void MeshCopyIndexData(
	ToveMeshRef mesh, void *buffer, int32_t size);
EXPORT void MeshCache(ToveMeshRef mesh, bool keyframe);
EXPORT void ReleaseMesh(ToveMeshRef mesh);

EXPORT void ConfigureShaderCode(ToveShaderLanguage language, int matrixRows);
EXPORT const char *GetPaintShaderCode(int numPaints);
EXPORT const char *GetImplicitFillShaderCode(
	const ToveShaderData *data, bool fragLine, bool meshBand, bool debug);
EXPORT const char *GetImplicitLineShaderCode(const ToveShaderData *data);

EXPORT ToveTesselatorRef NewAdaptiveTesselator(float resolution, int recursionLimit);
EXPORT ToveTesselatorRef NewRigidTesselator(int subdivisions, ToveHoles holes);
EXPORT ToveTesselatorRef NewAntiGrainTesselator(const AntiGrainSettings *settings);
EXPORT ToveMeshUpdateFlags TesselatorTessGraphics(ToveTesselatorRef tess,
	ToveGraphicsRef graphics, ToveMeshRef mesh, ToveMeshUpdateFlags flags);
EXPORT ToveMeshUpdateFlags TesselatorTessPath(ToveTesselatorRef tess,
	ToveGraphicsRef graphics, TovePathRef path,
	ToveMeshRef fillMesh, ToveMeshRef lineMesh, ToveMeshUpdateFlags flags);
EXPORT void TesselatorSetMaxSubdivisions(int subdivisions);
EXPORT void ReleaseTesselator(ToveTesselatorRef tess);
