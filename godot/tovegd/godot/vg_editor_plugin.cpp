/*************************************************************************/
/*  vg_editor_plugin.cpp          				                         */
/*************************************************************************/

#include "vg_editor_plugin.h"
#include "editor/plugins/canvas_item_editor_plugin.h"

bool VGEditorPlugin::forward_canvas_gui_input(const Ref<InputEvent> &p_event) {

    return vg_editor->forward_gui_input(p_event);
}

void VGEditorPlugin::forward_draw_over_viewport(Control *p_overlay) {

    vg_editor->forward_draw_over_viewport(p_overlay);
}

void VGEditorPlugin::edit(Object *p_object) {

	vg_editor->edit(Object::cast_to<Node>(p_object));
}

bool VGEditorPlugin::handles(Object *p_object) const {

	return p_object->is_class(klass);
}

void VGEditorPlugin::make_visible(bool p_visible) {

	if (p_visible) {

		vg_editor->show();
	} else {

		vg_editor->hide();
		vg_editor->edit(NULL);
	}
}

VGEditorPlugin::VGEditorPlugin(EditorNode *p_node) {

	editor = p_node;
	vg_editor = memnew(VGEditor(p_node));
	//vg_toolbar = memnew(VGEditor(p_node));
	klass = "VGPath";
	//CanvasItemEditor::get_singleton()->get_bottom_split()->add_child(vg_editor);
	CanvasItemEditor::get_singleton()->add_control_to_menu_panel(vg_editor);

    vg_editor->hide();
}

VGEditorPlugin::~VGEditorPlugin() {
}
