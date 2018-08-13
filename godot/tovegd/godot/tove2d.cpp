/*************************************************************************/
/*  tove2d.cpp   	                                                     */
/*************************************************************************/

#include "tove2d.h"

void graphics_to_mesh(Ref<ArrayMesh> &mesh, const tove::GraphicsRef &graphics, float quality) {

    ToveTesselationQuality t_quality;
    t_quality.stopCriterion = TOVE_MAX_ERROR;
    t_quality.recursionLimit = 8;
    t_quality.maximumError = 1.0 / (quality * 2);
    t_quality.arcTolerance = 1.0 / (quality * 2);

    tove::MeshRef tove_mesh = std::make_shared<tove::ColorMesh>();
    graphics->tesselate(tove_mesh, 1024, t_quality, HOLES_CW, 0);

    const int n = tove_mesh->getVertexCount();
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
            w[i] = triangles.array[i] - 1; // 1-based
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

    while (mesh->get_surface_count() > 0) {
        mesh->surface_remove(0);
    }

    mesh->add_surface_from_arrays(Mesh::PRIMITIVE_TRIANGLES, arr);
}

void graphics_to_texture(Ref<ArrayMesh> &mesh, const tove::GraphicsRef &graphics, float quality) {

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

    while (mesh->get_surface_count() > 0) {
        mesh->surface_remove(0);
    }

    mesh->add_surface_from_arrays(Mesh::PRIMITIVE_TRIANGLES, arr);
}

