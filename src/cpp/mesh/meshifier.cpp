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
#include "meshifier.h"
#include "mesh.h"

BEGIN_TOVE_NAMESPACE

void AbstractTesselator::beginTesselate(
	Graphics *graphics,
	float scale) {

	this->graphics = graphics;
}

void AbstractTesselator::endTesselate() {
	this->graphics = nullptr;
}

ToveMeshUpdateFlags AbstractTesselator::graphicsToMesh(
	Graphics *graphics,
	ToveMeshUpdateFlags update, // UPDATE_MESH_EVERYTHING
	const MeshRef &fill,
	const MeshRef &line) {

	const int n = graphics->getNumPaths();

	if (!hasFixedSize()) {
		fill->clear();
		line->clear();
	}

	const float *bounds = graphics->getBounds();
	const float extent = std::max(
		bounds[2] - bounds[0], bounds[3] - bounds[1]);

	beginTesselate(graphics, 1.0f / extent);

	ToveMeshUpdateFlags updated = 0;
	int fillIndex = 0;
	int lineIndex = 0;

	for (int i = 0; i < n; i++) {
		updated |= pathToMesh(
			update,
			graphics->getPath(i),
			fill, line,
			fillIndex, lineIndex);
	}

	if (&fill != &line) {
		fill->clip(fillIndex);
	}
	line->clip(lineIndex);

	endTesselate();

	return updated;
}


static void clip(
	const Graphics *graphics,
	const PathRef &path,
	ClipperPaths &subject) {

#ifdef NSVG_CLIP_PATHS
	const std::vector<TOVEclipPathIndex> &clipIndices =
		path->getClipIndices();

	if (!clipIndices.empty()) {
		for (TOVEclipPathIndex i : clipIndices) {
			ClipperLib::Clipper c;
			c.AddPaths(
				subject,
				ClipperLib::ptSubject,
				true);
			c.AddPaths(
				graphics->getClipAtIndex(i)->computed,
				ClipperLib::ptClip,
				true);
			c.Execute(
				ClipperLib::ctIntersection,
				subject);
		}
	}
#endif
}

AdaptiveTesselator::AdaptiveTesselator(
	AbstractAdaptiveFlattener *flattener) :
	flattener(flattener) {
}

AdaptiveTesselator::~AdaptiveTesselator() {
	delete flattener;
}

void AdaptiveTesselator::beginTesselate(
	Graphics *graphics,
	float scale) {

	AbstractTesselator::beginTesselate(graphics, scale);

	flattener->configure(scale);

	graphics->computeClipPaths(*this);
}

bool AdaptiveTesselator::hasFixedSize() const {
	return false;
}

void AdaptiveTesselator::renderStrokes(
	const PathRef &path,
	const ClipperLib::PolyNode *node,
	ClipperPaths &holes,
	Submesh *submesh) {

	const NSVGshape *shape = path->getNSVG();

	for (int i = 0; i < node->ChildCount(); i++) {
		renderStrokes(path, node->Childs[i], holes, submesh);
	}

	if (node->IsHole()) {
		if (!node->Contour.empty()) {
			holes.push_back(node->Contour);
		}
	} else {
		if (!node->Contour.empty() && shape->stroke.type == NSVG_PAINT_COLOR) {
			ClipperPaths paths;
			paths.push_back(node->Contour);
			paths.insert(paths.end(), holes.begin(), holes.end());
			clip(graphics, path, paths);
			submesh->addClipperPaths(
				paths, flattener->getClipperScale(), TOVE_HOLES_CW);
		}
		holes.clear();
	}
}

ToveMeshUpdateFlags AdaptiveTesselator::pathToMesh(
	ToveMeshUpdateFlags update,
	const PathRef &path,
	const MeshRef &fill,
	const MeshRef &line,
	int &fillIndex,
	int &lineIndex) {

	assert(fillIndex == fill->getVertexCount());
	assert(lineIndex == line->getVertexCount());

	const NSVGshape *shape = path->getNSVG();

	if ((shape->flags & NSVG_FLAGS_VISIBLE) == 0) {
		return UPDATE_MESH_EVERYTHING;
	}

	if (shape->fill.type == NSVG_PAINT_NONE &&
		shape->stroke.type == NSVG_PAINT_NONE) {
		return UPDATE_MESH_EVERYTHING;
	}

	Tesselation t;
	flattener->flatten(path, t);
	// ClosedPathsFromPolyTree

	if (!t.fill.empty() && shape->fill.type != NSVG_PAINT_NONE) {
		clip(graphics, path, t.fill);
		const int index0 = fill->getVertexCount();
 		// ClipperLib always gives us TOVE_HOLES_CW.
 		fill->submesh(path, 0)->addClipperPaths(
			t.fill, flattener->getClipperScale(), TOVE_HOLES_CW);
		fill->setFillColor(path, index0, fill->getVertexCount() - index0);
	}

	if (t.stroke.ChildCount() > 0 &&
		shape->stroke.type != NSVG_PAINT_NONE && shape->strokeWidth > 0.0) {
		const int index0 = line->getVertexCount();
		ClipperPaths holes;
		renderStrokes(path, &t.stroke, holes, line->submesh(path, 1));
		line->setLineColor(path, index0, line->getVertexCount() - index0);
	}

	fillIndex = fill->getVertexCount();
	lineIndex = line->getVertexCount();

	return UPDATE_MESH_EVERYTHING;
}

ClipperLib::Paths AdaptiveTesselator::toClipPath(
	const std::vector<PathRef> &paths) const {

	ClipperLib::Paths flattened;

	for (const PathRef &path : paths) {
		Tesselation t;
		flattener->flatten(path, t);
		ClipperLib::SimplifyPolygons(
			t.fill, path->getClipperFillType());

		if (paths.size() == 1) {
			return t.fill;
		}

		flattened.insert(
			flattened.end(),
			std::make_move_iterator(t.fill.begin()),
			std::make_move_iterator(t.fill.end()));
	}
	return flattened;
}

RigidTesselator::RigidTesselator(
	int subdivisions,
	ToveHoles holes) :

	flattener(subdivisions, 0.0),
	holes(holes) {
}

ToveMeshUpdateFlags RigidTesselator::pathToMesh(
	ToveMeshUpdateFlags _update,
	const PathRef &path,
	const MeshRef &fill,
	const MeshRef &line,
	int &fillIndex,
	int &lineIndex) {

	NSVGshape *shape = path->getNSVG();

	if ((shape->flags & NSVG_FLAGS_VISIBLE) == 0) {
		return _update;
	}

	if (shape->fill.type == NSVG_PAINT_NONE &&
		shape->stroke.type == NSVG_PAINT_NONE) {
		return _update;
	}

	const bool hasStroke = path->hasStroke();
	const int n = path->getNumSubpaths();
	const bool compound = (&fill == &line);
	assert(!compound || fillIndex == lineIndex);
	const int fillIndex0 = fillIndex;
	const int lineIndex0 = lineIndex;
	int index = 0;
	int lineBase = 0;

	Submesh *fillSubmesh = fill->submesh(path, 0);
	Submesh *lineSubmesh = line->submesh(path, 1);

	ToveMeshUpdateFlags update = _update;
	bool trianglesChanged = false;

	const float miterLimit = path->getMiterLimit();
	const bool miter = path->getLineJoin() == TOVE_LINEJOIN_MITER &&
		miterLimit > 0.0f;
	const bool reduceOverlap = false;

	const int verticesPerSegment = 4 + (miter ? 1 : 0) + (reduceOverlap ? 1 : 0);

	// in case of stroke-only paths, we still produce the fill vertices for
	// computing the stroke vertices, but then skip fill triangulation.

	if (update & (UPDATE_MESH_VERTICES | UPDATE_MESH_TRIANGLES)) {
		const float lineOffset = shape->strokeWidth * 0.5f;

		if (compound) {
			lineBase += path->getFlattenedSize(flattener);
		}

		for (int i = 0; i < n; i++) {
			const int k = flattener.flatten(
				path->getSubpath(i), fill, fillIndex0 + index);

			if (k == 0) {
				continue;
			}

			if (hasStroke) {
				const bool closed = path->getSubpath(i)->isClosed();
				const float miterLimitSquared = miterLimit * miterLimit;

				// note: for compound mode, the call to fill->vertices
				// needs to be second here; otherwise the vertices ptr
				// might get invalidated by a realloc in line->vertices.

				const int subpathLineVertex = lineIndex0 + lineBase + index * verticesPerSegment;
				auto out = line->vertices(subpathLineVertex, k * verticesPerSegment);

				const auto vertices = fill->vertices(
					fillIndex0 + index, k);

				int previous = find_unequal_backward(vertices, k, k);
				int next = find_unequal_forward(vertices, 0, k);
				int numSegments = (next - (previous + 1) + k) % k;

				for (int j = 0; j < k; j++) {
					if (j == next) {
						previous = next - 1;
						next = find_unequal_forward(vertices, j, k);
						numSegments = (next - (previous + 1) + k) % k;
					}

					const float x0 = vertices[previous].x;
					const float y0 = vertices[previous].y;

					const float x1E = vertices[j].x;
					const float y1E = vertices[j].y;

					const float x2E = vertices[next].x;
					const float y2E = vertices[next].y;

					float dx21 = x2E - x1E;
					float dy21 = y2E - y1E;

#if 1
					const int segmentIndex = (j - (previous + 1) + k) % k;
					const float segmentT1 = segmentIndex / (float)numSegments;
					const float segmentT2 = (segmentIndex + 1) / (float)numSegments;

					const float x2 = x1E + dx21 * segmentT2;
					const float y2 = y1E + dy21 * segmentT2;
					const float x1 = x1E + dx21 * segmentT1;
					const float y1 = y1E + dy21 * segmentT1;
#else
					const float x1 = x1E, y1 = y1E;
					const float x2 = x2E, y2 = y2E;
#endif

					const float d21 = std::sqrt(dx21 * dx21 + dy21 * dy21);
					dx21 /= d21;
					dy21 /= d21;

					const float ox = -dy21 * lineOffset;
					const float oy = dx21 * lineOffset;
	
					if (miter) {
						float dx10 = x1 - x0;
						float dy10 = y1 - y0;
						const float d10 = std::sqrt(dx10 * dx10 + dy10 * dy10);
						dx10 /= d10;
						dy10 /= d10;

						const float n10x = -dy10;
						const float n10y = dx10;

						const float n21x = -dy21;
						const float n21y = dx21;

						const float mx = (n10x + n21x) * 0.5f;
						const float my = (n10y + n21y) * 0.5f;

						const float wind = -n21y * n10x - n21x * -n10y;
						const float windSign = std::copysign(1.0f, wind);

						if ((mx * mx + my * my) * miterLimitSquared < 1.0f) {
							*out = vec2(x1 + ox * windSign, y1 + oy * windSign); // use bevel
						} else {
							const float l = (lineOffset / (mx * n10x + my * n10y)) * windSign;
							*out = vec2(x1 + l * mx, y1 + l * my);
						}
						out++;
					}

#if TOVE_RT_CLIP_PATH
					if (reduceOverlap) {
						*out++ = vec2(x1, y1);
					}
#endif

					*out++ = vec2(x1 - ox, y1 - oy);
					*out++ = vec2(x1 + ox, y1 + oy);

					*out++ = vec2(x2 - ox, y2 - oy);
					*out++ = vec2(x2 + ox, y2 + oy);
				}

#if TOVE_RT_CLIP_PATH
				if (reduceOverlap) {
					unoverlap(
						line->vertices(subpathLineVertex, k * verticesPerSegment),
						k, verticesPerSegment, miter);
				}
#endif
			}

			index += k;
		}

		if ((update & UPDATE_MESH_TRIANGLES) == 0 &&
			(update & UPDATE_MESH_AUTO_TRIANGLES)) {
			if (!fillSubmesh->findCachedTriangulation(trianglesChanged)) {
				update |= UPDATE_MESH_TRIANGLES;
			}
		}
	} else {
		index += path->getFlattenedSize(flattener);
		if (compound) {
			lineBase = index;
		}
	}

	if (shape->fill.type != NSVG_PAINT_NONE) {
		if (update & UPDATE_MESH_TRIANGLES) {
			fillSubmesh->triangulateFixedResolutionFill(
				fillIndex0, path, flattener, holes);
		}
		if (update & (UPDATE_MESH_COLORS | UPDATE_MESH_VERTICES)) {
			fill->setFillColor(path, fillIndex0, index);
		}
	}

	if (hasStroke) {
		const int v0 = lineIndex0 + lineBase;

		if (update & UPDATE_MESH_TRIANGLES) {
			lineSubmesh->triangulateFixedResolutionLine(
				v0, miter, reduceOverlap, verticesPerSegment, path, flattener);
		}
		if (update & (UPDATE_MESH_COLORS | UPDATE_MESH_VERTICES)) {
			line->setLineColor(path, v0, index * verticesPerSegment);
		}
	}

	if (trianglesChanged) {
		update |= UPDATE_MESH_TRIANGLES;
	}

	lineIndex = lineIndex0 + lineBase + index * verticesPerSegment;
	fillIndex = compound ? lineIndex : fillIndex0 + index;

	return update;
}

ClipperLib::Paths RigidTesselator::toClipPath(
	const std::vector<PathRef> &paths) const {

	return ClipperPaths();
}

bool RigidTesselator::hasFixedSize() const {
	return true;
}

END_TOVE_NAMESPACE
