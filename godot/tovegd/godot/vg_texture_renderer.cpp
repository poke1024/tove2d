/*************************************************************************/
/*  vg_texture_renderer.cpp                                              */
/*************************************************************************/

#include "vg_texture_renderer.h"
#include "vg_path.h"

static Ref<Image> tove_graphics_rasterize(
	const tove::GraphicsRef &p_tove_graphics,
	int p_width, int p_height,
	float p_tx, float p_ty,
	float p_scale) {

	ERR_FAIL_COND_V(p_width <= 0, Ref<Image>());
	ERR_FAIL_COND_V(p_height <= 0, Ref<Image>());

	const int w = p_width;
	const int h = p_height;

	PoolVector<uint8_t> dst_image;
	dst_image.resize(w * h * 4);

	{
		PoolVector<uint8_t>::Write dw = dst_image.write();
		p_tove_graphics->rasterize(&dw[0], p_width, p_height, w * 4, p_tx, p_ty, p_scale, nullptr);
	}

	Ref<Image> image;
	image.instance();
	image->create(w, h, false, Image::FORMAT_RGBA8, dst_image);

	return image;
}

VGTextureRenderer::VGTextureRenderer() : quality(1) {
}

float VGTextureRenderer::get_quality() {
	return quality;
}

void VGTextureRenderer::set_quality(float p_quality) {
	quality = p_quality;
	emit_changed();
}

void VGTextureRenderer::_bind_methods() {
	ClassDB::bind_method(D_METHOD("set_quality", "quality"), &VGTextureRenderer::set_quality);
	ClassDB::bind_method(D_METHOD("get_quality"), &VGTextureRenderer::get_quality);

	ADD_PROPERTY(PropertyInfo(Variant::REAL, "quality", PROPERTY_HINT_RANGE, "0,1,0.01"), "set_quality", "get_quality");
}

Rect2 VGTextureRenderer::render_mesh(Ref<ArrayMesh> &p_mesh, VGPath *p_path) {

    // const float resolution = quality;
    tove::GraphicsRef graphics = p_path->get_subtree_graphics();

    const float *bounds = graphics->getExactBounds();
    const float w = bounds[2] - bounds[0];
    const float h = bounds[3] - bounds[1];

    PoolVector<Vector3> faces;
	PoolVector<Vector3> normals;
	PoolVector<float> tangents;
	PoolVector<Vector2> uvs;

	faces.resize(6);
	normals.resize(6);
	tangents.resize(6 * 4);
	uvs.resize(6);

    Vector2 position = Vector2(bounds[0], bounds[1]);
    Vector2 size = Size2(w, h);

	Vector3 quad_faces[4] = {
		Vector3(position.x, position.y, 0),
		Vector3(position.x, position.y + size.y, 0),
		Vector3(position.x + size.x, position.y + size.y, 0),
		Vector3(position.x + size.x, position.y, 0),
	};

	static const int indices[6] = {
		0, 1, 2,
		0, 2, 3
	};

	for (int i = 0; i < 6; i++) {

		int j = indices[i];
		faces.set(i, quad_faces[j]);
		normals.set(i, Vector3(0, 0, 1));
		tangents.set(i * 4 + 0, 1.0);
		tangents.set(i * 4 + 1, 0.0);
		tangents.set(i * 4 + 2, 0.0);
		tangents.set(i * 4 + 3, 1.0);

		static const Vector2 quad_uv[4] = {
			Vector2(0, 0),
			Vector2(0, 1),
			Vector2(1, 1),
			Vector2(1, 0),
		};

		uvs.set(i, quad_uv[j]);
	}

    Array arr;
    arr.resize(Mesh::ARRAY_MAX);

	arr[VS::ARRAY_VERTEX] = faces;
	arr[VS::ARRAY_NORMAL] = normals;
	arr[VS::ARRAY_TANGENT] = tangents;
	arr[VS::ARRAY_TEX_UV] = uvs;

	clear_mesh(p_mesh);
	p_mesh->add_surface_from_arrays(Mesh::PRIMITIVE_TRIANGLES, arr);

    return tove_bounds_to_rect2(p_path->get_tove_path()->getExactBounds());
}

Ref<ImageTexture> VGTextureRenderer::render_texture(VGPath *p_path) {

    // VGPath *root = p_path->get_root_path();

	Size2 s = p_path->get_global_transform().get_scale();
	//float scale = MAX(s.width, s.height);

    /*tove::GraphicsRef root_graphics = root->get_subtree_graphics();
	const float *root_bounds = root_graphics->getExactBounds();
	const float rw = root_bounds[2] - root_bounds[0];
	const float rh = root_bounds[3] - root_bounds[1];
	const float resolution = quality / std::max(rw, rh);*/

	float resolution = quality;
	resolution *= MAX(s.width, s.height);

    tove::GraphicsRef graphics = p_path->get_subtree_graphics();
	const float *bounds = graphics->getExactBounds();
	const float w = bounds[2] - bounds[0];
	const float h = bounds[3] - bounds[1];

	Ref<ImageTexture> texture;
	texture.instance();
	texture->create_from_image(
		tove_graphics_rasterize(
			graphics,
			Math::ceil(w * resolution), Math::ceil(h * resolution),
			-bounds[0] * resolution, -bounds[1] * resolution,
			resolution), ImageTexture::FLAG_FILTER);
	return texture;
}
