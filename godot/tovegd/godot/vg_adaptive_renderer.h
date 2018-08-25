/*************************************************************************/
/*  vg_adaptive_renderer.h                                               */
/*************************************************************************/

#ifndef VG_ADAPTIVE_RENDERER_H
#define VG_ADAPTIVE_RENDERER_H

#include "vg_mesh_renderer.h"

class VGAdaptiveRenderer : public VGMeshRenderer {
	GDCLASS(VGAdaptiveRenderer, VGRenderer);

    float quality;

protected:
    void create_tesselator();

	static void _bind_methods();

public:
    VGAdaptiveRenderer();

    float get_quality();
    void set_quality(float p_quality);
};

#endif VG_ADAPTIVE_RENDERER_H
