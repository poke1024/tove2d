/*************************************************************************/
/*  vg_mesh_renderer.h                                                   */
/*************************************************************************/

#ifndef VG_MESH_RENDERER_H
#define VG_MESH_RENDERER_H

#include "vg_renderer.h"
#include "tove2d.h"

class VGMeshRenderer : public VGRenderer {
protected:
    tove::TesselatorRef tesselator;

	static void _bind_methods();

public:
    VGMeshRenderer();

    const tove::TesselatorRef &get_tesselator() const {
        return tesselator;
    }

    virtual Rect2 render_mesh(Ref<ArrayMesh> &p_mesh, VGPath *p_path);
    virtual Ref<ImageTexture> render_texture(VGPath *p_path);
};

#endif VG_MESH_RENDERER_H
