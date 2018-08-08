/*************************************************************************/
/*  register_types.cpp                                                   */
/*************************************************************************/

#include "register_types.h"
#include "godot/resource_format_loader_svg.h"
#include "godot/tove_graphics.h"
#include "godot/tove_instance.h"

static ResourceFormatLoaderSVG *svg_loader = NULL;

void register_tovegd_types() {

	svg_loader = memnew(ResourceFormatLoaderSVG);
	ResourceLoader::add_resource_format_loader(svg_loader);

	ClassDB::register_class<ToveGraphics>();
	ClassDB::register_class<ToveInstance>();
}

void unregister_tovegd_types() {
}
