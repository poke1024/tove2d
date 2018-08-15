/*************************************************************************/
/*  vector_graphics.cpp                                                  */
/*************************************************************************/

#include "vector_graphics.h"
#include "os/file_access.h"
#include "scene/resources/primitive_meshes.h"

VectorGraphics::VectorGraphics() {
    backend = TOVE_BACKEND_MESH;
	quality = 0.5f;
    mesh = Ref<ArrayMesh>(memnew(ArrayMesh));
	tove_graphics = std::make_shared<tove::Graphics>();
	dirty = false;
}

VectorGraphics::~VectorGraphics() {
}

Error VectorGraphics::load(const String &p_path, const String &p_units, float p_dpi) {

	Vector<uint8_t> buf = FileAccess::get_file_as_array(p_path);

	String str;
	str.parse_utf8((const char *)buf.ptr(), buf.size());

	tove_graphics = tove::Graphics::createFromSVG(
		str.utf8().ptr(), p_units.utf8().ptr(), p_dpi);

	const float *bounds = tove_graphics->getExactBounds();

	float s = 1024.0f / MAX(bounds[2] - bounds[0], bounds[3] - bounds[1]);
	if (s > 1.0f) {
		tove::nsvg::Transform transform(s, 0, 0, 0, s, 0);
		transform.setWantsScaleLineWidth(true);
		tove_graphics->set(tove_graphics, transform);
	}

	set_dirty();
	//_update_paths();

	return OK;
}

void VectorGraphics::load_default() {
	tove_graphics = std::make_shared<tove::Graphics>();

	tove::SubpathRef subpath = tove_graphics->beginSubpath();
	subpath->drawEllipse(0, 0, 100, 100);

	tove_graphics->setFillColor(std::make_shared<tove::Color>(0.8, 0.1, 0.1));
	tove_graphics->fill();

	tove_graphics->setLineColor(std::make_shared<tove::Color>(0, 0, 0));
	tove_graphics->stroke();

	set_dirty();
	//_update_paths();
}

void VectorGraphics::update_mesh() {

	if (!dirty) {
		return;
	}
	dirty = false;

    if (backend == TOVE_BACKEND_MESH) {
		tove_graphics_to_mesh(mesh, tove_graphics, quality);
		texture = Ref<ImageTexture>();
    } else if (backend == TOVE_BACKEND_TEXTURE) {
		tove_graphics_to_texture(mesh, tove_graphics, quality);

		const float resolution = quality;
		const float *bounds = tove_graphics->getExactBounds();
		const float w = bounds[2] - bounds[0];
		const float h = bounds[3] - bounds[1];

		texture.instance();
		texture->create_from_image(rasterize(
			w * resolution, h * resolution,
			-bounds[0] * resolution, -bounds[1] * resolution,
			resolution), ImageTexture::FLAG_FILTER);
	}
}

Ref<Image> VectorGraphics::rasterize(int p_width, int p_height, float p_tx, float p_ty, float p_scale) const {

	ERR_FAIL_COND_V(p_width <= 0, Ref<Image>());
	ERR_FAIL_COND_V(p_height <= 0, Ref<Image>());

	const int w = p_width;
	const int h = p_height;

	PoolVector<uint8_t> dst_image;
	dst_image.resize(w * h * 4);

    {
        PoolVector<uint8_t>::Write dw = dst_image.write();
		tove_graphics->rasterize(&dw[0], p_width, p_height, w * 4, p_tx, p_ty, p_scale, nullptr);
    }

	Ref<Image> image;
	image.instance();
	image->create(w, h, false, Image::FORMAT_RGBA8, dst_image);

	return image;
}

void VectorGraphics::_bind_methods() {

	ClassDB::bind_method(D_METHOD("load", "path", "units", "dpi"), &VectorGraphics::load, DEFVAL("px"), DEFVAL(96.0f));
	ClassDB::bind_method(D_METHOD("rasterize", "tx", "ty", "scale"), &VectorGraphics::rasterize, DEFVAL(0.0f), DEFVAL(0.0f), DEFVAL(1.0f));

    ClassDB::bind_method(D_METHOD("set_backend", "backend"), &VectorGraphics::set_backend, DEFVAL(TOVE_BACKEND_MESH));
	ClassDB::bind_method(D_METHOD("get_backend"), &VectorGraphics::get_backend);

	ClassDB::bind_method(D_METHOD("set_quality", "quality"), &VectorGraphics::set_quality, DEFVAL(1.0f));
	ClassDB::bind_method(D_METHOD("get_quality"), &VectorGraphics::get_quality);

	ADD_PROPERTY(PropertyInfo(Variant::INT, "backend", PROPERTY_HINT_ENUM, "Texture,Mesh"), "set_backend", "get_backend");
    ADD_PROPERTY(PropertyInfo(Variant::REAL, "quality"), "set_quality", "get_quality");

	ClassDB::bind_method(D_METHOD("insert_curve", "path", "subpath", "t"), &VectorGraphics::insert_curve);
	ClassDB::bind_method(D_METHOD("remove_curve", "path", "subpath", "curve"), &VectorGraphics::remove_curve);
	ClassDB::bind_method(D_METHOD("set_points", "path", "subpath", "points"), &VectorGraphics::set_points);

	/*ClassDB::bind_method(D_METHOD("set_paths", "paths"), &VectorGraphics::set_paths);
	ClassDB::bind_method(D_METHOD("get_paths"), &VectorGraphics::get_paths);

	ADD_PROPERTY(PropertyInfo(Variant::ARRAY, "paths"), "set_paths", "get_paths");*/
}

void VectorGraphics::set_backend(ToveBackend p_backend) {
	backend = p_backend;
	set_dirty();
}

ToveBackend VectorGraphics::get_backend() {
	return backend;
}

void VectorGraphics::set_quality(float p_quality) {
	quality = p_quality;
	set_dirty();
}

float VectorGraphics::get_quality() {
	return quality;
}

tove::GraphicsRef VectorGraphics::get_tove_graphics() const {
	return tove_graphics;
}

MeshInstance2D *VectorGraphics::create_mesh_node() {
	update_mesh();
	MeshInstance2D *mesh_inst = memnew(MeshInstance2D);
	mesh_inst->set_mesh(mesh);
	mesh_inst->set_texture(texture);
	return mesh_inst;
}

void VectorGraphics::set_points(int p_path, int p_subpath, Array p_points) {
	ERR_FAIL_INDEX(p_path, tove_graphics->getNumPaths());
	tove::PathRef path = tove_graphics->getPath(p_path);
	ERR_FAIL_INDEX(p_subpath, path->getNumSubpaths());
	tove::SubpathRef subpath = path->getSubpath(p_subpath);
	const int n = p_points.size();
	float *p = new float[2 * n];
	for (int i = 0; i < n; i++) {
		const Vector2 q = p_points[i];
		p[2 * i + 0] = q.x;
		p[2 * i + 1] = q.y;
	}
	try {
		subpath->setPoints(p, n, false);
		set_dirty();
	} catch(...) {
		delete[] p;
		throw;	
	}
	delete[] p;
}

void VectorGraphics::insert_curve(int p_path, int p_subpath, float p_t) {
	ERR_FAIL_INDEX(p_path, tove_graphics->getNumPaths());
	tove::PathRef path = tove_graphics->getPath(p_path);
	ERR_FAIL_INDEX(p_subpath, path->getNumSubpaths());
	tove::SubpathRef subpath = path->getSubpath(p_subpath);
	subpath->insertCurveAt(p_t);
	set_dirty();
}

void VectorGraphics::remove_curve(int p_path, int p_subpath, int p_curve) {
	ERR_FAIL_INDEX(p_path, tove_graphics->getNumPaths());
	tove::PathRef path = tove_graphics->getPath(p_path);
	ERR_FAIL_INDEX(p_subpath, path->getNumSubpaths());
	tove::SubpathRef subpath = path->getSubpath(p_subpath);
	subpath->removeCurve(p_curve);
	set_dirty();
}

void VectorGraphics::set_dirty() {
	dirty = true;
	_change_notify("curves");
}

bool VectorGraphics::_set(const StringName &p_name, const Variant &p_value) {
	String name = p_name;
	dirty = true;

	if (name == "backend") {
		backend = static_cast<ToveBackend>((int)p_value);
		/*if (p_value == "mesh") {
			backend = TOVE_BACKEND_MESH;
		} else if (p_value == "texture") {
			backend = TOVE_BACKEND_TEXTURE;
		} else {
			return false;
		}*/
	} else if (name == "quality") {
		quality = p_value;
	} else if (name.begins_with("paths/")) {
		int path = name.get_slicec('/', 1).to_int();
		String what = name.get_slicec('/', 2);

		if (tove_graphics->getNumPaths() == path) {
			tove::PathRef tove_path = std::make_shared<tove::Path>();
			tove_graphics->addPath(tove_path);
		}

		ERR_FAIL_INDEX_V(path, tove_graphics->getNumPaths(), false);
		tove::PathRef tove_path = tove_graphics->getPath(path);

		if (what == "name") {
			String s = p_value;
			CharString t = s.utf8();
			tove_path->setName(t.get_data());
		} else if (what == "line_width") {
			tove_path->setLineWidth(p_value);
		} else if (what == "fill_color") {
			if (p_value.get_type() == Variant::COLOR) {
				Color c = p_value;
				tove_path->setFillColor(std::make_shared<tove::Color>(c.r, c.g, c.b, c.a));
			}
		} else if (what == "line_color") {
			if (p_value.get_type() == Variant::COLOR) {
				Color c = p_value;
				tove_path->setLineColor(std::make_shared<tove::Color>(c.r, c.g, c.b, c.a));
			}
		} else if (what == "fill_rule") {
			if (p_value == "nonzero") {
				tove_path->setFillRule(TOVE_FILLRULE_NON_ZERO);
			} else if (p_value == "evenodd") {
				tove_path->setFillRule(TOVE_FILLRULE_EVEN_ODD);
			} else {
				return false;
			}
		} else if (what == "subpaths") {
			int subpath = name.get_slicec('/', 3).to_int();
			String subwhat = name.get_slicec('/', 4);

			if (tove_path->getNumSubpaths() == subpath) {
				tove::SubpathRef tove_subpath = std::make_shared<tove::Subpath>();
				tove_path->addSubpath(tove_subpath);
			}

			ERR_FAIL_INDEX_V(subpath, tove_path->getNumSubpaths(), false);
			tove::SubpathRef tove_subpath = tove_path->getSubpath(subpath);
			
			if (subwhat == "closed") {
				tove_subpath->setIsClosed(p_value);
			} if (subwhat == "points") {
				PoolVector2Array pts = p_value;
				const int n = pts.size();
				float *buf = new float[n * 2];
				for (int i = 0; i < n; i++) {
					const Vector2 &p = pts[i];
					buf[2 * i + 0] = p.x;
					buf[2 * i + 1] = p.y;
				}
				tove_subpath->setPoints(buf, n, false);
				delete[] buf;
			} else {
				return false;
			}
		} else {
			return true;
		}
	} else {
		return false;
	}

	return true;	
}

bool VectorGraphics::_get(const StringName &p_name, Variant &r_ret) const {
	String name = p_name;

	if (name == "backend") {
		switch (backend) {
			r_ret = (int)backend;
			/*case TOVE_BACKEND_MESH: {
				r_ret = "mesh";
			} break;
			case TOVE_BACKEND_TEXTURE: {
				r_ret = "texture";
			} break;
			default: {
				return false;
			} break;*/
		}
	} else if (name == "quality") {
		r_ret = quality;
	} else if (name.begins_with("paths/")) {
		int path = name.get_slicec('/', 1).to_int();
		String what = name.get_slicec('/', 2);
		ERR_FAIL_INDEX_V(path, tove_graphics->getNumPaths(), false);
		tove::PathRef tove_path = tove_graphics->getPath(path);

		if (what == "name") {
			r_ret = String::utf8(tove_path->getName());
		} else if (what == "line_width") {
			r_ret = tove_path->getLineWidth();
		} else if (what == "fill_color") {
			tove::PaintRef paint = tove_path->getFillColor();
			if (paint.get()) {
				ToveRGBA rgba;
				paint->getColorStop(0, rgba, 1);
				r_ret = Color(rgba.r, rgba.b, rgba.b, rgba.a);
			} else {
				r_ret = Variant();
			}
		} else if (what == "line_color") {
			tove::PaintRef paint = tove_path->getLineColor();
			if (paint.get()) {
				ToveRGBA rgba;
				paint->getColorStop(0, rgba, 1);
				r_ret = Color(rgba.r, rgba.b, rgba.b, rgba.a);
			} else {
				r_ret = Variant();
			}
		} else if (what == "fill_rule") {
			switch (tove_path->getFillRule()) {
				case TOVE_FILLRULE_NON_ZERO: {
					r_ret = "nonzero";
				} break;
				case TOVE_FILLRULE_EVEN_ODD: {
					r_ret = "evenodd";
				} break;
				default: {
					return false;
				} break;
			}
		} else if (what.begins_with("subpaths")) {
			int subpath = name.get_slicec('/', 3).to_int();
			String subwhat = name.get_slicec('/', 4);
			ERR_FAIL_INDEX_V(subpath, tove_path->getNumSubpaths(), false);
			tove::SubpathRef tove_subpath = tove_path->getSubpath(subpath);
			if (subwhat == "closed") {
				r_ret = tove_subpath->isClosed();
			} else if (subwhat == "points") {
				const int n = tove_subpath->getNumPoints();
				PoolVector2Array out;
				out.resize(n);
				{
					PoolVector2Array::Write w = out.write();
					const float *pts = tove_subpath->getPoints();
					for (int i = 0; i < n; i++) {
						w[i] = Vector2(pts[2 * i + 0], pts[2 * i + 1]);
					}
				}
				r_ret = out;
			} else {
				return false;
			}
		} else {
			return false;
		}
	} else {
		return false;
	}

	return true;
}

void VectorGraphics::_get_property_list(List<PropertyInfo> *p_list) const {

	//p_list->push_back(PropertyInfo(Variant::STRING, "backend", PROPERTY_HINT_NONE, "", PROPERTY_USAGE_NOEDITOR | PROPERTY_USAGE_INTERNAL));
	//p_list->push_back(PropertyInfo(Variant::REAL, "quality", PROPERTY_HINT_NONE, "", PROPERTY_USAGE_NOEDITOR | PROPERTY_USAGE_INTERNAL));

	for (int i = 0; i < tove_graphics->getNumPaths(); i++) {
		const tove::PathRef tove_path = tove_graphics->getPath(i);
		const String path_prefix = "paths/" + itos(i);

		p_list->push_back(PropertyInfo(Variant::STRING, path_prefix + "/name", PROPERTY_HINT_NONE, "", PROPERTY_USAGE_NOEDITOR | PROPERTY_USAGE_INTERNAL));
		p_list->push_back(PropertyInfo(Variant::REAL, path_prefix + "/line_width", PROPERTY_HINT_NONE, "", PROPERTY_USAGE_NOEDITOR | PROPERTY_USAGE_INTERNAL));
		p_list->push_back(PropertyInfo(Variant::COLOR, path_prefix + "/fill_color", PROPERTY_HINT_NONE, "", PROPERTY_USAGE_NOEDITOR | PROPERTY_USAGE_INTERNAL));
		p_list->push_back(PropertyInfo(Variant::COLOR, path_prefix + "/line_color", PROPERTY_HINT_NONE, "", PROPERTY_USAGE_NOEDITOR | PROPERTY_USAGE_INTERNAL));
		p_list->push_back(PropertyInfo(Variant::INT, path_prefix + "/fill_rule", PROPERTY_HINT_NONE, "", PROPERTY_USAGE_NOEDITOR | PROPERTY_USAGE_INTERNAL));

		for (int j = 0; j < tove_path->getNumSubpaths(); j++) {
			const String subpath_prefix = path_prefix + "/subpaths/" + itos(j);

			p_list->push_back(PropertyInfo(Variant::BOOL, subpath_prefix + "/closed", PROPERTY_HINT_NONE, "", PROPERTY_USAGE_NOEDITOR | PROPERTY_USAGE_INTERNAL));			
			p_list->push_back(PropertyInfo(Variant::POOL_VECTOR2_ARRAY, subpath_prefix + "/points", PROPERTY_HINT_NONE, "", PROPERTY_USAGE_NOEDITOR | PROPERTY_USAGE_INTERNAL));			
		}
	}
}
