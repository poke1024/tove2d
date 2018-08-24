/*************************************************************************/
/*  tove2d.h     	                                                     */
/*************************************************************************/

#ifndef TOVEGD_TOVE2D_H
#define TOVEGD_TOVE2D_H

#include <stdint.h>
#include "core/class_db.h"
#include "scene/2d/mesh_instance_2d.h"

#include "../src/cpp/graphics.h"
#include "../src/cpp/path.h"
#include "../src/cpp/subpath.h"
#include "../src/cpp/paint.h"

enum ToveBackend {
	TOVE_BACKEND_INHERIT,
    TOVE_BACKEND_TEXTURE,
    TOVE_BACKEND_MESH
};

VARIANT_ENUM_CAST(ToveBackend);

struct VGMeshData {
	Ref<ArrayMesh> mesh;
	Ref<ImageTexture> texture;
};

class ToveGraphicsBackend {
	const tove::GraphicsRef &graphics;
	const float quality;

public:
	inline ToveGraphicsBackend(
		const tove::GraphicsRef &p_graphics,
		float p_quality) :
		graphics(p_graphics),
		quality(p_quality) {
	}

	void mesh(Ref<ArrayMesh> &mesh);
	void texture(Ref<ArrayMesh> &mesh);

	VGMeshData create_mesh_data(ToveBackend p_backend);
	Ref<ImageTexture> create_texture();
};

class TovePathBackend : public ToveGraphicsBackend {
public:
	TovePathBackend(
		const tove::PathRef &p_path,
		const tove::ClipSetRef &p_clip_set,
		float p_quality);
};

Ref<Image> tove_graphics_rasterize(
	const tove::GraphicsRef &tove_graphics,
	int p_width, int p_height,
	float p_tx, float p_ty,
	float p_scale);

#endif // TOVEGD_TOVE2D_H
