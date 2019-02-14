/*************************************************************************/
/*  vg_editor.h  			          			                         */
/*************************************************************************/

#ifndef VG_EDITOR_H
#define VG_EDITOR_H

#include "editor/editor_node.h"
#include "editor/editor_plugin.h"
#include "scene/gui/tool_button.h"
#include "vg_path.h"

class CanvasItemEditor;

class VGTool : public Reference {
protected:
	CanvasItemEditor * const canvas_item_editor;

	Vector2 global_to_local(CanvasItem *p_node, const Vector2 p_where) const;

public:
	VGTool();

	virtual bool forward_gui_input(const Ref<InputEvent> &p_event) = 0;
	virtual void forward_canvas_draw_over_viewport(Control *p_overlay) { }
};

class VGEditor : public HBoxContainer {

	GDCLASS(VGEditor, HBoxContainer);

    VGPath *node_vg;
	Ref<VGTool> tool;

	Ref<ArrayMesh> overlay;
	Transform2D overlay_full_xform;
	Transform2D overlay_draw_xform;

	Vector<ToolButton*> tool_buttons;

	enum {
		TOOL_TRANSFORM = 0,
		TOOL_CURVE,
		TOOL_ELLIPSE
	};

	ToolButton *button_bake;

	CanvasItemEditor *canvas_item_editor;
	EditorNode *editor;
	Panel *panel;
	ConfirmationDialog *create_resource;

protected:
	UndoRedo *undo_redo;

	virtual void _tool_selected(int p_tool);
	void _create_mesh_node();

	void _notification(int p_what);
	void _node_removed(Node *p_node);
	static void _bind_methods();

	bool is_inside(const Vector2 &p_pos) const;

	bool _is_empty() const;

	void _update_overlay(bool p_always_update = false);
	virtual void _changed_callback(Object *p_changed, const char *p_prop);
	void _line_width_changed(double p_value);

protected:
	virtual Node2D *_get_node() const;
	virtual void _set_node(Node *p_node_vg);

	virtual bool _has_resource() const;
	virtual void _create_resource();

public:
	bool forward_gui_input(const Ref<InputEvent> &p_event);
	void forward_canvas_draw_over_viewport(Control *p_overlay);

	void edit(Node *p_node_vg);
	EditorNode *get_editor_node() const {
		return editor;
	}

	VGEditor(EditorNode *p_editor);
};

#endif // VG_EDITOR_H
