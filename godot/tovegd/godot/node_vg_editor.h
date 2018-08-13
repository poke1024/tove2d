/*************************************************************************/
/*  node_vg_editor.h                 			                         */
/*************************************************************************/

#ifndef NOVE_VG_EDITOR_H
#define NOVE_VG_EDITOR_H

#include "editor/editor_node.h"
#include "editor/editor_plugin.h"
#include "scene/gui/tool_button.h"
#include "node_vg.h"

class CanvasItemEditor;

class NodeVGEditor : public HBoxContainer {

	GDCLASS(NodeVGEditor, HBoxContainer);

    NodeVG *node_vg;
	int active_path;
	Ref<ArrayMesh> overlay;
	Transform2D overlay_xform;

	ToolButton *button_create;
	ToolButton *button_edit;
	ToolButton *button_delete;

	struct Vertex {
		Vertex();
		Vertex(int p_pt);
		Vertex(int p_path, int p_subpath, int p_pt);

		bool operator==(const Vertex &p_vertex) const;
		bool operator!=(const Vertex &p_vertex) const;

		bool valid() const;

		int path;
		int subpath;
		int pt;
	};

	struct PosVertex : public Vertex {
		PosVertex();
		PosVertex(const Vertex &p_vertex, const Vector2 &p_pos);
		PosVertex(int p_path, int p_subpath, int p_pt, const Vector2 &p_pos);

		Vector2 pos;
	};

	struct SubpathPos {
		SubpathPos();
		SubpathPos(int p_path, int p_subpath, float p_t);

		bool valid() const;

		int path;
		int subpath;
		float t;
	};

	class PointIterator {
	private:
		int path;
		tove::PathRef path_ref;
		int n_subpaths;
		int subpath;
		tove::SubpathRef subpath_ref;
		int pt;
		int n_pts;
		bool closed;

	public:
		PointIterator(
			int p_path,
			const tove::PathRef &p_path_ref);

		bool next();

		PosVertex get_pos_vertex() const;
		Vertex get_vertex() const;
		Vector2 get_pos() const;
	};

	class ControlPointIterator {
	private:
		const Vertex &selected_point;
		const Vertex &edited_point;
		int edited_pt0;

		int path;
		tove::PathRef path_ref;
		int n_subpaths;
		int subpath;
		tove::SubpathRef subpath_ref;
		int pt;
		int d_pt;
		int n_pts;
		bool closed;

		bool is_visible() const;

	public:
		ControlPointIterator(
			int p_path,
			const tove::PathRef &p_path_ref,
			const Vertex &p_selected_point,
			const Vertex &p_edited_point);

		bool next();

		PosVertex get_pos_vertex() const;
		Vector2 get_pos() const;
		Vector2 get_knot_pos() const;
	};

	PosVertex edited_point;
	Vertex hover_point; // point under mouse cursor
	Vertex selected_point; // currently selected

	Array pre_move_edit;

	CanvasItemEditor *canvas_item_editor;
	EditorNode *editor;
	Panel *panel;
	ConfirmationDialog *create_resource;

protected:
	enum {

		MODE_CREATE,
		MODE_EDIT,
		MODE_DELETE,
		MODE_CONT,

	};

	int mode;

	UndoRedo *undo_redo;

	virtual void _menu_option(int p_option);
	void _wip_close();
	bool _delete_point(const Vector2 &p_gpoint);

	void _notification(int p_what);
	void _node_removed(Node *p_node);
	static void _bind_methods();

	void remove_point(const Vertex &p_vertex);
	Vertex get_active_point() const;
	PosVertex closest_point(const Vector2 &p_pos, bool p_cp = false) const;
	SubpathPos closest_subpath_point(const Vector2 &p_pos) const;
	int find_path_at_point(const Vector2 &p_pos) const;

	bool _is_empty() const;
	void _commit_action();

	Array _get_points(const Vertex &p_vertex);
	Array _get_points(const SubpathPos &p_pos);
	void _update_overlay(bool p_always_update = false);

protected:
	virtual Node2D *_get_node() const;
	virtual void _set_node(Node *p_node_vg);

	virtual bool _has_resource() const;
	virtual void _create_resource();

public:
	bool forward_gui_input(const Ref<InputEvent> &p_event);
	void forward_draw_over_viewport(Control *p_overlay);

	void edit(Node *p_node_vg);
	NodeVGEditor(EditorNode *p_editor);
};

#endif // NOVE_VG_EDITOR_H
