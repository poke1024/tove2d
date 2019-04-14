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

#include "common.h"
#include "references.h"
#include <sstream>

BEGIN_TOVE_NAMESPACE

void encounteredNilRef(const char *typeName) {
    if (tove::report::config.level <= TOVE_REPORT_WARN) {
        std::ostringstream s;
        s << "encountered nil " << typeName;
        tove::report::warn(s.str().c_str());
    }
}

References<Graphics, ToveGraphicsRef> shapes;
References<Path, TovePathRef> paths;
References<Subpath, ToveSubpathRef> trajectories;
References<AbstractPaint, TovePaintRef> paints;
References<AbstractFeed, ToveFeedRef> shaderLinks;
References<AbstractMesh, ToveMeshRef> meshes;
References<AbstractTesselator, ToveTesselatorRef> tesselators;
References<Palette, TovePaletteRef> palettes;
References<std::string, ToveNameRef> names;

END_TOVE_NAMESPACE
