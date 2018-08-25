/*************************************************************************/
/*  register_types.cpp                                                   */
/*************************************************************************/

#include "register_types.h"
#include "godot/resource_format_loader_svg.h"
#include "godot/vg_path.h"
#include "godot/vg_paint.h"
#include "godot/vg_color.h"
#include "godot/vg_renderer.h"
#include "godot/vg_texture_renderer.h"
#include "godot/vg_adaptive_renderer.h"

#ifdef TOOLS_ENABLED
#include "godot/vg_editor_plugin.h"
#endif

#if GDTOVE_SVG_RFL
static ResourceFormatLoaderSVG *svg_loader = NULL;
#endif

#ifdef TOOLS_ENABLED
static void editor_init_callback() {
	EditorNode *editor = EditorNode::get_singleton();
	editor->add_editor_plugin(memnew(VGEditorPlugin(editor)));
}
#endif

void register_tovegd_types() {
#if GDTOVE_SVG_RFL
	svg_loader = memnew(ResourceFormatLoaderSVG);
	ResourceLoader::add_resource_format_loader(svg_loader);
#endif

	ClassDB::register_class<VGPath>();
	ClassDB::register_class<VGPaint>();
	ClassDB::register_class<VGColor>();

	ClassDB::register_class<VGRenderer>();
	ClassDB::register_class<VGTextureRenderer>();
	ClassDB::register_class<VGAdaptiveRenderer>();

#ifdef TOOLS_ENABLED
	EditorNode::add_init_callback(editor_init_callback);
#endif
}

void unregister_tovegd_types() {
}
