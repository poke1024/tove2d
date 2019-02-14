/*************************************************************************/
/*  vg_renderer.h                                                        */
/*************************************************************************/

#ifndef VG_RENDERER_H
#define VG_RENDERER_H

#include "core/resource.h"
#include "scene/2d/mesh_instance_2d.h"

class VGPath;

class VGRenderer : public Resource {
	GDCLASS(VGRenderer, Resource);

protected:
    static void clear_mesh(Ref<ArrayMesh> &p_mesh);

public:
    virtual Rect2 render_mesh(Ref<ArrayMesh> &p_mesh, VGPath *p_path) {
        return Rect2();
    }
    virtual Ref<ImageTexture> render_texture(VGPath *p_path) {
        return Ref<ImageTexture>();
    }
};

#endif // VG_RENDERER_H
