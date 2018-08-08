/*************************************************************************/
/*  tove_instance.cpp                                                    */
/*************************************************************************/

#include "tove_instance.h"

void ToveInstance::update_mesh() {

	if (graphics.is_valid()) {

		graphics->update_mesh(this);
		update();
	}
}

void ToveInstance::_bind_methods() {

	ClassDB::bind_method(D_METHOD("set_svg"), &ToveInstance::set_svg);
	ClassDB::bind_method(D_METHOD("get_svg"), &ToveInstance::get_svg);

    ClassDB::bind_method(D_METHOD("set_display", "display"), &ToveInstance::set_display, DEFVAL(TOVE_DISPLAY_MESH));
	ClassDB::bind_method(D_METHOD("get_display"), &ToveInstance::get_display);

	ClassDB::bind_method(D_METHOD("set_quality", "quality"), &ToveInstance::set_quality, DEFVAL(1.0f));
	ClassDB::bind_method(D_METHOD("get_quality"), &ToveInstance::get_quality);

	ADD_PROPERTY(PropertyInfo(Variant::OBJECT, "graphics", PROPERTY_HINT_RESOURCE_TYPE, "Texture"), "set_svg", "get_svg");
    ADD_PROPERTY(PropertyInfo(Variant::INT, "display", PROPERTY_HINT_ENUM, "Texture,Mesh"), "set_display", "get_display");
	ADD_PROPERTY(PropertyInfo(Variant::REAL, "quality", PROPERTY_HINT_RANGE, "0,1,0.01"), "set_quality", "get_quality");
}

Ref<Resource> ToveInstance::get_svg() const {

	return svg;
}

void ToveInstance::set_svg(const Ref<Resource> &p_svg) {

    svg = p_svg;

	// this is a hack, as we always get a texture here due to
	// Godot's default SVG importer being an image importer.
	if (p_svg.is_valid()) {
		graphics.instance();
		if (graphics->load(p_svg->get_path(), "px", 96) != OK) {
			graphics = Ref<ToveGraphics>();
		} else {
			update_mesh();
		}
	} else {
		graphics = Ref<ToveGraphics>();
	}
}

void ToveInstance::set_display(ToveDisplay p_display) {
    if (graphics.is_valid()) {
		graphics->set_display(p_display);
		update_mesh();
	}
}

ToveDisplay ToveInstance::get_display() {
    return graphics.is_valid() ? graphics->get_display() : TOVE_DISPLAY_MESH;
}

void ToveInstance::set_quality(float quality) {
	if (graphics.is_valid()) {
		graphics->set_quality(quality);
		update_mesh();
	}
}

float ToveInstance::get_quality() {
	return graphics.is_valid() ? graphics->get_quality() : 0.5f;
}

ToveInstance::ToveInstance() {
}
