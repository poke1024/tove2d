/*************************************************************************/
/*  node_vg.cpp  	                                                     */
/*************************************************************************/

#include "node_vg.h"

void NodeVG::_notification(int p_what) {

	if (p_what == NOTIFICATION_DRAW) {
		if (vg.is_valid()) {
			draw_mesh(vg->get_mesh(), vg->get_texture(), Ref<Texture>());
		}
	}
}

void NodeVG::_bind_methods() {

	ClassDB::bind_method(D_METHOD("set_graphics"), &NodeVG::set_graphics);
	ClassDB::bind_method(D_METHOD("get_graphics"), &NodeVG::get_graphics);

    ClassDB::bind_method(D_METHOD("set_backend", "backend"), &NodeVG::set_backend, DEFVAL(TOVE_BACKEND_MESH));
	ClassDB::bind_method(D_METHOD("get_backend"), &NodeVG::get_backend);

	ClassDB::bind_method(D_METHOD("set_quality", "quality"), &NodeVG::set_quality, DEFVAL(1.0f));
	ClassDB::bind_method(D_METHOD("get_quality"), &NodeVG::get_quality);

	ADD_PROPERTY(PropertyInfo(Variant::OBJECT, "graphics", PROPERTY_HINT_RESOURCE_TYPE, "Texture"), "set_graphics", "get_graphics");
    ADD_PROPERTY(PropertyInfo(Variant::INT, "backend", PROPERTY_HINT_ENUM, "Texture,Mesh", PROPERTY_USAGE_EDITOR | PROPERTY_USAGE_EDITOR_HELPER), "set_backend", "get_backend");
	ADD_PROPERTY(PropertyInfo(Variant::REAL, "quality", PROPERTY_HINT_RANGE, "0,1,0.01", PROPERTY_USAGE_EDITOR | PROPERTY_USAGE_EDITOR_HELPER), "set_quality", "get_quality");

	ClassDB::bind_method(D_METHOD("insert_curve", "path", "subpath", "t"), &NodeVG::insert_curve);
	ClassDB::bind_method(D_METHOD("remove_curve", "path", "subpath", "curve"), &NodeVG::remove_curve);
	ClassDB::bind_method(D_METHOD("set_points", "path", "subpath", "points"), &NodeVG::set_points);
}

Ref<Resource> NodeVG::get_graphics() const {

	return vg;
}

void NodeVG::set_graphics(const Ref<Resource> &p_vg) {

	if (p_vg == vg) {
		return;
	}

	if (vg.is_valid()) {
		vg->remove_change_receptor(this);
	}

	if (!p_vg.is_valid()) {
		vg = Ref<VectorGraphics>();
	} else {
		Ref<VectorGraphics> new_vg = p_vg;
		if (new_vg.is_valid()) {
			vg = new_vg;
		} else if (p_vg->get_path().ends_with(".svg")) {
			vg.instance();

			// this is a hack, as we always get a texture here due to
			// Godot's default SVG importer being an image importer.			
			if (vg->load(p_vg->get_path(), "px", 96) == OK) {
				// OK
			}
		}
	}

	if (vg.is_valid()) {
		vg->add_change_receptor(this);		
	}

	update();
}

void NodeVG::set_backend(ToveBackend p_backend) {
    if (vg.is_valid()) {
		vg->set_backend(p_backend);
	}
}

ToveBackend NodeVG::get_backend() {
    return vg.is_valid() ? vg->get_backend() : TOVE_BACKEND_MESH;
}

void NodeVG::set_quality(float quality) {
	if (vg.is_valid()) {
		vg->set_quality(quality);
	}
}

float NodeVG::get_quality() {
	return vg.is_valid() ? vg->get_quality() : 0.5f;
}

Rect2 NodeVG::_edit_get_rect() const {
	const tove::GraphicsRef tove_graphics = get_tove_graphics();
	if (tove_graphics.get()) {
		const float *bounds = tove_graphics->getBounds();
		return Rect2(bounds[0], bounds[1], bounds[2], bounds[3]);
	} else {
		return Node2D::_edit_get_rect();
	}
}

bool NodeVG::_edit_is_selected_on_click(const Point2 &p_point, double p_tolerance) const {
	const tove::GraphicsRef tove_graphics = get_tove_graphics();
	if (tove_graphics.get()) {
		return tove_graphics->hit(p_point.x, p_point.y).get();
	} else {
		return false;
	}
}

void NodeVG::_changed_callback(Object *p_changed, const char *p_prop) {

	// Changes to the VectorGraphics need to trigger an update to make
	// the editor redraw the NodeVG with the updated VectorGraphics.
	if (vg.is_valid() && vg.ptr() == p_changed) {
		update();

#ifdef TOOLS_ENABLED
 		_change_notify(p_prop); // propagate to editor
#endif
	}
}

Ref<VectorGraphics> NodeVG::get_vector_graphics() const {
	return vg;
}

tove::GraphicsRef NodeVG::get_tove_graphics() const {
	return vg.is_valid() ? vg->get_tove_graphics() : tove::GraphicsRef();
}

void NodeVG::set_points(int p_path, int p_subpath, Array p_points) {
	ERR_FAIL_COND(!vg.is_valid());
	vg->set_points(p_path, p_subpath, p_points);
}

void NodeVG::insert_curve(int p_path, int p_subpath, float p_t) {
	ERR_FAIL_COND(!vg.is_valid());
	vg->insert_curve(p_path, p_subpath, p_t);
}

void NodeVG::remove_curve(int p_path, int p_subpath, int p_curve) {
	ERR_FAIL_COND(!vg.is_valid());
	vg->remove_curve(p_path, p_subpath, p_curve);
}

NodeVG::NodeVG() {
	vg.instance();
	vg->load_default();
	vg->add_change_receptor(this);		
}
