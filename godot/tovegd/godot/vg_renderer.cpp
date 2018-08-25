/*************************************************************************/
/*  vg_renderer.h                                                        */
/*************************************************************************/

#include "vg_renderer.h"

void VGRenderer::clear_mesh(Ref<ArrayMesh> &p_mesh) {
	if (p_mesh.is_valid()) {
		while (p_mesh->get_surface_count() > 0) {
			p_mesh->surface_remove(0);
		}
	} else {
		p_mesh.instance();
	}
}
