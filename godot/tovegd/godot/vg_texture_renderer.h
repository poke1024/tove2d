/*************************************************************************/
/*  vg_texture_renderer.h                                                */
/*************************************************************************/

#ifndef VG_TEXTURE_RENDERER_H
#define VG_TEXTURE_RENDERER_H

#include "vg_renderer.h"
#include "tove2d.h"

class VGTextureRenderer : public VGRenderer {
	GDCLASS(VGTextureRenderer, VGRenderer);

    float quality;

protected:
	static void _bind_methods();

public:
    VGTextureRenderer();

    float get_quality();
    void set_quality(float p_quality);

    virtual Rect2 render_mesh(Ref<ArrayMesh> &p_mesh, VGPath *p_path);
    virtual Ref<ImageTexture> render_texture(VGPath *p_path);
};

#endif VG_TEXTURE_RENDERER_H
