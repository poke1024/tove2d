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

BEGIN_TOVE_NAMESPACE

class AbstractMeshifier {
public:
	virtual ToveMeshUpdateFlags pathToMesh(
		Graphics *graphics,
		const PathRef &path,
		const MeshRef &fill,
		const MeshRef &line,
		int &fillIndex,
		int &lineIndex) = 0;

	virtual ToveMeshUpdateFlags graphicsToMesh(
		const GraphicsRef &graphics,
		const MeshRef &fill,
		const MeshRef &line);

	virtual ClipperLib::Paths toClipPath(
		const std::vector<PathRef> &paths) const = 0;

	virtual bool hasFixedSize() const = 0;
};

class AdaptiveMeshifier : public AbstractMeshifier {
private:
	void renderStrokes(
		Graphics *graphics,
		const PathRef &path,
		const ClipperLib::PolyNode *node,
		ClipperPaths &holes,
		Submesh *submesh);

	AdaptiveFlattener flattener;

public:
	AdaptiveMeshifier(float scale, const ToveTesselationQuality &quality);

	virtual ToveMeshUpdateFlags pathToMesh(
		Graphics *graphics,
		const PathRef &path,
		const MeshRef &fill,
		const MeshRef &line,
		int &fillIndex,
		int &lineIndex);

	virtual ClipperLib::Paths toClipPath(
		const std::vector<PathRef> &paths) const;

	virtual bool hasFixedSize() const;
};

class FixedMeshifier : public AbstractMeshifier {
private:
	int _nvertices;
	const int depth;
	const ToveMeshUpdateFlags _update;
	const ToveHoles holes;

public:
	FixedMeshifier(float scale, int recursionLimit,
		ToveHoles holes, ToveMeshUpdateFlags update);

	virtual ToveMeshUpdateFlags pathToMesh(
		Graphics *graphics,
		const PathRef &path,
		const MeshRef &fill,
		const MeshRef &line,
		int &fillIndex,
		int &lineIndex);

	virtual ClipperLib::Paths toClipPath(
		const std::vector<PathRef> &paths) const;

	virtual bool hasFixedSize() const;
};

END_TOVE_NAMESPACE

#endif // __TOVE_MESH_MESHIFIER
