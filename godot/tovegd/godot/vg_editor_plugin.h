/*************************************************************************/
/*  vg_editor_plugin.h    		      			                         */
/*************************************************************************/

#ifndef VG_EDITOR_PLUGIN_H
#define VG_EDITOR_PLUGIN_H

#include "vg_editor.h"

class VGEditorPlugin : public EditorPlugin {
	GDCLASS(VGEditorPlugin, EditorPlugin);

	VGEditor *vg_editor;
	EditorNode *editor;
	String klass;

public:
	virtual bool forward_canvas_gui_input(const Ref<InputEvent> &p_event);
	virtual void forward_canvas_draw_over_viewport(Control *p_overlay);

	bool has_main_screen() const { return false; }
	virtual String get_name() const { return klass; }
	virtual void edit(Object *p_object);
	virtual bool handles(Object *p_object) const;
	virtual void make_visible(bool p_visible);

	VGEditorPlugin(EditorNode *p_node);
	~VGEditorPlugin();
};

#endif // VG_EDITOR_PLUGIN_H
