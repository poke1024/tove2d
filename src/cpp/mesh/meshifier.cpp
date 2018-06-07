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

AdaptiveMeshifier::AdaptiveMeshifier(
	float scale, const ToveTesselationQuality *quality, ToveHoles holes) :
	tess(scale, quality), holes(holes) {
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
				mesh->add(paths, paint, HOLES_NONE);
			}
		}
		holes.clear();
	}
}

ToveMeshUpdateFlags AdaptiveMeshifier::operator()(
	const PathRef &path, const MeshRef &fill, const MeshRef &line) {

	fill->clearTriangles();
	line->clearTriangles();

	NSVGshape *shape = path->getNSVG();

	if ((shape->flags & NSVG_FLAGS_VISIBLE) == 0) {
		return 0;
	}

	if (shape->fill.type == NSVG_PAINT_NONE && shape->stroke.type == NSVG_PAINT_NONE) {
		return 0;
	}

	Tesselation t;
	tess(shape, t);

	const float scale = tess.scale;

	if (!t.fill.empty() && shape->fill.type != NSVG_PAINT_NONE) {
		MeshPaint paint;
		fill->initializePaint(paint, shape->fill, shape->opacity, scale);
		fill->add(t.fill, paint, holes);
	}

	if (t.stroke.ChildCount() > 0 && shape->stroke.type != NSVG_PAINT_NONE && shape->strokeWidth > 0.0) {
		MeshPaint paint;
		line->initializePaint(paint, shape->stroke, shape->opacity, scale);
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
	_nvertices(0),
	scale(scale),
	depth(std::min(MAX_FLATTEN_RECURSIONS, quality->recursionLimit)),
	holes(holes),
	_update(update) {
}

ToveMeshUpdateFlags FixedMeshifier::operator()(const PathRef &path, const MeshRef &fill, const MeshRef &line) {
	NSVGshape *shape = path->getNSVG();

	if ((shape->flags & NSVG_FLAGS_VISIBLE) == 0) {
		return _update;
	}

	if (shape->fill.type == NSVG_PAINT_NONE && shape->stroke.type == NSVG_PAINT_NONE) {
		return _update;
	}

	const bool hasStroke = shape->stroke.type != NSVG_PAINT_NONE && shape->strokeWidth > 0.0;

	FixedFlattener flattener(scale, depth, 0.0);

	const int n = path->getNumTrajectories();
	int index0 = _nvertices;
	int index = 0;

	ToveMeshUpdateFlags update = _update;
	bool triangleCacheFlag = false;

	const bool compound = (&fill == &line);
	int linebase = 0;

	// in case of stroke-only paths, we still produce the fill vertices for
	// computing the stroke vertices, but then skip fill triangulation.

	if (update & (UPDATE_MESH_VERTICES | UPDATE_MESH_TRIANGLES)) {
		const float lineOffset = shape->strokeWidth * 0.5;

		if (compound) {
			for (int i = 0; i < n; i++) {
				linebase += flattener.size(&path->getTrajectory(i)->nsvg);
			}
		}

		for (int i = 0; i < n; i++) {
			int k = flattener.flatten(
				&path->getTrajectory(i)->nsvg, fill, index0 + index);

			if (hasStroke) {
				const bool closed = path->getTrajectory(i)->isClosed();

				float *outer = line->vertices(
					index0 + linebase + index * 2, k * 2);
				float *inner = outer + k * 2;

				const float *vertices = fill->vertices(index0 + index, k);

				// only supports mitered lines for now.

				int previous = find_unequal_backward(vertices, k, k);
				int next = find_unequal_forward(vertices, 0, k);

				for (int j = 0; j < k; j++) {
					if (j == next) {
						previous = next - 1;
						next = find_unequal_forward(vertices, j, k);
					}

					float x0 = vertices[previous * 2 + 0];
					float y0 = vertices[previous * 2 + 1];

					float x1 = vertices[j * 2 + 0];
					float y1 = vertices[j * 2 + 1];

					float x2 = vertices[next * 2 + 0];
					float y2 = vertices[next * 2 + 1];

					float dx10 = x1 - x0;
					float dy10 = y1 - y0;
					float d10 = sqrt(dx10 * dx10 + dy10 * dy10);
					dx10 /= d10;
					dy10 /= d10;

					float dx21 = x2 - x1;
					float dy21 = y2 - y1;
					float d21 = sqrt(dx21 * dx21 + dy21 * dy21);
					dx21 /= d21;
					dy21 /= d21;

					float tx = dx10 + dx21;
					float ty = dy10 + dy21;
					float td = sqrt(tx * tx + ty * ty);
					tx /= td;
					ty /= td;

					float mx = -ty;
					float my = tx;

					float nx = -dy10;
					float ny = dx10;

					float l_inner = lineOffset / (mx * nx + my * ny);
					float l_outer = l_inner;

					// this miter limit application is a hack, as it does not produce
					// a bevel as it should (we'd need additional vertices here, which
					// we don't have). still - it's much better than having no limit.
					l_outer = std::min(l_outer, shape->miterLimit);
					l_inner = std::min(l_inner, lineOffset * 2);

					float direction = dx21 * nx + dy21 * ny;
					if (direction < 0.0f) {
						std::swap(l_inner, l_outer);
					}

					if (!closed) {
						if (j == 0) {
							l_inner = lineOffset;
							l_outer = lineOffset;
							mx = -dy21;
							my = dx21;
						} else if (j == k - 1) {
							l_inner = lineOffset;
							l_outer = lineOffset;
							mx = -dy10;
							my = dx10;
						}
					}

					outer[j * 2 + 0] = x1 - l_outer * mx;
					outer[j * 2 + 1] = y1 - l_outer * my;

					inner[j * 2 + 0] = x1 + l_inner * mx;
					inner[j * 2 + 1] = y1 + l_inner * my;
				}

#if 0
				for (int j = 0; j < k; j++) {
					printf("vertex %d  %f %f\n", j, vertices[j * 2 + 0], vertices[j * 2 + 1]);
				}
				for (int j = 0; j < k; j++) {
					printf("outer %d  %f %f\n", j, outer[j * 2 + 0], outer[j * 2 + 1]);
				}
				for (int j = 0; j < k; j++) {
					printf("inner %d  %f %f\n", j, inner[j * 2 + 0], inner[j * 2 + 1]);
				}
#endif
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
			index += path->getTrajectorySize(i, flattener);
		}
		if (compound) {
			linebase = index;
		}
	}

	if (shape->fill.type != NSVG_PAINT_NONE) {
		if (update & UPDATE_MESH_TRIANGLES) {
			fill->triangulateFill(_nvertices, path, flattener, holes);
		}
		if (update & UPDATE_MESH_COLORS) {
			MeshPaint paint;
			fill->initializePaint(paint, shape->fill, shape->opacity, scale);
			fill->addColor(_nvertices, index, paint);
		}
	}

	if (hasStroke) {
		int v0;
		if (!compound) {
			v0 = _nvertices * 2;
		} else {
			v0 = _nvertices + linebase;
		}

		if (update & UPDATE_MESH_TRIANGLES) {
			line->triangulateLine(v0, path, flattener);
		}
		if (update & UPDATE_MESH_COLORS) {
			MeshPaint paint;
			line->initializePaint(paint, shape->stroke, shape->opacity, scale);
			line->addColor(v0, index * 2, paint);
		}
	}

	if (!compound) {
		_nvertices += index;
	} else {
		_nvertices += index * 3;
	}

	if (triangleCacheFlag) {
		update |= UPDATE_MESH_TRIANGLES;
	}

	return update;
}
