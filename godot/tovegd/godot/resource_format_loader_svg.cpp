/*************************************************************************/
/*  resource_format_loader_svg.cpp                                       */
/*************************************************************************/

#include "resource_format_loader_svg.h"
#include "vector_graphics.h"

RES ResourceFormatLoaderSVG::load(const String &p_path, const String &p_original_path, Error *r_error) {

	if (r_error)
		*r_error = ERR_FILE_CANT_OPEN;

	Ref<VectorGraphics> graphics;
	graphics.instance();
	Error err = graphics->load(p_path, "px", 96);
	if (r_error)
		*r_error = err;

	return graphics;
}

void ResourceFormatLoaderSVG::get_recognized_extensions(List<String> *p_extensions) const {

	p_extensions->push_back("svg");
}

bool ResourceFormatLoaderSVG::handles_type(const String &p_type) const {

	return (p_type == "SVG");
}

String ResourceFormatLoaderSVG::get_resource_type(const String &p_path) const {

	String el = p_path.get_extension().to_lower();
	if (el == "svg")
		return "VectorGraphics";
	return "";
}
