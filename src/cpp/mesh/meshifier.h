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

#ifndef __TOVE_MESH_MESHIFIER
#define __TOVE_MESH_MESHIFIER 1

#include "../graphics.h"
#include "paint.h"
#include "../../thirdparty/polypartition.h"

class AbstractMeshifier {
public:
	virtual ToveMeshUpdateFlags operator()(const PathRef &path, const MeshRef &fill, const MeshRef &line) = 0;

	virtual ToveMeshUpdateFlags graphics(const GraphicsRef &graphics, const MeshRef &fill, const MeshRef &line) {
		const int n = graphics->getNumPaths();
		ToveMeshUpdateFlags updated = 0;
		for (int i = 0; i < n; i++) {
			updated |= (*this)(graphics->getPath(i), fill, line);
		}
		return updated;
	}
};

class AdaptiveMeshifier : public AbstractMeshifier {
private:
	void renderStrokes(NSVGshape *shape, const ClipperLib::PolyNode *node, ClipperPaths &holes, const MeshPaint &paint, const MeshRef &mesh);

	AdaptiveFlattener tess;
	const ToveHoles holes;

protected:
	void mesh(const std::list<TPPLPoly> &triangles, const MeshPaint &paint);

public:
	AdaptiveMeshifier(float scale, const ToveTesselationQuality *quality, ToveHoles holes);

	virtual ToveMeshUpdateFlags operator()(const PathRef &path, const MeshRef &fill, const MeshRef &line);
};

class FixedMeshifier : public AbstractMeshifier {
private:
	int _nvertices;
	const float scale;
	const int depth;
	const ToveMeshUpdateFlags _update;
	const ToveHoles holes;

public:
	FixedMeshifier(float scale, const ToveTesselationQuality *quality, ToveHoles holes, ToveMeshUpdateFlags update);

	virtual ToveMeshUpdateFlags operator()(const PathRef &path, const MeshRef &fill, const MeshRef &line);
};

#endif // __TOVE_MESH_MESHIFIER
