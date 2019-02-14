/*************************************************************************/
/*  vg_editor.cpp         						                         */
/*************************************************************************/

#include "vg_editor.h"

#include "editor/plugins/canvas_item_editor_plugin.h"
#include "core/os/keyboard.h"
#include "../src/cpp/mesh/meshifier.h"

static Array subpath_points_array(const tove::SubpathRef &subpath) {
	const int n = subpath->getNumPoints();
	const float *p = subpath->getPoints();
	Array a;
	a.resize(n);
	for (int i = 0; i < n; i++) {
		a[i] = Vector2(p[2 * i + 0], p[2 * i + 1]);
	}
	return a;
}

inline bool is_control_point(int pt) {
	return (pt % 3) != 0;
}

Vector2 VGTool::global_to_local(CanvasItem *p_node, const Vector2 p_where) const {
	return p_node->get_global_transform().affine_inverse().xform(
		canvas_item_editor->snap_point(canvas_item_editor->get_canvas_transform().affine_inverse().xform(p_where)));
}

class GaussJordan {
private:
	Vector3 A[3];
	Vector3 b;

	void swap_rows(int i, int j) {
		if (i != j) {
			{
				Vector3 t = A[i];
				A[i] = A[j];
				A[j] = t;
			}
			{
				float t = b[i];
				b[i] = b[j];
				b[j] = t;
			}
		}
	}

	int find_pivot(int h) {
		int i_max = h;
		float m = 0.0f;
		for (int i = h; i < 3; i++) {
			float t = Math::abs(A[i][h]);
			if (t > m) {
				m = t;
				i_max = i;
			}
		}
		return i_max;
	}

	Vector3 find_x() {
		Vector3 x;
		for (int i = 2; i >= 0; i--) {
			float t = b[i];
			for (int j = 2; j > i; j--) {
				t -= x[j] * A[i][j];
			}
			x[i] = t / A[i][i];
		}
		return x;
	}

public:
	GaussJordan(Vector3 A1, Vector3 A2, Vector3 A3, Vector3 b) : b(b) {
		A[0] = A1;
		A[1] = A2;
		A[2] = A3;
	}

	Vector3 solve() {
		for (int d = 0; d < 3; d++) {
			swap_rows(d, find_pivot(d));

			for (int i = d + 1; i < 3; i++) {
				const float t = A[i][d] / A[d][d];
				A[i][d] = 0;

				for (int j = d + 1; j < 3; j++) {
					A[i][j] = A[i][j] - A[d][j] * t;
				}
				b[i] -= t * b[d];
			}
		}

		return find_x();
	}
};

Transform2D mapping_to_transform(const Vector2 &u1, const Vector2 &v1, const Vector2 &u2, const Vector2 &v2, const Vector2 &u3, const Vector2 &v3) {

	GaussJordan gx(
		Vector3(u1.x, u1.y, 1),
		Vector3(u2.x, u2.y, 1),
		Vector3(u3.x, u3.y, 1),
		Vector3(v1.x, v2.x, v3.x)
	);
	Vector3 x = gx.solve();

	GaussJordan gy(
		Vector3(u1.x, u1.y, 1),
		Vector3(u2.x, u2.y, 1),
		Vector3(u3.x, u3.y, 1),
		Vector3(v1.y, v2.y, v3.y)
	);
	Vector3 y = gy.solve();

	return Transform2D(x[0], y[0], x[1], y[1], x[2], y[2]);
}

Vector2 ray_ray(const Vector2 &s1, const Vector2 &r1, const Vector2 &s2, const Vector2 &r2) {
	float t = -((-(r2.y*s1.x) + r2.y*s2.x + r2.x*s1.y - r2.x*s2.y)/(r2.x*r1.y - r1.x*r2.y));
	return s1 + t * r1;
}

VGTool::VGTool() : canvas_item_editor(CanvasItemEditor::get_singleton()) {
}

class VGTransformTool : public VGTool {
	VGEditor *vg_editor;
	VGPath *path;

	struct Pos {
		Vector2 local;
		Vector2 global;
	};
	typedef Pos Pos4[4];

	enum Widget {
		WIDGET_NONE,
		WIDGET_CORNER,
		WIDGET_MIDPOINT,
		WIDGET_ROTATE
	};

	Widget clicked;
	Vector2 offset;

	struct {
		Pos corner;
		Pos opposite;
		Pos between;
		Vector2 u, v;
		Transform2D t;

		void init(const Pos4 &p, int i) {
			corner = p[i];
			opposite = p[(i + 2) % 4];
			between = p[(i + 1) % 4];

			u = (between.global - opposite.global).normalized();
			v = (between.global - corner.global).normalized();
		}
	} corner_widget;

	struct {
		Pos corner_0;
		Pos corner_1;
		Pos opposite_0;
		Pos opposite_1;
		Pos opposite_2;
		Vector2 u;
		Transform2D t;

		void init(const Pos4 &p, int i) {
			corner_0 = p[i];
			corner_1 = p[(i + 1) % 4];

			opposite_0 = p[(i + 3) % 4];
			opposite_1 = p[(i + 2) % 4];
			opposite_2 = p[(i + 1) % 4];

			u = (opposite_2.global - opposite_1.global).normalized();
		}
	} midpoint_widget;

	struct {
		Vector2 drag_from;
		Vector2 drag_rotation_center;
	} rotate_widget;

	void get_corners(Pos4 &r_pos, const Transform2D &p_xform) {
		//Rect2 r = tove_bounds_to_rect2(path->get_tove_path()->getBounds());
		Rect2 r = tove_bounds_to_rect2(path->get_subtree_graphics()->getBounds());
		Vector2 p[4];
		for (int i = 0; i < 4; i++) {
			const int j = i ^ (i >> 1);
			r_pos[i].local = Point2(r.position.x + ((j & 1) ? r.size.x : 0.0f), r.position.y + ((j & 2) ? r.size.y : 0.0f));
			r_pos[i].global = p_xform.xform(r_pos[i].local);
		}
	}

	Vector2 get_rotation_handle_position(const Pos4 &p_pos, Vector2 *r_origin = NULL) {
		float x00 = p_pos[0].global.x, y00 = p_pos[0].global.y;
		float x10 = p_pos[1].global.x, y10 = p_pos[1].global.y;
		float x11 = p_pos[2].global.x, y11 = p_pos[2].global.y;
		float phi = Math::atan2(y11 - y10, x11 - x10) + M_PI;
		float mx = (x00 + x10) / 2, my = (y00 + y10) / 2;
		float toplen = 30;
		if (r_origin) {
			*r_origin = Vector2(mx, my);
		}
		return Vector2(mx + toplen * Math::cos(phi), my + toplen * Math::sin(phi));
	}

	void update(const Vector2 &u1, const Vector2 &v1, const Vector2 &u2, const Vector2 &v2, const Vector2 &u3, const Vector2 &v3) {
		Transform2D t = mapping_to_transform(u1, v1, u2, v2, u3, v3);
		const float eps = 1e-3;
		if (t.elements[0].length() > eps && t.elements[1].length() > eps) {
			path->set_transform(t);
		}
	}

public:
	VGTransformTool(VGEditor *p_vg_editor, VGPath *p_path) :vg_editor(p_vg_editor), path(p_path) {
		clicked = WIDGET_NONE;
	}

	virtual bool forward_gui_input(const Ref<InputEvent> &p_event) {
		Ref<InputEventMouseButton> mb = p_event;
		if (mb.is_valid()) {
			if (mb->get_button_index() == BUTTON_LEFT && mb->is_pressed()) {
				const CanvasItem *pi = path->get_parent_item();
				Transform2D xform = canvas_item_editor->get_canvas_transform() * (pi ? pi->get_global_transform() : Transform2D());

				const real_t grab_threshold = EDITOR_DEF("editors/poly_editor/point_grab_radius", 8);

				clicked = WIDGET_NONE;
				Point2 q = mb->get_position();
				q = xform.affine_inverse().xform(q);

				Pos4 p;
				get_corners(p, path->get_transform());

				for (int i = 0; i < 4; i++) {
					if ((p[i].global - q).length() < grab_threshold) {
						offset = p[i].global - q;
						corner_widget.init(p, i);
						corner_widget.t = xform;
						clicked = WIDGET_CORNER;
						return true;
					}

					Vector2 midpoint = (p[i].global + p[(i + 1) % 4].global) / 2;
					if ((midpoint - q).length() < grab_threshold) {
						offset = midpoint - q;
						midpoint_widget.init(p, i);
						midpoint_widget.t = xform;
						clicked = WIDGET_MIDPOINT;
						return true;
					}
				}

				//

				xform = canvas_item_editor->get_canvas_transform() * path->get_global_transform();

				get_corners(p, xform);
				Vector2 rot_pos = get_rotation_handle_position(p);
				q = mb->get_position();

				if ((rot_pos - q).length() < grab_threshold) {
					offset = rot_pos - q;
					rotate_widget.drag_from = canvas_item_editor->get_canvas_transform().affine_inverse().xform(mb->get_position());

					Vector2 drag_rotation_center;
					if (path->_edit_use_pivot()) {
						drag_rotation_center = path->get_global_transform_with_canvas().xform(path->_edit_get_pivot());
					} else {
						drag_rotation_center = path->get_global_transform_with_canvas().get_origin();
					}
					rotate_widget.drag_rotation_center = drag_rotation_center;

					clicked = WIDGET_ROTATE;
					return true;
				}
			} else if (mb->get_button_index() == BUTTON_LEFT && clicked != WIDGET_NONE) {
				clicked = WIDGET_NONE;
				path->recenter();
				path->set_dirty(true);
				return true;
			}
		}

		Ref<InputEventMouseMotion> mm = p_event;

		if (clicked == WIDGET_NONE && path && mm.is_valid() && (mm->get_button_mask() & BUTTON_MASK_LEFT)) {
			return false;
		}

		if (clicked != WIDGET_NONE && path && mm.is_valid()) {
			if (mm->get_button_mask() & BUTTON_MASK_LEFT) {
				Vector2 g = mm->get_position();

				const CanvasItem *pi = path->get_parent_item();
				Transform2D xform = canvas_item_editor->get_canvas_transform() * (pi ? pi->get_global_transform() : Transform2D());
				g = xform.affine_inverse().xform(g);

				g = g + offset;

				switch (clicked) {
					case WIDGET_CORNER: {
						Vector2 p = ray_ray(corner_widget.opposite.global, corner_widget.u, g, corner_widget.v);

						update(
							corner_widget.between.local, p,
							corner_widget.opposite.local, corner_widget.opposite.global,
							corner_widget.corner.local, g);
					} break;
				
					case WIDGET_MIDPOINT: {
						Vector2 m = (midpoint_widget.opposite_0.global + midpoint_widget.opposite_1.global) / 2;
						Vector2 u = midpoint_widget.u;
						float l = (g - m).dot(u);

						update(
							midpoint_widget.opposite_0.local, midpoint_widget.opposite_0.global,
							midpoint_widget.opposite_1.local, midpoint_widget.opposite_1.global,
							(midpoint_widget.corner_0.local + midpoint_widget.corner_1.local) / 2, m + u * l);
					} break;

					case WIDGET_ROTATE: {
						Vector2 drag_rotation_center = rotate_widget.drag_rotation_center;
						Vector2 drag_from = rotate_widget.drag_from;
						Vector2 drag_to = canvas_item_editor->get_canvas_transform().affine_inverse().xform(mm->get_position());
						float phi = (drag_from - drag_rotation_center).angle_to(drag_to - drag_rotation_center);
						rotate_widget.drag_from = drag_to;

						path->_edit_set_rotation(canvas_item_editor->snap_angle(path->_edit_get_rotation() + phi, path->_edit_get_rotation()));
					} break;

					case WIDGET_NONE: {
					} break;
				}

				canvas_item_editor->get_viewport_control()->update();
				vg_editor->update();

				return true;
			}
		}

		return false;
	}

	virtual void forward_canvas_draw_over_viewport(Control *p_overlay) {
		Transform2D xform = canvas_item_editor->get_canvas_transform() * path->get_global_transform();
		const Ref<Texture> handle = vg_editor->get_icon("EditorHandle", "EditorIcons");
		Control *vpc = canvas_item_editor->get_viewport_control();
		
		Pos4 p;
		get_corners(p, xform);

		Vector2 rot_org;
		Vector2 rot_pos = get_rotation_handle_position(p, &rot_org);

		const Color line_color = Color(0.5, 0.5, 0.5);
		for (int i = 0; i < 4; i++) {
			vpc->draw_line(p[i].global, p[(i + 1) % 4].global, line_color, 2 * EDSCALE);
		}
		vpc->draw_line(rot_org, rot_pos, line_color, 2 * EDSCALE);

		for (int i = 0; i < 4; i++) {
			vpc->draw_texture(handle, p[i].global - handle->get_size() * 0.5);
		}

		for (int i = 0; i < 4; i++) {
			Vector2 q = (p[i].global + p[(i + 1) % 4].global) / 2;
			vpc->draw_texture(handle, q - handle->get_size() * 0.5);
		}

		vpc->draw_texture(handle, rot_pos - handle->get_size() * 0.5);
	}
};

class VGEllipseTool : public VGTool {
	VGPath *root;
	VGPath *path;

	tove::PathRef tove_path;
	Vector2 point0;

public:	
	VGEllipseTool(VGPath *p_root) : root(p_root) {
		path = NULL;
	}

	virtual bool forward_gui_input(const Ref<InputEvent> &p_event) {
		Ref<InputEventMouseButton> mb = p_event;
		if (mb.is_valid()) {
			if (mb->get_button_index() == BUTTON_LEFT) {
				if (mb->is_pressed() && path == NULL) {
					path = memnew(VGPath);
					root->add_child(path);
					if (root->get_owner()) {
						path->set_owner(root->get_owner());
					}

					tove::SubpathRef tove_subpath = std::make_shared<tove::Subpath>();
					tove_subpath->drawEllipse(0, 0, 0, 0);

					tove_path = std::make_shared<tove::Path>();
					tove_path->addSubpath(tove_subpath);
					tove_path->setFillColor(std::make_shared<tove::Color>(0.8, 0.1, 0.1));
					tove_path->setLineColor(std::make_shared<tove::Color>(0, 0, 0));

					path->set_tove_path(tove_path);

					point0 = global_to_local(root, mb->get_position());
				} else if (!mb->is_pressed()) {

					if (path && (global_to_local(root, mb->get_position()) - point0).length() < 2.0f) {
						root->remove_child(path);
						memdelete(path);
						path = NULL;
					} else if (path) {
						// commit

						//undo_redo->add_do_method(root, "add_vgpath");
						//undo_redo->add_undo_method(root, "remove_vgpath", root->get_child_count());
						
						//undo_redo->add_do_method(root, "set_child_fill_color", n, subpath_points_array(tove_subpath));
						//undo_redo->add_do_method(root, "set_child_line_color", n, subpath_points_array(tove_subpath));
						//undo_redo->add_do_method(root, "set_child_points", n, subpath_points_array(tove_subpath));
						//undo_redo->add_undo_method(root, "remove_child", n);
						//undo_redo->commit_action();
					}

					path = NULL;
				}
			}

			return true;
		}

		Ref<InputEventMouseMotion> mm = p_event;

		if (path && mm.is_valid()) {
			if (mm->get_button_mask() & BUTTON_MASK_LEFT) {
				// Vector2 gpoint = mm->get_position();
				Vector2 point1 = global_to_local(root, mm->get_position());

				Vector2 center = (point0 + point1) / 2;
				Vector2 radius = (point1 - center).abs();
				//float tx, ty = cx, cy

				tove::SubpathRef tove_subpath = std::make_shared<tove::Subpath>();
				tove_subpath->drawEllipse(0, 0, radius.x, radius.y);
				//tove_path->getSubpath(0)->set(tove_subpath);

				path->set_points(0, subpath_points_array(tove_subpath));
				path->set_position(center);

				//const float *bounds = path->get_tove_path()->getBounds();
				//printf("%f %f\n", bounds[0], bounds[1]);
			}

			return true;
		}

		return false;
	}
};

struct SubpathId {
	SubpathId();
	SubpathId(int p_subpath);

	bool operator==(const SubpathId &p_id) const;
	bool operator!=(const SubpathId &p_id) const;

	bool valid() const;

	int subpath;
};

struct Vertex : public SubpathId {
	Vertex();
	Vertex(int p_pt);
	Vertex(int p_subpath, int p_pt);

	bool operator==(const Vertex &p_vertex) const;
	bool operator!=(const Vertex &p_vertex) const;

	bool valid() const;

	int pt;
};

struct PosVertex : public Vertex {
	PosVertex();
	PosVertex(const Vertex &p_vertex, const Vector2 &p_pos);
	PosVertex(int p_subpath, int p_pt, const Vector2 &p_pos);

	Vector2 pos;
};

struct SubpathPos : public SubpathId {
	SubpathPos();
	SubpathPos(int p_subpath, float p_t);

	bool valid() const;

	float t;
};

class PointIterator {
private:
	tove::PathRef path_ref;
	int n_subpaths;
	int subpath;
	tove::SubpathRef subpath_ref;
	int pt;
	int n_pts;
	bool closed;

public:
	PointIterator(
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

	// int path;
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
		const tove::PathRef &p_path_ref,
		const Vertex &p_selected_point,
		const Vertex &p_edited_point);

	bool next();

	PosVertex get_pos_vertex() const;
	Vector2 get_pos() const;
	Vector2 get_knot_pos() const;
};

class VGCurveTool : public VGTool {
	// VGPath *root;
	VGPath *node_vg;

	VGEditor *vg_editor;
	EditorNode *editor;
	UndoRedo *undo_redo;

	PosVertex edited_point;
	SubpathId edited_path;
	Vertex hover_point; // point under mouse cursor
	Vertex selected_point; // currently selected

	Array pre_move_edit;

	uint64_t mb_down_time;
	Vector2 mb_down_where;
	SubpathPos mb_down_at;

	void remove_point(const Vertex &p_vertex);
	Vertex get_active_point() const;
	PosVertex closest_point(const Vector2 &p_pos, bool p_cp = false) const;
	SubpathPos closest_subpath_point(const Vector2 &p_pos) const;
	Array _get_points(const SubpathId &p_id);

	void _commit_action();
	bool _is_empty() const;

	VGPath *_get_node() const {
		return node_vg;
	}

public:	
	VGCurveTool(VGEditor *p_vg_editor, VGPath *p_path) :
		vg_editor(p_vg_editor), editor(p_vg_editor->get_editor_node()) {

		node_vg = p_path;

		undo_redo = editor->get_undo_redo();

		edited_point = PosVertex();
		edited_path = SubpathId();
		hover_point = Vertex();
		selected_point = Vertex();
	}

	virtual bool forward_gui_input(const Ref<InputEvent> &p_event) {
		Ref<InputEventMouseButton> mb = p_event;

		if (mb.is_valid()) {

			Transform2D xform = canvas_item_editor->get_canvas_transform() * node_vg->get_global_transform();

			if (mb->get_button_index() == BUTTON_LEFT && mb->is_pressed() && mb->is_doubleclick()) {
				Point2 p = xform.affine_inverse().xform(mb->get_position());
				VGPath *clicked = node_vg->find_clicked_child(p);
				if (clicked) {
					editor->push_item(clicked);
				}
				return true;
			}

			Vector2 gpoint = mb->get_position();
			// Vector2 cpoint = node_vg->get_global_transform().affine_inverse().xform(canvas_item_editor->snap_point(canvas_item_editor->get_canvas_transform().affine_inverse().xform(mb->get_position())));

			if (true) {

				if (mb->get_button_index() == BUTTON_LEFT) {

					if (mb->is_pressed()) {

						const PosVertex closest = closest_point(gpoint, true);
						const SubpathPos insert = closest.valid() ? SubpathPos() : closest_subpath_point(gpoint);

						edited_point = PosVertex();
						edited_path = SubpathId();
						hover_point = Vertex();

						if (insert.valid()) {

							pre_move_edit = _get_points(insert);
							mb_down_time = OS::get_singleton()->get_ticks_msec();
							mb_down_where = gpoint;
							mb_down_at = insert;

							return true;
						} else if (closest.valid()) {

							pre_move_edit = _get_points(closest);
							edited_point = PosVertex(closest, xform.affine_inverse().xform(closest.pos));
							edited_path = edited_point;
							mb_down_time = 0;
							if (!is_control_point(closest.pt)) {
								selected_point = closest;
							}
							canvas_item_editor->get_viewport_control()->update();
							return true;
						} else {

							if (selected_point.valid()) {
								selected_point = Vertex();
								canvas_item_editor->get_viewport_control()->update();
							}

							return false;
						}
					} else {
						const bool clicked = !edited_path.valid() && mb_down_time > 0 &&
							OS::get_singleton()->get_ticks_msec() - mb_down_time < 500;
						mb_down_time = 0;
						edited_point = PosVertex();

						if (clicked) {
							undo_redo->create_action(TTR("Insert Curve"));

							const SubpathPos insert = mb_down_at;
							undo_redo->add_do_method(node_vg, "insert_curve",
								insert.subpath, insert.t);
							undo_redo->add_undo_method(node_vg, "set_points",
								insert.subpath, pre_move_edit);

							_commit_action();

							selected_point = Vertex(insert.subpath, 3 * (1 + int(Math::floor(insert.t))));
							return true;
						} else if (edited_path.valid()) {

							undo_redo->create_action(TTR("Edit Path"));

							undo_redo->add_do_method(node_vg, "set_points",
								edited_path.subpath, _get_points(edited_path));
							undo_redo->add_undo_method(node_vg, "set_points",
								edited_path.subpath, pre_move_edit);

							edited_path = SubpathId();
							_commit_action();

							return true;
						}
					}
				} else if (mb->get_button_index() == BUTTON_RIGHT && mb->is_pressed() && !edited_point.valid()) {

					const PosVertex closest = closest_point(gpoint);

					if (closest.valid()) {

						remove_point(closest);
						return true;
					}
				}
			} /*else if (mode == MODE_DELETE) {

				if (mb->get_button_index() == BUTTON_LEFT && mb->is_pressed()) {

					const PosVertex closest = closest_point(gpoint);

					if (closest.valid()) {

						remove_point(closest);
						return true;
					}
				}
			}*/

			if (mb->get_button_index() == BUTTON_LEFT && mb->is_pressed()) {

				return true;
			}
		}

		Ref<InputEventMouseMotion> mm = p_event;

		if (mm.is_valid()) {

			Vector2 gpoint = mm->get_position();

			if (mm->get_button_mask() & BUTTON_MASK_LEFT) {
				if (mb_down_time > 0 && gpoint.distance_to(mb_down_where) > 2) {
					edited_path = mb_down_at;
					mb_down_time = 0;
				}

				if (edited_point.valid()) {
					Vector2 cpoint = _get_node()->get_global_transform().affine_inverse().xform(canvas_item_editor->snap_point(canvas_item_editor->get_canvas_transform().affine_inverse().xform(gpoint)));
					//cpoint = node_vg->get_vg_transform().affine_inverse().xform(cpoint);
					edited_point = PosVertex(edited_point, cpoint);

					tove::SubpathRef subpath = node_vg->get_subpath(edited_point.subpath);
					subpath->move(edited_point.pt, cpoint.x, cpoint.y);

					node_vg->set_dirty();
					canvas_item_editor->get_viewport_control()->update();
				} else if (edited_path.valid()) {
					Vector2 cpoint = _get_node()->get_global_transform().affine_inverse().xform(canvas_item_editor->snap_point(canvas_item_editor->get_canvas_transform().affine_inverse().xform(gpoint)));
					//cpoint = node_vg->get_vg_transform().affine_inverse().xform(cpoint);
					tove::SubpathRef subpath = node_vg->get_subpath(mb_down_at.subpath);
					subpath->mould(mb_down_at.t, cpoint.x, cpoint.y);

					node_vg->set_dirty();
					canvas_item_editor->get_viewport_control()->update();
				}
			} else if (true) {

				const PosVertex new_hover_point = closest_point(gpoint);
				if (hover_point != new_hover_point) {

					hover_point = new_hover_point;
					canvas_item_editor->get_viewport_control()->update();
				}
			}
		}

		Ref<InputEventKey> k = p_event;

		if (k.is_valid() && k->is_pressed()) {

			if (k->get_scancode() == KEY_DELETE || k->get_scancode() == KEY_BACKSPACE) {

				const Vertex active_point = get_active_point();

				if (active_point.valid()) {

					remove_point(active_point);
					return true;
				}
			}
		}

		return false;
	}

	void forward_canvas_draw_over_viewport(Control *p_overlay)
	{
		Control *vpc = canvas_item_editor->get_viewport_control();
		Transform2D xform = canvas_item_editor->get_canvas_transform() *
			_get_node()->get_global_transform();
		const Ref<Texture> handle = vg_editor->get_icon("EditorHandle", "EditorIcons");

		const tove::PathRef path = node_vg->get_tove_path();

		{
			const Color line_color = Color(0.5, 0.5, 0.5);
			ControlPointIterator cp_iterator(path, selected_point, edited_point);
			while (cp_iterator.next()) {
				Vector2 p = xform.xform(cp_iterator.get_knot_pos());
				Vector2 q = xform.xform(cp_iterator.get_pos());
				vpc->draw_line(p, q, line_color, 2 * EDSCALE);
			}
		}	

		{
			const Vertex active_point = get_active_point();

			PointIterator pt_iterator(path);
			while (pt_iterator.next()) {
				Vector2 p = xform.xform(pt_iterator.get_pos());
				const Color modulate = (pt_iterator.get_vertex() == active_point) ?
					Color(0.5, 1, 2) : Color(1, 1, 1);
				vpc->draw_texture(handle, p - handle->get_size() * 0.5, modulate);
			}		
		}

		{
			ControlPointIterator cp_iterator(path, selected_point, edited_point);
			const Size2 s = handle->get_size() * 0.75;
			while (cp_iterator.next()) {
				Vector2 p = xform.xform(cp_iterator.get_pos());
				Rect2 r = Rect2(p - s / 2, s);
				vpc->draw_texture_rect(handle, r, false, Color(1, 1, 1));
			}
		}		
	}
};

/* static void reown_children(Node *p_node) {
	const int n = p_node->get_child_count();
	for (int i = 0; i < n; i++) {
		Node *child = p_node->get_child(i);
		child->force_parent_owned();
		reown_children(child);
	}
} */

Array VGCurveTool::_get_points(const SubpathId &p_id) {
	tove::PathRef path = node_vg->get_tove_path();
	return subpath_points_array(path->getSubpath(p_id.subpath));
}

Node2D *VGEditor::_get_node() const {
    return node_vg;
}

void VGEditor::_set_node(Node *p_node_vg) {
	if (node_vg) {
		node_vg->remove_change_receptor(this);
	}
    node_vg = Object::cast_to<VGPath>(p_node_vg);
	if (node_vg) {
		node_vg->add_change_receptor(this);
	}
	if (!node_vg) {
		tool = Ref<VGTool>();
	}
}

SubpathId::SubpathId() :
	subpath(-1) {
}

SubpathId::SubpathId(int p_subpath) :
	subpath(p_subpath) {
}

bool SubpathId::operator==(const SubpathId &p_id) const {

	return subpath == p_id.subpath;
}

bool SubpathId::operator!=(const SubpathId &p_id) const {

	return !(*this == p_id);
}

bool SubpathId::valid() const {

	return subpath >= 0;
}

Vertex::Vertex() :
	pt(-1) {
	// invalid vertex
}

Vertex::Vertex(int p_pt) :
	pt(p_pt) {
}

Vertex::Vertex(int p_subpath, int p_pt) :
	SubpathId(p_subpath),
	pt(p_pt) {
}

bool Vertex::operator==(const Vertex &p_vertex) const {

	return subpath == p_vertex.subpath && pt == p_vertex.pt;
}

bool Vertex::operator!=(const Vertex &p_vertex) const {

	return !(*this == p_vertex);
}

bool Vertex::valid() const {

	return pt >= 0;
}


PosVertex::PosVertex() {
	// invalid vertex
}

PosVertex::PosVertex(const Vertex &p_vertex, const Vector2 &p_pos) :
	Vertex(p_vertex.subpath, p_vertex.pt),
	pos(p_pos) {
}

PosVertex::PosVertex(int p_subpath, int p_pt, const Vector2 &p_pos) :
	Vertex(p_subpath, p_pt),
	pos(p_pos) {
}


SubpathPos::SubpathPos() :
	t(-1) {
	// invalid vertex
}

SubpathPos::SubpathPos(int p_subpath, float p_t) :
	SubpathId(p_subpath),
	t(p_t) {
	// vertex p_vertex of polygon p_polygon
}

bool SubpathPos::valid() const {
	return t >= 0;
}



bool VGEditor::_is_empty() const {

	if (!_get_node())
		return true;

	return node_vg->is_empty() == 0;
}

bool VGEditor::_has_resource() const {

	return node_vg;
}

void VGEditor::_create_resource() {
}

void VGEditor::_tool_selected(int p_tool) {

	for (int i = 0; i < tool_buttons.size(); i++) {
		tool_buttons[i]->set_pressed(i == p_tool);
	}

	if (!node_vg) {
		tool = Ref<VGTool>();
	} else {
		switch (p_tool) {
			case TOOL_TRANSFORM:
				tool = Ref<VGTool>(memnew(VGTransformTool(this, node_vg)));
				break;

			case TOOL_CURVE:
				tool = Ref<VGTool>(memnew(VGCurveTool(this, node_vg)));
				break;

			case TOOL_ELLIPSE:
				tool = Ref<VGTool>(memnew(VGEllipseTool(node_vg)));
				break;
		}
	}

	canvas_item_editor->get_viewport_control()->update();
}

void VGEditor::_create_mesh_node() {

	if (!node_vg) {
		return;
	}

	MeshInstance2D *baked = node_vg->create_mesh_node();

	Node *parent = node_vg->get_parent();

	undo_redo->create_action(TTR("Create Mesh Node"));
	undo_redo->add_do_method(parent, "add_child_below_node", node_vg, baked);
	undo_redo->add_do_method(baked, "set_owner", node_vg->get_owner());
	undo_redo->add_undo_method(parent, "remove_child", baked);
	undo_redo->add_do_reference(baked);
	undo_redo->commit_action();

	//add_child_below_node(baked, node_vg, true);
	/*reown_children(node_vg);
	EditorNode::get_singleton()->get_scene_tree_dock()->replace_node(node_vg, baked);
	edit(NULL);*/
}

void VGEditor::_notification(int p_what) {

	switch (p_what) {

		case NOTIFICATION_READY: {

			tool_buttons[0]->set_icon(EditorNode::get_singleton()->get_gui_base()->get_icon("ToolSelect", "EditorIcons"));
			tool_buttons[1]->set_icon(EditorNode::get_singleton()->get_gui_base()->get_icon("EditBezier", "EditorIcons"));
			tool_buttons[2]->set_icon(EditorNode::get_singleton()->get_gui_base()->get_icon("SphereShape", "EditorIcons"));

			button_bake->set_icon(EditorNode::get_singleton()->get_gui_base()->get_icon("MeshInstance2D", "EditorIcons"));			

			get_tree()->connect("node_removed", this, "_node_removed");

			create_resource->connect("confirmed", this, "_create_resource");

		} break;
		case NOTIFICATION_PHYSICS_PROCESS: {

		} break;
	}
}

void VGEditor::_node_removed(Node *p_node) {

	if (p_node == _get_node()) {
		edit(NULL);
		hide();

		canvas_item_editor->get_viewport_control()->update();
	}
}

bool VGEditor::forward_gui_input(const Ref<InputEvent> &p_event) {

	if (tool_buttons[TOOL_TRANSFORM]->is_pressed()) {
		Ref<InputEventMouseButton> mb = p_event;
		if (mb.is_valid() && mb->get_button_index() == BUTTON_LEFT && mb->is_pressed() && mb->is_doubleclick()) {
			_tool_selected(TOOL_CURVE);
		}
	}

	if (tool.is_valid()) {
		return tool->forward_gui_input(p_event);
	}

	if (!_get_node())
		return false;

	Ref<InputEventMouseButton> mb = p_event;

	if (!_has_resource()) {

		if (mb.is_valid() && mb->get_button_index() == 1 && mb->is_pressed()) {
			create_resource->set_text(String("No vector graphics resource on this node.\nCreate and assign one?"));
			create_resource->popup_centered_minsize();
		}
		return (mb.is_valid() && mb->get_button_index() == 1);
	}

	return false;
}

PointIterator::PointIterator(
	const tove::PathRef &p_path_ref) :
	
	path_ref(p_path_ref) {

	n_subpaths = path_ref->getNumSubpaths();

	subpath = -1;
	pt = 0;
	n_pts = 0;
}

bool PointIterator::next() {
	pt += 3;

	while (pt >= n_pts - (closed ? 3 : 0)) {
		if (subpath + 1 >= n_subpaths) {
			return false;
		}
		subpath += 1;
		subpath_ref = path_ref->getSubpath(subpath);

		pt = 0;
		n_pts = subpath_ref->getNumPoints();
		closed = subpath_ref->isClosed();
	}

	return true;
}

Vertex PointIterator::get_vertex() const {
	return Vertex(subpath, pt);
}

PosVertex PointIterator::get_pos_vertex() const {
	return PosVertex(subpath, pt, get_pos());
}

Vector2 PointIterator::get_pos() const {
	ERR_FAIL_INDEX_V(pt, n_pts, Vector2(0, 0));
	const float *points = subpath_ref->getPoints();
	const int i = 2 * pt;
	return Vector2(points[i + 0], points[i + 1]);
}

inline int knot_point(int pt) {
	switch (pt % 3) { // * . . *
		case 1:
			return pt - 1;
		case 2:
			return pt + 1;
		default:
			return pt;
	}
}

ControlPointIterator::ControlPointIterator(
	const tove::PathRef &p_path_ref,
	const Vertex &p_selected_point,
	const Vertex &p_edited_point) :
	
	selected_point(p_selected_point),
	edited_point(p_edited_point),
	path_ref(p_path_ref) {

	n_subpaths = path_ref->getNumSubpaths();
	edited_pt0 = knot_point(edited_point.pt);

	subpath = -1;
	pt = 0;
	d_pt = -1;
	n_pts = 0;
}

bool ControlPointIterator::is_visible() const {
	if (edited_point.subpath == subpath && edited_pt0 == pt) {
		return true;
	} else if (selected_point.subpath == subpath) {
		int distance = ABS(selected_point.pt - (pt + d_pt));
		distance = MIN(distance, n_pts - distance - (closed ? 1 : 0));
		return distance <= 2;
	} else {
		return false;
	}
}

bool ControlPointIterator::next() {
	while (true) {
		if (d_pt < 0) {
			d_pt = 1;
		} else {
			d_pt = -1;
			pt += 3;
		}

		while (pt + d_pt >= n_pts) {
			if (subpath + 1 >= n_subpaths) {
				return false;
			}
			subpath += 1;
			subpath_ref = path_ref->getSubpath(subpath);

			pt = 0;
			d_pt = -1;
			n_pts = subpath_ref->getNumPoints();
			closed = subpath_ref->isClosed();
		}

		if (is_visible()) {
			return true;
		}
	}
}

PosVertex ControlPointIterator::get_pos_vertex() const {
	return PosVertex(subpath, pt + d_pt, get_pos());
}

Vector2 ControlPointIterator::get_pos() const {
	int j = pt + d_pt;
	if (j < 0 && closed) {
		j += n_pts;
	}
	ERR_FAIL_INDEX_V(j, n_pts, Vector2(0, 0));
	const float *points = subpath_ref->getPoints();
	const int i = 2 * j;
	return Vector2(points[i + 0], points[i + 1]);
}

Vector2 ControlPointIterator::get_knot_pos() const {
	ERR_FAIL_INDEX_V(pt, n_pts, Vector2(0, 0));
	const float *points = subpath_ref->getPoints();
	const int i = 2 * pt;
	return Vector2(points[i + 0], points[i + 1]);
}

void VGEditor::forward_canvas_draw_over_viewport(Control *p_overlay) {

	if (!node_vg)
		return;

	Control *vpc = canvas_item_editor->get_viewport_control();
	/* Transform2D xform = canvas_item_editor->get_canvas_transform() *
		_get_node()->get_global_transform(); */
	const Ref<Texture> handle = get_icon("EditorHandle", "EditorIcons");

	_update_overlay();
	if (overlay.is_valid()) {
		vpc->draw_set_transform_matrix(overlay_draw_xform);
		vpc->draw_mesh(overlay, Ref<Texture>(), Ref<Texture>());
		vpc->draw_set_transform_matrix(Transform2D());
	}

	if (tool.is_valid()) {
		tool->forward_canvas_draw_over_viewport(p_overlay);
		return;
	}
}

void VGEditor::edit(Node *p_node_vg) {

	if (p_node_vg == node_vg) {
		return;
	}

	if (!canvas_item_editor) {
		canvas_item_editor = CanvasItemEditor::get_singleton();
	}

	if (p_node_vg) {

		_set_node(p_node_vg);
		_tool_selected(TOOL_TRANSFORM);
		_update_overlay(true);

	} else {

		_set_node(NULL);
		overlay = Ref<Mesh>();
		_update_overlay(true);
	}

	canvas_item_editor->get_viewport_control()->update();
}

void VGEditor::_bind_methods() {

	ClassDB::bind_method(D_METHOD("_node_removed"), &VGEditor::_node_removed);
	ClassDB::bind_method(D_METHOD("_tool_selected"), &VGEditor::_tool_selected);
	ClassDB::bind_method(D_METHOD("_create_mesh_node"), &VGEditor::_create_mesh_node);
	ClassDB::bind_method(D_METHOD("_create_resource"), &VGEditor::_create_resource);
}

void VGCurveTool::_commit_action() {

	undo_redo->add_do_method(canvas_item_editor->get_viewport_control(), "update");
	undo_redo->add_undo_method(canvas_item_editor->get_viewport_control(), "update");
	undo_redo->commit_action();
}

bool VGCurveTool::_is_empty() const {

	if (!_get_node())
		return true;

	return node_vg->is_empty() == 0;
}

void VGCurveTool::remove_point(const Vertex &p_vertex) {

	tove::SubpathRef subpath = node_vg->get_subpath(p_vertex.subpath);

	if (subpath->getNumPoints() >= 4) {

		undo_redo->create_action(TTR("Remove Subpath Curve"));

		Array previous = subpath_points_array(subpath);
		undo_redo->add_do_method(node_vg, "remove_curve",
			p_vertex.subpath, p_vertex.pt / 3);
		undo_redo->add_undo_method(node_vg, "set_points",
			p_vertex.subpath, previous);

		_commit_action();
	} else {


	}

	canvas_item_editor->get_viewport_control()->update();

	//if (_is_empty())
	//	_tool_selected(0);

	hover_point = Vertex();
	if (selected_point == p_vertex)
		selected_point = Vertex();
}

Vertex VGCurveTool::get_active_point() const {

	return hover_point.valid() ? hover_point : selected_point;
}

static Vector2 basis_xform_inv(const Transform2D &xform, const Vector2 &v) {
	return xform.affine_inverse().basis_xform(v);
}

static Vector2 xform_inv(const Transform2D &xform, const Vector2 &p) {
	return xform.affine_inverse().xform(p);
}

PosVertex VGCurveTool::closest_point(const Vector2 &p_pos, bool p_cp) const {

	if (!node_vg) {
		return PosVertex();
	}

	const real_t grab_threshold = EDITOR_DEF("editors/poly_editor/point_grab_radius", 8);

	const Transform2D xform = canvas_item_editor->get_canvas_transform() *
		_get_node()->get_global_transform();

	const Vector2 pos = xform_inv(xform, p_pos);
	const float eps = basis_xform_inv(xform, Vector2(grab_threshold, 0)).length();

	PosVertex closest;
	real_t closest_dist = eps;

	const tove::PathRef path = node_vg->get_tove_path();

	{
		PointIterator pt_iterator(path);
		while (pt_iterator.next()) {
			real_t d = pt_iterator.get_pos().distance_to(pos);
			if (d < closest_dist) {
				closest_dist = d;
				closest = pt_iterator.get_pos_vertex();
			}
		}
	}

	if (p_cp) {

		ControlPointIterator cp_iterator(path, selected_point, edited_point);
		while (cp_iterator.next()) {
			real_t d = cp_iterator.get_pos().distance_to(pos);
			if (d < closest_dist) {
				closest_dist = d;
				closest = cp_iterator.get_pos_vertex();
			}
		}
	}
	

	return closest;
}

SubpathPos VGCurveTool::closest_subpath_point(const Vector2 &p_pos) const {

	if (!node_vg) {
		return SubpathPos();
	}

	const real_t grab_threshold = EDITOR_DEF("editors/poly_editor/point_grab_radius", 8);

	const Transform2D xform = canvas_item_editor->get_canvas_transform() *
		_get_node()->get_global_transform();

	const Vector2 pos = xform_inv(xform, p_pos);
	float dmin = basis_xform_inv(xform, Vector2(0.5, 0)).length();
	float dmax = basis_xform_inv(xform, Vector2(grab_threshold, 0)).length();

	const int n_subpaths = node_vg->get_num_subpaths();
	for (int j = 0; j < n_subpaths; j++) {
		const tove::SubpathRef subpath = node_vg->get_subpath(j);
	    ToveNearest nearest = subpath->nearest(pos.x, pos.y, dmin, dmax);
		if (nearest.t >= 0) {
			return SubpathPos(j, nearest.t);
		}
	}

	return SubpathPos();
}

bool VGEditor::is_inside(const Vector2 &p_pos) const {

	if (!node_vg) {
		return false;
	}

	const Transform2D xform = canvas_item_editor->get_canvas_transform() *
		_get_node()->get_global_transform();

	return node_vg->is_inside(xform_inv(xform, p_pos));
}

void VGEditor::_update_overlay(bool p_always_update) {
	if (!node_vg) {
		overlay = Ref<Mesh>();
		return;
	}

	Transform2D xform = canvas_item_editor->get_canvas_transform() *
		node_vg->get_global_transform();

	overlay_draw_xform = Transform2D().translated(xform.elements[2]);

	if (!p_always_update) {
		float err = 0.0f;
		for (int i = 0; i < 2; i++) {
			err += (xform.elements[i] - overlay_full_xform.elements[i]).length_squared();
		}
		if (err < 0.001) {
			return;
		}
	}

	tove::nsvg::Transform transform(
		xform.elements[0].x, xform.elements[1].x, 0,
		xform.elements[0].y, xform.elements[1].y, 0
	);

	tove::PathRef tove_path = tove::tove_make_shared<tove::Path>();
	tove_path->set(node_vg->get_tove_path(), transform);

	tove::GraphicsRef overlay_graphics = tove::tove_make_shared<tove::Graphics>();
	overlay_graphics->addPath(tove_path);

	tove_path->setFillColor(tove::PaintRef());
	tove_path->setLineColor(tove::tove_make_shared<tove::Color>(0.4, 0.4, 0.8));
	tove_path->setLineWidth(2);

    overlay = Ref<ArrayMesh>(memnew(ArrayMesh));
	if (overlay_graphics->getNumPaths() > 0) {
		tove::TesselatorRef tesselator = tove::tove_make_shared<tove::AdaptiveTesselator>(
			new tove::AdaptiveFlattener<tove::DefaultCurveFlattener>(
				tove::DefaultCurveFlattener(100, 6)));
	    tove::MeshRef tove_mesh = tove::tove_make_shared<tove::ColorMesh>(); 
		tesselator->graphicsToMesh(overlay_graphics.get(), UPDATE_MESH_EVERYTHING, tove_mesh, tove_mesh);
		copy_mesh(overlay, tove_mesh);
	}
	overlay_full_xform = xform;
}

void VGEditor::_changed_callback(Object *p_changed, const char *p_prop) {
	if (node_vg && node_vg == p_changed && strcmp(p_prop, "path_shape") == 0) {
		_update_overlay(true);
		canvas_item_editor->get_viewport_control()->update();
	}
}

VGEditor::VGEditor(EditorNode *p_editor) {
    node_vg = NULL;

	canvas_item_editor = NULL;
	editor = p_editor;
	undo_redo = editor->get_undo_redo();

	add_child(memnew(VSeparator));
	
	const int n_tools = 3;
	for (int i = 0; i < n_tools; i++) {
		ToolButton *button = memnew(ToolButton);
		add_child(button);
		button->connect("pressed", this, "_tool_selected", varray(i));
		button->set_toggle_mode(true);
		button->set_pressed(i == 0);
		tool_buttons.push_back(button);
	}

	VSeparator *sep = memnew(VSeparator);
	sep->set_h_size_flags(SIZE_EXPAND_FILL);
	add_child(sep);

	button_bake = memnew(ToolButton);
	add_child(button_bake);
	button_bake->connect("pressed", this, "_create_mesh_node");
	button_bake->set_tooltip(TTR("Bake into mesh"));

	create_resource = memnew(ConfirmationDialog);
	add_child(create_resource);
	create_resource->get_ok()->set_text(TTR("Create"));
}
