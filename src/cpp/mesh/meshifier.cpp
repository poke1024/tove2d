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

ToveMeshUpdateFlags AbstractMeshifier::graphics(const GraphicsRef &graphics,
	const MeshRef &fill, const MeshRef &line) {

	fill->clear();
	line->clear();

	const int n = graphics->getNumPaths();
	ToveMeshUpdateFlags updated = 0;
	for (int i = 0; i < n; i++) {
		updated |= (*this)(graphics->getPath(i), fill, line, true);
	}
	return updated;
}


AdaptiveMeshifier::AdaptiveMeshifier(
	float scale, const ToveTesselationQuality *quality) :
	tess(scale, quality) {
}

void AdaptiveMeshifier::renderStrokes(
	NSVGshape *shape,
	const ClipperLib::PolyNode *node,
	ClipperPaths &holes,
	const MeshPaint &paint,
	const MeshRef &mesh) {

	for (int i = 0; i < node->ChildCount(); i++) {
		renderStrokes(shape, node->Childs[i], holes, paint, mesh);
	}

	if (node->IsHole()) {
		if (!node->Contour.empty()) {
			holes.push_back(node->Contour);
		}
	} else {
		if (!node->Contour.empty()) {
			ClipperPaths paths;
			paths.push_back(node->Contour);
			paths.insert(paths.end(), holes.begin(), holes.end());

			if (shape->stroke.type == NSVG_PAINT_COLOR) {
				mesh->add(paths, tess.scale, paint, HOLES_CW);
			}
		}
		holes.clear();
	}
}

ToveMeshUpdateFlags AdaptiveMeshifier::operator()(
	const PathRef &path, const MeshRef &fill, const MeshRef &line, bool append) {

	if (!append) {
		fill->clear();
		line->clear();
	}

	NSVGshape *shape = path->getNSVG();

	if ((shape->flags & NSVG_FLAGS_VISIBLE) == 0) {
		return 0;
	}

	if (shape->fill.type == NSVG_PAINT_NONE &&
		shape->stroke.type == NSVG_PAINT_NONE) {
		return 0;
	}

	Tesselation t;
	tess(shape, t);

	if (!t.fill.empty() && shape->fill.type != NSVG_PAINT_NONE) {
		MeshPaint paint;
		fill->initializePaint(paint, shape->fill, shape->opacity, 1.0f);
 		// ClipperLib always gives us HOLES_CW.
 		fill->add(t.fill, tess.scale, paint, HOLES_CW);
	}

	if (t.stroke.ChildCount() > 0 &&
		shape->stroke.type != NSVG_PAINT_NONE && shape->strokeWidth > 0.0) {
		MeshPaint paint;
		line->initializePaint(paint, shape->stroke, shape->opacity, 1.0f);
		ClipperPaths holes;
		renderStrokes(shape, &t.stroke, holes, paint, line);
	}

	return 0;
}

FixedMeshifier::FixedMeshifier(
	float scale,
	const ToveTesselationQuality *quality,
	ToveHoles holes,
	ToveMeshUpdateFlags update) :
	depth(std::min(MAX_FLATTEN_RECURSIONS, quality->recursionLimit)),
	holes(holes),
	_update(update) {
	assert(scale == 1.0f);
}

ToveMeshUpdateFlags FixedMeshifier::operator()(
	const PathRef &path,
	const MeshRef &fill,
	const MeshRef &line,
	bool append) {

	NSVGshape *shape = path->getNSVG();

	if ((shape->flags & NSVG_FLAGS_VISIBLE) == 0) {
		return _update;
	}

	if (shape->fill.type == NSVG_PAINT_NONE &&
		shape->stroke.type == NSVG_PAINT_NONE) {
		return _update;
	}

	const bool hasStroke = path->hasStroke();

	FixedFlattener flattener(depth, 0.0);

	const int n = path->getNumSubpaths();
	const bool compound = (&fill == &line);
	const int index0 = compound && append ? fill->getVertexCount() : 0;
	int index = 0;
	int linebase = 0;

	ToveMeshUpdateFlags update = _update;
	bool triangleCacheFlag = false;

	float miterLimit = path->getMiterLimit();
	bool miter = path->getLineJoin() == LINEJOIN_MITER && miterLimit > 0.0f;

	// in case of stroke-only paths, we still produce the fill vertices for
	// computing the stroke vertices, but then skip fill triangulation.

	if (update & (UPDATE_MESH_VERTICES | UPDATE_MESH_TRIANGLES)) {
		const float lineOffset = shape->strokeWidth * 0.5f;

		if (compound) {
			for (int i = 0; i < n; i++) {
				linebase += path->getSubpathSize(i, flattener);
			}
		}

		for (int i = 0; i < n; i++) {
			int k = flattener.flatten(
				&path->getSubpath(i)->nsvg, fill, index0 + index);

			if (hasStroke) {
				const bool closed = path->getSubpath(i)->isClosed();
				const int verticesPerSegment = miter ? 5 : 4;
				const float miterLimitSquared = miterLimit * miterLimit;

				// note: for compound mode, the call to fill->vertices
				// needs to be second here; otherwise the vertices ptr
				// might get invalidated by a realloc in line->vertices.

				float *vertex = line->vertices(
					index0 + linebase + index * verticesPerSegment,
					k * verticesPerSegment);

				const float *vertices = fill->vertices(index0 + index, k);

				int previous = find_unequal_backward(vertices, k, k);
				int next = find_unequal_forward(vertices, 0, k);

				for (int j = 0; j < k; j++) {
					if (j == next) {
						previous = next - 1;
						next = find_unequal_forward(vertices, j, k);
					}

					const float x0 = vertices[previous * 2 + 0];
					const float y0 = vertices[previous * 2 + 1];

					const float x1 = vertices[j * 2 + 0];
					const float y1 = vertices[j * 2 + 1];

					const float x2 = vertices[next * 2 + 0];
					const float y2 = vertices[next * 2 + 1];

					float dx21 = x2 - x1;
					float dy21 = y2 - y1;
					float d21 = std::sqrt(dx21 * dx21 + dy21 * dy21);
					dx21 /= d21;
					dy21 /= d21;

					const float ox = -dy21 * lineOffset;
					const float oy = dx21 * lineOffset;

					if (miter) {
						float dx10 = x1 - x0;
						float dy10 = y1 - y0;
						float d10 = sqrt(dx10 * dx10 + dy10 * dy10);
						dx10 /= d10;
						dy10 /= d10;

						const float n10x = -dy10;
						const float n10y = dx10;

						const float n21x = -dy21;
						const float n21y = dx21;

						const float mx = (n10x + n21x) * 0.5f;
						const float my = (n10y + n21y) * 0.5f;

						const float wind = -n21y * n10x - n21x * -n10y;

						if ((mx * mx + my * my) * miterLimitSquared < 1.0f) {
							// use bevel
							if (wind < 0.0f) {
								*vertex++ = x1 - ox;
								*vertex++ = y1 - oy;
							} else {
								*vertex++ = x1 + ox;
								*vertex++ = y1 + oy;
							}
						} else {
							float l = lineOffset / (mx * n10x + my * n10y);
							l = l * (wind < 0.0f ? -1.0f : 1.0f);

							*vertex++ = x1 + l * mx;
							*vertex++ = y1 + l * my;
						}
					}

					// outer
					*vertex++ = x1 - ox;
					*vertex++ = y1 - oy;

					// inner
					*vertex++ = x1 + ox;
					*vertex++ = y1 + oy;

					// outer
					*vertex++ = x2 - ox;
					*vertex++ = y2 - oy;

					// inner
					*vertex++ = x2 + ox;
					*vertex++ = y2 + oy;
				}
			}

			index += k;
		}

		if ((update & UPDATE_MESH_TRIANGLES) == 0 &&
			(update & UPDATE_MESH_AUTO_TRIANGLES)) {
			if (!fill->checkTriangles(triangleCacheFlag)) {
				update |= UPDATE_MESH_TRIANGLES;
			}
		}
	} else {
		for (int i = 0; i < n; i++) {
			index += path->getSubpathSize(i, flattener);
		}
		if (compound) {
			linebase = index;
		}
	}

	if (shape->fill.type != NSVG_PAINT_NONE) {
		if (update & UPDATE_MESH_TRIANGLES) {
			fill->triangulateFill(index0, path, flattener, holes);
		}
		if (update & UPDATE_MESH_COLORS) {
			MeshPaint paint;
			fill->initializePaint(paint, shape->fill, shape->opacity, 1.0f);
			fill->addColor(index0, index, paint);
		}
	}

	if (hasStroke) {
		const int v0 = index0 + linebase;
		const int verticesPerSegment = miter ? 5 : 4;

		if (update & UPDATE_MESH_TRIANGLES) {
			line->triangulateLine(v0, miter, path, flattener);
		}
		if (update & UPDATE_MESH_COLORS) {
			MeshPaint paint;
			line->initializePaint(paint, shape->stroke, shape->opacity, 1.0f);
			line->addColor(v0, index * verticesPerSegment, paint);
		}
	}

	if (triangleCacheFlag) {
		update |= UPDATE_MESH_TRIANGLES;
	}

	return update;
}
