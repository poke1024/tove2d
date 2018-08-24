/*************************************************************************/
/*  vg_adaptive_renderer.h                                               */
/*************************************************************************/

#ifndef VG_ADAPTIVE_RENDERER_H
#define VG_ADAPTIVE_RENDERER_H

#include "vg_renderer.h"

class VGAdaptiveRenderer : public VGRenderer {
	GDCLASS(VGAdaptiveRenderer, VGRenderer);

    float quality;

public:
    VGAdaptiveRenderer();

    virtual VGMeshData graphics_to_mesh(const tove::GraphicsRef &p_tove_graphics) const;
};

#endif VG_ADAPTIVE_RENDERER_H
