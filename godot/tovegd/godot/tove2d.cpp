/*************************************************************************/
/*  tove2d.cpp   	                                                     */
/*************************************************************************/

#include "tove2d.h"
#include "../src/cpp/mesh/meshifier.h"

static tove::GraphicsRef path_to_graphics(const tove::PathRef &p_path, const tove::ClipSetRef &p_clip_set) {
	tove::GraphicsRef tove_graphics = tove::tove_make_shared<tove::Graphics>(p_clip_set);
	tove_graphics->addPath(p_path);
	return tove_graphics;
}

TovePathBackend::TovePathBackend(
	const tove::PathRef &p_path,
	const tove::ClipSetRef &p_clip_set,
	float p_quality) : ToveGraphicsBackend(path_to_graphics(p_path, p_clip_set), p_quality) {
}

VGMeshData ToveGraphicsBackend::create_mesh_data(ToveBackend p_backend) {
	VGMeshData composite;

	if (p_backend == TOVE_BACKEND_MESH) {
		mesh(composite.mesh);
		composite.texture = Ref<ImageTexture>();
	} else if (p_backend == TOVE_BACKEND_TEXTURE) {
		texture(composite.mesh);
		composite.texture = create_texture();
	}

	return composite;
}

static void clear_mesh(Ref<ArrayMesh> &p_mesh) {
	if (p_mesh.is_valid()) {
		while (p_mesh->get_surface_count() > 0) {
			p_mesh->surface_remove(0);
		}
	} else {
		p_mesh.instance();
	}
}

void ToveGraphicsBackend::mesh(Ref<ArrayMesh> &p_mesh) {
	clear_mesh(p_mesh);
	
	tove::TesselatorRef tess = tove::tove_make_shared<tove::AdaptiveTesselator>(
		new tove::AdaptiveFlattener<tove::DefaultCurveFlattener>(
			tove::DefaultCurveFlattener(quality, 6)));

	//tove::TesselatorRef tess = tove::tove_make_shared<tove::RigidTesselator>(3, TOVE_HOLES_NONE);
	
    tove::MeshRef tove_mesh = tove::tove_make_shared<tove::ColorMesh>();
	tess->graphicsToMesh(graphics.get(), UPDATE_MESH_EVERYTHING, tove_mesh, tove_mesh);

    const int n = tove_mesh->getVertexCount();
	if (n < 1) {
		return;
	}

    int stride = sizeof(float) * 2 + 4;
    int size = n * stride;
    uint8_t *vertices = new uint8_t[size];
    tove_mesh->copyVertexData(vertices, size);

    ToveTriangles triangles = tove_mesh->getTriangles();

    PoolVector<int> iarr;
    iarr.resize(triangles.size);
    {
        PoolVector<int>::Write w = iarr.write();
        for (int i = 0; i < triangles.size; i++) {
            w[i] = triangles.array[i];
        }
    }

    PoolVector<Vector3> varr;
    varr.resize(n);

    {
        PoolVector<Vector3>::Write w = varr.write();
        for (int i = 0; i < n; i++) {
            float *p = (float*)(vertices + i * stride);
            w[i] = Vector3(p[0], p[1], 0);
        }
    }

    PoolVector<Color> carr;
    carr.resize(n);

    {
        PoolVector<Color>::Write w = carr.write();
        for (int i = 0; i < n; i++) {
            uint8_t *p = vertices + i * stride + 2 * sizeof(float);
            w[i] = Color(p[0] / 255.0, p[1] / 255.0, p[2] / 255.0, p[3] / 255.0);
        }
    }

    delete[] vertices;

    Array arr;
    arr.resize(Mesh::ARRAY_MAX);
    arr[Mesh::ARRAY_VERTEX] = varr;
    arr[Mesh::ARRAY_INDEX] = iarr;
    arr[Mesh::ARRAY_COLOR] = carr;

	p_mesh->add_surface_from_arrays(Mesh::PRIMITIVE_TRIANGLES, arr);
}

void ToveGraphicsBackend::texture(Ref<ArrayMesh> &p_mesh) {

    const float resolution = quality;

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
}

Ref<ImageTexture> ToveGraphicsBackend::create_texture() {
	const float *bounds = graphics->getExactBounds();
	const float w = bounds[2] - bounds[0];
	const float h = bounds[3] - bounds[1];

	const float resolution = quality / std::max(w, h);

	Ref<ImageTexture> texture;
	texture.instance();
	texture->create_from_image(
		tove_graphics_rasterize(
			graphics,
			w * resolution, h * resolution,
			-bounds[0] * resolution, -bounds[1] * resolution,
			resolution), ImageTexture::FLAG_FILTER);
	return texture;
}

Ref<Image> tove_graphics_rasterize(
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
