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

BEGIN_TOVE_NAMESPACE

class AbstractTesselator {
protected:
	const Graphics *graphics;

public:
	ToveMeshUpdateFlags graphicsToMesh(
		Graphics *graphics,
		ToveMeshUpdateFlags update,
		const MeshRef &fill,
		const MeshRef &line);

	virtual void beginTesselate(
		Graphics *graphics,
		float scale);

	void endTesselate();

	virtual ToveMeshUpdateFlags pathToMesh(
		ToveMeshUpdateFlags update,
		const PathRef &path,
		const MeshRef &fill,
		const MeshRef &line,
		int &fillIndex,
		int &lineIndex) = 0;

	virtual ClipperLib::Paths toClipPath(
		const std::vector<PathRef> &paths) const = 0;

	virtual bool hasFixedSize() const = 0;

	inline AbstractTesselator() : graphics(nullptr) {
	}

	virtual ~AbstractTesselator() {
	}
};

class AdaptiveTesselator : public AbstractTesselator {
private:
	void renderStrokes(
		const PathRef &path,
		const ClipperLib::PolyNode *node,
		ClipperPaths &holes,
		Submesh *submesh);

	AbstractAdaptiveFlattener *flattener;

public:
	AdaptiveTesselator(
		AbstractAdaptiveFlattener *flattener);

	virtual ~AdaptiveTesselator();

	virtual void beginTesselate(
		Graphics *graphics,
		float scale);

	virtual ToveMeshUpdateFlags pathToMesh(
		ToveMeshUpdateFlags update,
		const PathRef &path,
		const MeshRef &fill,
		const MeshRef &line,
		int &fillIndex,
		int &lineIndex);

	virtual ClipperLib::Paths toClipPath(
		const std::vector<PathRef> &paths) const;

	virtual bool hasFixedSize() const;
};

class RigidTesselator : public AbstractTesselator {
private:
	const RigidFlattener flattener;
	const ToveHoles holes;

public:
	RigidTesselator(
		int subdivisions,
		ToveHoles holes);

	virtual ToveMeshUpdateFlags pathToMesh(
		ToveMeshUpdateFlags update,
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
