/*************************************************************************/
/*  node_vg_editor_plugin.cpp          			                         */
/*************************************************************************/

#include "node_vg_editor_plugin.h"
#include "editor/plugins/canvas_item_editor_plugin.h"

bool NodeVGEditorPlugin::forward_canvas_gui_input(const Ref<InputEvent> &p_event) {

    return vg_editor->forward_gui_input(p_event);
}

void NodeVGEditorPlugin::forward_draw_over_viewport(Control *p_overlay) {

    vg_editor->forward_draw_over_viewport(p_overlay);
}

void NodeVGEditorPlugin::edit(Object *p_object) {

	vg_editor->edit(Object::cast_to<Node>(p_object));
}

bool NodeVGEditorPlugin::handles(Object *p_object) const {

	return p_object->is_class(klass);
}

void NodeVGEditorPlugin::make_visible(bool p_visible) {

	if (p_visible) {

		vg_editor->show();
	} else {

		vg_editor->hide();
		vg_editor->edit(NULL);
	}
}

NodeVGEditorPlugin::NodeVGEditorPlugin(EditorNode *p_node) {

	editor = p_node;
	vg_editor =  memnew(NodeVGEditor(p_node));
	klass = "NodeVG";
	CanvasItemEditor::get_singleton()->add_control_to_menu_panel(vg_editor);

    vg_editor->hide();
}

NodeVGEditorPlugin::~NodeVGEditorPlugin() {
}
