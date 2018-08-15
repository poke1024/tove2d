/*************************************************************************/
/*  register_types.cpp                                                   */
/*************************************************************************/

#include "register_types.h"
#include "godot/resource_format_loader_svg.h"
#include "godot/vector_graphics.h"
#include "godot/node_vg.h"

#ifdef TOOLS_ENABLED
#include "godot/node_vg_editor_plugin.h"
#endif

#if GDTOVE_SVG_RFL
static ResourceFormatLoaderSVG *svg_loader = NULL;
#endif

#ifdef TOOLS_ENABLED
static void editor_init_callback() {
	EditorNode *editor = EditorNode::get_singleton();
	editor->add_editor_plugin(memnew(NodeVGEditorPlugin(editor)));
}
#endif

void register_tovegd_types() {
#if GDTOVE_SVG_RFL
	svg_loader = memnew(ResourceFormatLoaderSVG);
	ResourceLoader::add_resource_format_loader(svg_loader);
#endif

	ClassDB::register_class<VectorGraphics>();
	ClassDB::register_class<NodeVG>();

#ifdef TOOLS_ENABLED
	EditorNode::add_init_callback(editor_init_callback);
#endif
}

void unregister_tovegd_types() {
}
