/*************************************************************************/
/*  vg_color.cp          	                                             */
/*************************************************************************/

#include "vg_color.h"

void VGColor::_bind_methods() {
	ClassDB::bind_method(D_METHOD("set_color", "color"), &VGColor::set_color);
	ClassDB::bind_method(D_METHOD("get_color"), &VGColor::get_color);

	ADD_PROPERTY(PropertyInfo(Variant::COLOR, "color"), "set_color", "get_color");
}

void VGColor::set_color(const Color &p_color) {
    color = p_color;
	_change_notify("color");
}

Color VGColor::get_color() const {
    return color;
}
