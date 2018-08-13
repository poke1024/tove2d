/*************************************************************************/
/*  node_vg.cpp  	                                                     */
/*************************************************************************/

#include "node_vg.h"

void NodeVG::update_mesh() {

	if (graphics.is_valid()) {

		graphics->update_instance(this);
		update();
	}
}

void NodeVG::_bind_methods() {

	ClassDB::bind_method(D_METHOD("set_svg"), &NodeVG::set_svg);
	ClassDB::bind_method(D_METHOD("get_svg"), &NodeVG::get_svg);

    ClassDB::bind_method(D_METHOD("set_display", "display"), &NodeVG::set_display, DEFVAL(TOVE_DISPLAY_MESH));
	ClassDB::bind_method(D_METHOD("get_display"), &NodeVG::get_display);

	ClassDB::bind_method(D_METHOD("set_quality", "quality"), &NodeVG::set_quality, DEFVAL(1.0f));
	ClassDB::bind_method(D_METHOD("get_quality"), &NodeVG::get_quality);

	ADD_PROPERTY(PropertyInfo(Variant::OBJECT, "graphics", PROPERTY_HINT_RESOURCE_TYPE, "Texture"), "set_svg", "get_svg");
    ADD_PROPERTY(PropertyInfo(Variant::INT, "display", PROPERTY_HINT_ENUM, "Texture,Mesh"), "set_display", "get_display");
	ADD_PROPERTY(PropertyInfo(Variant::REAL, "quality", PROPERTY_HINT_RANGE, "0,1,0.01"), "set_quality", "get_quality");

	ClassDB::bind_method(D_METHOD("insert_curve", "path", "subpath", "t"), &NodeVG::insert_curve);
	ClassDB::bind_method(D_METHOD("remove_curve", "path", "subpath", "curve"), &NodeVG::remove_curve);
	ClassDB::bind_method(D_METHOD("set_points", "path", "subpath", "points"), &NodeVG::set_points);
}

Ref<Resource> NodeVG::get_svg() const {

	return svg;
}

void NodeVG::set_svg(const Ref<Resource> &p_svg) {

    svg = p_svg;

	graphics.instance();

	// this is a hack, as we always get a texture here due to
	// Godot's default SVG importer being an image importer.
	if (p_svg.is_valid()) {
		if (graphics->load(p_svg->get_path(), "px", 96) == OK) {
			update_mesh();
		}
	}
}

void NodeVG::set_display(ToveDisplay p_display) {
    if (graphics.is_valid()) {
		graphics->set_display(p_display);
		update_mesh();
	}
}

ToveDisplay NodeVG::get_display() {
    return graphics.is_valid() ? graphics->get_display() : TOVE_DISPLAY_MESH;
}

void NodeVG::set_quality(float quality) {
	if (graphics.is_valid()) {
		graphics->set_quality(quality);
		update_mesh();
	}
}

float NodeVG::get_quality() {
	return graphics.is_valid() ? graphics->get_quality() : 0.5f;
}

bool NodeVG::_edit_is_selected_on_click(const Point2 &p_point, double p_tolerance) const {
	const tove::GraphicsRef &graphics = get_tove_graphics();
	return graphics.get() ? graphics->hit(p_point.x, p_point.y).get() != NULL : false;
}

const Ref<VectorGraphics> &NodeVG::get_vector_graphics() const {
	return graphics;
}

const tove::GraphicsRef &NodeVG::get_tove_graphics() const {
	return graphics->get_tove_graphics();
}

void NodeVG::set_points(int p_path, int p_subpath, Array p_points) {
	ERR_FAIL_COND(!graphics.is_valid());
	graphics->set_points(p_path, p_subpath, p_points);
	update_mesh();
}

void NodeVG::insert_curve(int p_path, int p_subpath, float p_t) {
	ERR_FAIL_COND(!graphics.is_valid());
	graphics->insert_curve(p_path, p_subpath, p_t);
	update_mesh();
}

void NodeVG::remove_curve(int p_path, int p_subpath, int p_curve) {
	ERR_FAIL_COND(!graphics.is_valid());
	graphics->remove_curve(p_path, p_subpath, p_curve);
	update_mesh();
}

NodeVG::NodeVG() {
	graphics.instance();
	graphics->load_default();
	graphics->update_instance(this);
}
