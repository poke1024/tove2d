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

static void reown_children(Node *p_node) {
	const int n = p_node->get_child_count();
	for (int i = 0; i < n; i++) {
		Node *child = p_node->get_child(i);
		child->force_parent_owned();
		reown_children(child);
	}
}

Array VGEditor::_get_points(const SubpathId &p_id) {
	tove::PathRef path = node_vg->get_tove_path();
	return subpath_points_array(path->getSubpath(p_id.subpath));
}

Node2D *VGEditor::_get_node() const {
    return node_vg;
}

void VGEditor::_set_node(Node *p_node_vg) {
    node_vg = Object::cast_to<VGPath>(p_node_vg);
}

VGEditor::SubpathId::SubpathId() :
	subpath(-1) {
}

VGEditor::SubpathId::SubpathId(int p_subpath) :
	subpath(p_subpath) {
}

bool VGEditor::SubpathId::operator==(const VGEditor::SubpathId &p_id) const {

	return subpath == p_id.subpath;
}

bool VGEditor::SubpathId::operator!=(const VGEditor::SubpathId &p_id) const {

	return !(*this == p_id);
}

bool VGEditor::SubpathId::valid() const {

	return subpath >= 0;
}

VGEditor::Vertex::Vertex() :
	pt(-1) {
	// invalid vertex
}

VGEditor::Vertex::Vertex(int p_pt) :
	pt(p_pt) {
}

VGEditor::Vertex::Vertex(int p_subpath, int p_pt) :
	SubpathId(p_subpath),
	pt(p_pt) {
}

bool VGEditor::Vertex::operator==(const VGEditor::Vertex &p_vertex) const {

	return subpath == p_vertex.subpath && pt == p_vertex.pt;
}

bool VGEditor::Vertex::operator!=(const VGEditor::Vertex &p_vertex) const {

	return !(*this == p_vertex);
}

bool VGEditor::Vertex::valid() const {

	return pt >= 0;
}


VGEditor::PosVertex::PosVertex() {
	// invalid vertex
}

VGEditor::PosVertex::PosVertex(const Vertex &p_vertex, const Vector2 &p_pos) :
	Vertex(p_vertex.subpath, p_vertex.pt),
	pos(p_pos) {
}

VGEditor::PosVertex::PosVertex(int p_subpath, int p_pt, const Vector2 &p_pos) :
	Vertex(p_subpath, p_pt),
	pos(p_pos) {
}


VGEditor::SubpathPos::SubpathPos() :
	t(-1) {
	// invalid vertex
}

VGEditor::SubpathPos::SubpathPos(int p_subpath, float p_t) :
	SubpathId(p_subpath),
	t(p_t) {
	// vertex p_vertex of polygon p_polygon
}

bool VGEditor::SubpathPos::valid() const {
	return t >= 0;
}



bool VGEditor::_is_empty() const {

	if (!_get_node())
		return true;

	return node_vg->is_empty() == 0;
}

void VGEditor::_commit_action() {

	undo_redo->add_do_method(canvas_item_editor->get_viewport_control(), "update");
	undo_redo->add_undo_method(canvas_item_editor->get_viewport_control(), "update");
	undo_redo->commit_action();
}

bool VGEditor::_has_resource() const {

	return node_vg;
}

void VGEditor::_create_resource() {
}

void VGEditor::_menu_option(int p_option) {

	switch (p_option) {

		case MODE_CREATE: {

			mode = MODE_CREATE;
			button_create->set_pressed(true);
			button_edit->set_pressed(false);
			button_delete->set_pressed(false);
		} break;
		case MODE_EDIT: {

			mode = MODE_EDIT;
			button_create->set_pressed(false);
			button_edit->set_pressed(true);
			button_delete->set_pressed(false);
		} break;
		case MODE_DELETE: {

			mode = MODE_DELETE;
			button_create->set_pressed(false);
			button_edit->set_pressed(false);
			button_delete->set_pressed(true);
		} break;
	}
}

void VGEditor::_create_mesh_node() {

	if (!node_vg) {
		return;
	}

	MeshInstance2D *baked = node_vg->create_mesh_node();
	reown_children(node_vg);
	EditorNode::get_singleton()->get_scene_tree_dock()->replace_node(node_vg, baked);
	edit(NULL);
}

void VGEditor::_notification(int p_what) {

	switch (p_what) {

		case NOTIFICATION_READY: {

			button_create->set_icon(EditorNode::get_singleton()->get_gui_base()->get_icon("CurveCreate", "EditorIcons"));
			button_edit->set_icon(EditorNode::get_singleton()->get_gui_base()->get_icon("CurveEdit", "EditorIcons"));
			button_delete->set_icon(EditorNode::get_singleton()->get_gui_base()->get_icon("CurveDelete", "EditorIcons"));
			button_edit->set_pressed(true);

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

	if (mb.is_valid()) {

		Transform2D xform = canvas_item_editor->get_canvas_transform() * _get_node()->get_global_transform();

		if (mb->get_button_index() == BUTTON_LEFT && mb->is_pressed() && mb->is_doubleclick()) {
			Point2 p = xform.affine_inverse().xform(mb->get_position());
			VGPath *clicked = node_vg->find_clicked_child(p);
			if (clicked) {
				editor->push_item(clicked);
			}
			return true;
		}

		Vector2 gpoint = mb->get_position();
		Vector2 cpoint = _get_node()->get_global_transform().affine_inverse().xform(canvas_item_editor->snap_point(canvas_item_editor->get_canvas_transform().affine_inverse().xform(mb->get_position())));

		if (mode == MODE_EDIT || mode == MODE_CREATE) {

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
		} else if (mode == MODE_DELETE) {

			if (mb->get_button_index() == BUTTON_LEFT && mb->is_pressed()) {

				const PosVertex closest = closest_point(gpoint);

				if (closest.valid()) {

					remove_point(closest);
					return true;
				}
			}
		}

		if (mode == MODE_CREATE) {

			if (mb->get_button_index() == BUTTON_LEFT && mb->is_pressed()) {

				return true;
			}
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
				edited_point = PosVertex(edited_point, cpoint);

				tove::SubpathRef subpath = node_vg->get_subpath(edited_point.subpath);
				subpath->move(edited_point.pt, cpoint.x, cpoint.y);

				node_vg->set_dirty();
				canvas_item_editor->get_viewport_control()->update();
			} else if (edited_path.valid()) {
				Vector2 cpoint = _get_node()->get_global_transform().affine_inverse().xform(canvas_item_editor->snap_point(canvas_item_editor->get_canvas_transform().affine_inverse().xform(gpoint)));
				tove::SubpathRef subpath = node_vg->get_subpath(mb_down_at.subpath);
				subpath->mould(mb_down_at.t, cpoint.x, cpoint.y);

				node_vg->set_dirty();
				canvas_item_editor->get_viewport_control()->update();
			}
		} else if (mode == MODE_EDIT || mode == MODE_CREATE) {

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

VGEditor::PointIterator::PointIterator(
	const tove::PathRef &p_path_ref) :
	
	path_ref(p_path_ref) {

	n_subpaths = path_ref->getNumSubpaths();

	subpath = -1;
	pt = 0;
	n_pts = 0;
}

bool VGEditor::PointIterator::next() {
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

VGEditor::Vertex VGEditor::PointIterator::get_vertex() const {
	return Vertex(subpath, pt);
}

VGEditor::PosVertex VGEditor::PointIterator::get_pos_vertex() const {
	return PosVertex(subpath, pt, get_pos());
}

Vector2 VGEditor::PointIterator::get_pos() const {
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

VGEditor::ControlPointIterator::ControlPointIterator(
	const tove::PathRef &p_path_ref,
	const Vertex &p_selected_point,
	const Vertex &p_edited_point) :
	
	path_ref(p_path_ref),
	selected_point(p_selected_point),
	edited_point(p_edited_point) {

	n_subpaths = path_ref->getNumSubpaths();
	edited_pt0 = knot_point(edited_point.pt);

	subpath = -1;
	pt = 0;
	d_pt = -1;
	n_pts = 0;
}

bool VGEditor::ControlPointIterator::is_visible() const {
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

bool VGEditor::ControlPointIterator::next() {
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

VGEditor::PosVertex VGEditor::ControlPointIterator::get_pos_vertex() const {
	return VGEditor::PosVertex(subpath, pt + d_pt, get_pos());
}

Vector2 VGEditor::ControlPointIterator::get_pos() const {
	int j = pt + d_pt;
	if (j < 0 && closed) {
		j += n_pts;
	}
	ERR_FAIL_INDEX_V(j, n_pts, Vector2(0, 0));
	const float *points = subpath_ref->getPoints();
	const int i = 2 * j;
	return Vector2(points[i + 0], points[i + 1]);
}

Vector2 VGEditor::ControlPointIterator::get_knot_pos() const {
	ERR_FAIL_INDEX_V(pt, n_pts, Vector2(0, 0));
	const float *points = subpath_ref->getPoints();
	const int i = 2 * pt;
	return Vector2(points[i + 0], points[i + 1]);
}

void VGEditor::forward_draw_over_viewport(Control *p_overlay) {
	if (!node_vg)
		return;

	Control *vpc = canvas_item_editor->get_viewport_control();
	Transform2D xform = canvas_item_editor->get_canvas_transform() *
		_get_node()->get_global_transform();
	const Ref<Texture> handle = get_icon("EditorHandle", "EditorIcons");

	const tove::PathRef path = node_vg->get_tove_path();

	_update_overlay();
	if (overlay.is_valid()) {
		vpc->draw_set_transform_matrix(overlay_draw_xform);
		vpc->draw_mesh(overlay, Ref<Texture>(), Ref<Texture>());
		vpc->draw_set_transform_matrix(Transform2D());
	}

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

void VGEditor::edit(Node *p_node_vg) {

	if (!canvas_item_editor) {
		canvas_item_editor = CanvasItemEditor::get_singleton();
	}

	if (node_vg) {
		node_vg->remove_change_receptor(this);
	}

	if (p_node_vg) {

		_set_node(p_node_vg);

		//Enable the pencil tool if the polygon is empty
		if (_is_empty())
			_menu_option(MODE_CREATE);

		edited_point = PosVertex();
		edited_path = SubpathId();
		hover_point = Vertex();
		selected_point = Vertex();

		_update_overlay(true);

	} else {

		_set_node(NULL);
		overlay = Ref<Mesh>();
		_update_overlay(true);
	}

	if (node_vg) {
		node_vg->add_change_receptor(this);
	}

	canvas_item_editor->get_viewport_control()->update();
}

void VGEditor::_bind_methods() {

	ClassDB::bind_method(D_METHOD("_node_removed"), &VGEditor::_node_removed);
	ClassDB::bind_method(D_METHOD("_menu_option"), &VGEditor::_menu_option);
	ClassDB::bind_method(D_METHOD("_create_mesh_node"), &VGEditor::_create_mesh_node);
	ClassDB::bind_method(D_METHOD("_create_resource"), &VGEditor::_create_resource);
}

void VGEditor::remove_point(const Vertex &p_vertex) {

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

	if (_is_empty())
		_menu_option(MODE_CREATE);

	hover_point = Vertex();
	if (selected_point == p_vertex)
		selected_point = Vertex();
}

VGEditor::Vertex VGEditor::get_active_point() const {

	return hover_point.valid() ? hover_point : selected_point;
}

static Vector2 basis_xform_inv(const Transform2D &xform, const Vector2 &v) {
	return xform.affine_inverse().basis_xform(v);
}

static Vector2 xform_inv(const Transform2D &xform, const Vector2 &p) {
	return xform.affine_inverse().xform(p);
}

VGEditor::PosVertex VGEditor::closest_point(const Vector2 &p_pos, bool p_cp) const {

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

VGEditor::SubpathPos VGEditor::closest_subpath_point(const Vector2 &p_pos) const {

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
		_get_node()->get_global_transform();

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
				tove::DefaultCurveFlattener(200.0f, 6)));
	    tove::MeshRef tove_mesh = tove::tove_make_shared<tove::ColorMesh>(); 
		tesselator->graphicsToMesh(overlay_graphics.get(), UPDATE_MESH_EVERYTHING, tove_mesh, tove_mesh);
		copy_mesh(overlay, tove_mesh);
	}
	overlay_full_xform = xform;
}

void VGEditor::_changed_callback(Object *p_changed, const char *p_prop) {
	if (node_vg && node_vg == p_changed) {
		_update_overlay(true);
	}	
}

VGEditor::VGEditor(EditorNode *p_editor) {

	/*add_child(memnew(VSeparator));
	button_create = memnew(ToolButton);
	add_child(button_create);
	button_create->connect("pressed", this, "_toggle_edit");
	button_create->set_toggle_mode(true);
	button_create->set_tooltip(TTR("Toggle vector graphics editing"));*/



    node_vg = NULL;

	canvas_item_editor = NULL;
	editor = p_editor;
	undo_redo = editor->get_undo_redo();

	edited_point = PosVertex();
	edited_path = SubpathId();

	hover_point = Vertex();
	selected_point = Vertex();

	mb_down_time = 0;

	add_child(memnew(VSeparator));
	button_create = memnew(ToolButton);
	add_child(button_create);
	button_create->connect("pressed", this, "_menu_option", varray(MODE_CREATE));
	button_create->set_toggle_mode(true);
	button_create->set_tooltip(TTR("Create a new polygon from scratch"));

	button_edit = memnew(ToolButton);
	add_child(button_edit);
	button_edit->connect("pressed", this, "_menu_option", varray(MODE_EDIT));
	button_edit->set_toggle_mode(true);
	button_edit->set_tooltip(TTR("Edit existing polygon:\nLMB: Move Point.\nCtrl+LMB: Split Segment.\nRMB: Erase Point."));

	button_delete = memnew(ToolButton);
	add_child(button_delete);
	button_delete->connect("pressed", this, "_menu_option", varray(MODE_DELETE));
	button_delete->set_toggle_mode(true);
	button_delete->set_tooltip(TTR("Delete points"));

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

	mode = MODE_EDIT;
}
