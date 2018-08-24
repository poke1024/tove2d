#include "vg_adaptive_renderer.h"

VGAdaptiveRenderer::VGAdaptiveRenderer() : quality(100) {
}

VGMeshData VGAdaptiveRenderer::graphics_to_mesh(const tove::GraphicsRef &p_tove_graphics) const {
	return ToveGraphicsBackend(p_tove_graphics, quality).create_mesh_data(TOVE_BACKEND_MESH);
}

/*

    ADD_PROPERTY(PropertyInfo(Variant::INT, "backend", PROPERTY_HINT_ENUM, "Inherit,Texture,Mesh"), "set_backend", "get_backend");
	ADD_PROPERTY(PropertyInfo(Variant::REAL, "quality", PROPERTY_HINT_RANGE, "0,2000,10"), "set_quality", "get_quality");
*/
