/*************************************************************************/
/*  node_vg_editor_plugin.h          			                         */
/*************************************************************************/

#ifndef NOVE_VG_EDITOR_PLUGIN_H
#define NOVE_VG_EDITOR_PLUGIN_H

#include "node_vg_editor.h"

class NodeVGEditorPlugin : public EditorPlugin {
	GDCLASS(NodeVGEditorPlugin, EditorPlugin);

	NodeVGEditor *vg_editor;
	EditorNode *editor;
	String klass;

public:
	virtual bool forward_canvas_gui_input(const Ref<InputEvent> &p_event);
	virtual void forward_draw_over_viewport(Control *p_overlay);

	bool has_main_screen() const { return false; }
	virtual String get_name() const { return klass; }
	virtual void edit(Object *p_object);
	virtual bool handles(Object *p_object) const;
	virtual void make_visible(bool p_visible);

	NodeVGEditorPlugin(EditorNode *p_node);
	~NodeVGEditorPlugin();
};

#endif // NOVE_VG_EDITOR_PLUGIN_H
