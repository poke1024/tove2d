/*
 * TÖVE - Animated vector graphics for LÖVE.
 * https://github.com/poke1024/Tove
 *
 * Copyright (c) 2018, Bernhard Liebl
 *
 * Distributed under the MIT license. See LICENSE file for details.
 *
 * All rights reserved.
 */

#include "tove2d.h"
#include "common.h"

#include "../thirdparty/polypartition.h"

// an important note about naming of things:

// what NSVG calls a "shape" is called a "path" in SVG; what NSVG calls a "path" is really
// a "sub path" or a "trajectory". see this explanation by Mark Kilgard:
// (https://www.opengl.org/discussion_boards/showthread.php/175260-GPU-accelerated-path-rendering?p=1225200&viewfull=1#post1225200)

// which leads us to this renaming:
// 		NSVG path -> Trajectory
// 		NSVG shape -> Path
// 		NSVG image -> Graphics

// at the same time, we keep the names of beginPath() and closePath() functions in Graphics,
// even though they should really be called begin beginTrajectory() and endTrajectory().
// Yet HTML5, Flash and Apple's CoreGraphics all use beginPath() and closePath(), so hey,
// who are we to change convention.

/*#include "utils.h"
#include "nsvg.h"

#include "claimable.h"
#include "primitive.h"
#include "trajectory.h"
#include "paint.h"

#include "path.h"

#include "graphics.h"

typedef ClipperLib::Path ClipperPath;
typedef ClipperLib::Paths ClipperPaths;
typedef ClipperLib::IntPoint ClipperPoint;


#include "mesh/flatten.h"
#include "mesh/paint.h"
#include "mesh/utils.h"
#include "mesh/partition.h"
#include "mesh/triangles.h"

#include "mesh/mesh.h"
#include "mesh/meshifier.h"
*/
