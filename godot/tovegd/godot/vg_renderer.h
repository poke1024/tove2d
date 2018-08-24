/*************************************************************************/
/*  vg_renderer.h                                                        */
/*************************************************************************/

#ifndef VG_RENDERER_H
#define VG_RENDERER_H

#include "resource.h"
#include "tove2d.h"

class VGRenderer : public Resource {
	GDCLASS(VGRenderer, Resource);

public:
    virtual VGMeshData graphics_to_mesh(const tove::GraphicsRef &p_tove_graphics) const {
        return VGMeshData();
    }
};

#endif VG_RENDERER_H
