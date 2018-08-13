/*************************************************************************/
/*  vector_graphics.cpp                                                  */
/*************************************************************************/

#include "vector_graphics.h"
#include "os/file_access.h"
#include "scene/resources/primitive_meshes.h"

VectorGraphics::VectorGraphics() {
    display = TOVE_DISPLAY_MESH;
	quality = 0.5f;
    mesh = Ref<ArrayMesh>(memnew(ArrayMesh));
}

VectorGraphics::~VectorGraphics() {
}

Error VectorGraphics::load(const String &p_path, const String &p_units, float p_dpi) {

	Vector<uint8_t> buf = FileAccess::get_file_as_array(p_path);

	String str;
	str.parse_utf8((const char *)buf.ptr(), buf.size());

	graphics = tove::Graphics::createFromSVG(
		str.utf8().ptr(), p_units.utf8().ptr(), p_dpi);

	const float *bounds = graphics->getExactBounds();

	float s = 1024.0f / MAX(bounds[2] - bounds[0], bounds[3] - bounds[1]);
	if (s > 1.0f) {
		tove::nsvg::Transform transform(s, 0, 0, 0, s, 0);
		transform.setWantsScaleLineWidth(true);
		graphics->set(graphics, transform);
	}

	return OK;
}

void VectorGraphics::load_default() {
	graphics = std::make_shared<tove::Graphics>();

	tove::SubpathRef subpath = graphics->beginSubpath();
	subpath->drawEllipse(0, 0, 100, 100);

	graphics->setFillColor(std::make_shared<tove::Color>(0.8, 0.1, 0.1));
	graphics->fill();

	graphics->setLineColor(std::make_shared<tove::Color>(0, 0, 0));
	graphics->stroke();
}

void VectorGraphics::update_mesh() {

    ERR_FAIL_COND(!graphics.get());

    if (display == TOVE_DISPLAY_MESH) {
		graphics_to_mesh(mesh, graphics, quality);
		texture = Ref<ImageTexture>();
    } else if (display == TOVE_DISPLAY_TEXTURE) {
		graphics_to_texture(mesh, graphics, quality);

		const float resolution = quality;
		const float *bounds = graphics->getExactBounds();
		const float w = bounds[2] - bounds[0];
		const float h = bounds[3] - bounds[1];

		texture.instance();
		texture->create_from_image(rasterize(
			w * resolution, h * resolution,
			-bounds[0] * resolution, -bounds[1] * resolution,
			resolution), ImageTexture::FLAG_FILTER);
	}
}

void VectorGraphics::update_instance(MeshInstance2D *p_instance) {

    ERR_FAIL_COND(!graphics.get());

	update_mesh();

	p_instance->set_mesh(mesh);
	p_instance->set_texture(texture);
}

Ref<Image> VectorGraphics::rasterize(int p_width, int p_height, float p_tx, float p_ty, float p_scale) const {

	ERR_FAIL_COND_V(!graphics.get(), Ref<Image>());

	ERR_FAIL_COND_V(p_width <= 0, Ref<Image>());
	ERR_FAIL_COND_V(p_height <= 0, Ref<Image>());

	const int w = p_width;
	const int h = p_height;

	PoolVector<uint8_t> dst_image;
	dst_image.resize(w * h * 4);

    {
        PoolVector<uint8_t>::Write dw = dst_image.write();
		graphics->rasterize(&dw[0], p_width, p_height, w * 4, p_tx, p_ty, p_scale, nullptr);
    }

	Ref<Image> image;
	image.instance();
	image->create(w, h, false, Image::FORMAT_RGBA8, dst_image);

	return image;
}

void VectorGraphics::_bind_methods() {

	ClassDB::bind_method(D_METHOD("load", "path", "units", "dpi"), &VectorGraphics::load, DEFVAL("px"), DEFVAL(96.0f));
	ClassDB::bind_method(D_METHOD("rasterize", "tx", "ty", "scale"), &VectorGraphics::rasterize, DEFVAL(0.0f), DEFVAL(0.0f), DEFVAL(1.0f));

    ClassDB::bind_method(D_METHOD("set_display", "display"), &VectorGraphics::set_display, DEFVAL(TOVE_DISPLAY_MESH));
	ClassDB::bind_method(D_METHOD("get_display"), &VectorGraphics::get_display);

	ClassDB::bind_method(D_METHOD("set_quality", "quality"), &VectorGraphics::set_quality, DEFVAL(1.0f));
	ClassDB::bind_method(D_METHOD("get_quality"), &VectorGraphics::get_quality);

	ADD_PROPERTY(PropertyInfo(Variant::INT, "display", PROPERTY_HINT_ENUM, "Texture,Mesh"), "set_display", "get_display");
    ADD_PROPERTY(PropertyInfo(Variant::REAL, "quality"), "set_quality", "get_quality");

	ClassDB::bind_method(D_METHOD("insert_curve", "path", "subpath", "t"), &VectorGraphics::insert_curve);
	ClassDB::bind_method(D_METHOD("remove_curve", "path", "subpath", "curve"), &VectorGraphics::remove_curve);
	ClassDB::bind_method(D_METHOD("set_points", "path", "subpath", "points"), &VectorGraphics::set_points);
}

void VectorGraphics::set_display(ToveDisplay p_display) {
	display = p_display;
}

ToveDisplay VectorGraphics::get_display() {
	return display;
}

void VectorGraphics::set_quality(float quality) {
	this->quality = quality;
}

float VectorGraphics::get_quality() {
	return quality;
}

const tove::GraphicsRef &VectorGraphics::get_tove_graphics() const {
	return graphics;
}

void VectorGraphics::set_points(int p_path, int p_subpath, Array p_points) {
	ERR_FAIL_INDEX(p_path, graphics->getNumPaths());
	tove::PathRef path = graphics->getPath(p_path);
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
	} catch(...) {
		delete[] p;
		throw;	
	}
	delete[] p;
}

void VectorGraphics::insert_curve(int p_path, int p_subpath, float p_t) {
	ERR_FAIL_INDEX(p_path, graphics->getNumPaths());
	tove::PathRef path = graphics->getPath(p_path);
	ERR_FAIL_INDEX(p_subpath, path->getNumSubpaths());
	tove::SubpathRef subpath = path->getSubpath(p_subpath);
	subpath->insertCurveAt(p_t);
}

void VectorGraphics::remove_curve(int p_path, int p_subpath, int p_curve) {
	ERR_FAIL_INDEX(p_path, graphics->getNumPaths());
	tove::PathRef path = graphics->getPath(p_path);
	ERR_FAIL_INDEX(p_subpath, path->getNumSubpaths());
	tove::SubpathRef subpath = path->getSubpath(p_subpath);
	subpath->removeCurve(p_curve);
}
