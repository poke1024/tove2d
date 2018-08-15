/*************************************************************************/
/*  vector_graphics.h 	                                                 */
/*************************************************************************/

#ifndef TOVEGD_GRAPHICS_H
#define TOVEGD_GRAPHICS_H

#include "tove2d.h"
#include "scene/2d/mesh_instance_2d.h"
#include "core/image.h"

class VectorGraphics : public Resource {
	GDCLASS(VectorGraphics, Resource);
	RES_BASE_EXTENSION("vg");

	tove::GraphicsRef tove_graphics;
	Array paths;

	ToveBackend backend;
	float quality;

	Ref<ArrayMesh> mesh;
	Ref<ImageTexture> texture;
	bool dirty;

	void update_mesh();

protected:
	bool _set(const StringName &p_name, const Variant &p_value);
	bool _get(const StringName &p_name, Variant &r_ret) const;
	void _get_property_list(List<PropertyInfo> *p_list) const;

	static void _bind_methods();

public:
	VectorGraphics();
	virtual ~VectorGraphics();

	Error load(const String &p_path, const String &p_units, float p_dpi);
	void load_default();

	inline const Ref<ArrayMesh> &get_mesh() {
		if (dirty) {
			update_mesh();
		}
		return mesh;
	}

	inline const Ref<ImageTexture> &get_texture() {
		if (dirty) {
			update_mesh();
		}
		return texture;
	}

	Ref<Image> rasterize(
        int p_width, int p_height,
        float p_tx, float p_ty, float p_scale) const;

	void set_backend(ToveBackend p_backend);
	ToveBackend get_backend();

	void set_quality(float quality);
	float get_quality();

	tove::GraphicsRef get_tove_graphics() const;
	MeshInstance2D *create_mesh_node();

	void set_points(int p_path, int p_subpath, Array p_points);
	void insert_curve(int p_path, int p_subpath, float p_t);
	void remove_curve(int p_path, int p_subpath, int p_curve);
	void set_dirty();
};

#endif // TOVEGD_GRAPHICS_H
