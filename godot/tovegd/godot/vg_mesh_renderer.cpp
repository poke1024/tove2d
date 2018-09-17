/*************************************************************************/
/*  vg_mesh_renderer.cpp                                                 */
/*************************************************************************/

#include "vg_mesh_renderer.h"
#include "vg_path.h"
#include "../src/cpp/mesh/meshifier.h"

class Renderer {
	int fill_index;
	int line_index;

    tove::MeshRef tove_mesh;
    tove::GraphicsRef root_graphics;

public:
    Renderer(const tove::MeshRef &p_tove_mesh, const tove::GraphicsRef &p_root_graphics) {
        fill_index = 0;
        line_index = 0;
        tove_mesh = p_tove_mesh;
        root_graphics = p_root_graphics;
    }

    void traverse(Node *p_node, const Transform2D &p_transform) {
        const int n = p_node->get_child_count();
        for (int i = 0; i < n; i++) {
            Node *child = p_node->get_child(i);

            Transform2D t;
            if (child->is_class_ptr(CanvasItem::get_class_ptr_static())) {
                t = p_transform * Object::cast_to<CanvasItem>(child)->get_transform();
            } else {
                t = p_transform;
            }
            
            traverse(child, t);
        }

        if (p_node->is_class_ptr(VGPath::get_class_ptr_static())) {
            VGPath *path = Object::cast_to<VGPath>(p_node);
            Ref<VGRenderer> renderer = path->get_inherited_renderer();
            if (renderer.is_valid() && renderer->is_class_ptr(VGMeshRenderer::get_class_ptr_static())) {
                tove::TesselatorRef tesselator =
                    Object::cast_to<VGMeshRenderer>(renderer.ptr())->get_tesselator();
                if (tesselator) {
                    Size2 s = path->get_global_transform().get_scale();
                    tesselator->beginTesselate(root_graphics.get(), MAX(s.width, s.height));

                    tesselator->pathToMesh(
                        UPDATE_MESH_EVERYTHING,
                        new_transformed_path(path->get_tove_path(), p_transform),
                        tove_mesh, tove_mesh,
                        fill_index, line_index);

                    tesselator->endTesselate();
                }
            }
        }
    }
};

VGMeshRenderer::VGMeshRenderer() {
}

Rect2 VGMeshRenderer::render_mesh(Ref<ArrayMesh> &p_mesh, VGPath *p_path) {
	clear_mesh(p_mesh);

    VGPath *root = p_path->get_root_path();
    tove::GraphicsRef subtree_graphics = root->get_subtree_graphics();
		
    tove::MeshRef tove_mesh = tove::tove_make_shared<tove::ColorMesh>();
    Renderer r(tove_mesh, subtree_graphics);
    r.traverse(p_path, Transform2D());

    copy_mesh(p_mesh, tove_mesh);

    return tove_bounds_to_rect2(p_path->get_tove_path()->getBounds());
}

Ref<ImageTexture> VGMeshRenderer::render_texture(VGPath *p_path) {
    return Ref<ImageTexture>();
}
