/*************************************************************************/
/*  tove2d.cpp   	                                                     */
/*************************************************************************/

#include "tove2d.h"
#include "../src/cpp/mesh/meshifier.h"

tove::PathRef new_transformed_path(const tove::PathRef &p_tove_path, const Transform2D &p_transform) {
    const Vector2 &tx = p_transform.elements[0];
    const Vector2 &ty = p_transform.elements[1];
    const Vector2 &to = p_transform.elements[2];
    tove::nsvg::Transform tove_transform(tx.x, ty.x, to.x, tx.y, ty.y, to.y);
    tove::PathRef tove_path = tove::tove_make_shared<tove::Path>();
    tove_path->set(p_tove_path, tove_transform);
    return tove_path;
}

void copy_mesh(Ref<ArrayMesh> &p_mesh, tove::MeshRef &p_tove_mesh) {
    const int n = p_tove_mesh->getVertexCount();
    if (n < 1) {
        return;
    }

    int stride = sizeof(float) * 2 + 4;
    int size = n * stride;
    uint8_t *vertices = new uint8_t[size];
    p_tove_mesh->copyVertexData(vertices, size);

    const int indexCount = p_tove_mesh->getIndexCount();
    ToveVertexIndex *indices = new ToveVertexIndex[indexCount];
    p_tove_mesh->copyIndexData(indices, indexCount);

    PoolVector<int> iarr;
    iarr.resize(indexCount);
    {
        PoolVector<int>::Write w = iarr.write();
        for (int i = 0; i < indexCount; i++) {
            w[i] = indices[i];
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
    delete[] indices;

    Array arr;
    arr.resize(Mesh::ARRAY_MAX);
    arr[Mesh::ARRAY_VERTEX] = varr;
    arr[Mesh::ARRAY_INDEX] = iarr;
    arr[Mesh::ARRAY_COLOR] = carr;

    p_mesh->add_surface_from_arrays(Mesh::PRIMITIVE_TRIANGLES, arr);
}
