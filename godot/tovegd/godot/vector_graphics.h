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

	ToveBackend backend;
	float quality;

	struct Path {
		Transform2D transform;
		tove::PathRef tove_path;
		VGMeshData mesh_data;
		bool dirty;
		inline Path() : dirty(false) {}
	};
	Vector<Path> paths;
	tove::ClipSetRef clip_set;

	bool any_path_dirty;
	bool editing;
	Rect2 bounds;
	VGMeshData composite;

	tove::GraphicsRef create_tove_graphics() const;
	void update_mesh_representation();

	inline bool use_composite_mesh_representation() const {
		return editing;
	}

protected:
	bool _set(const StringName &p_name, const Variant &p_value);
	bool _get(const StringName &p_name, Variant &r_ret) const;
	void _get_property_list(List<PropertyInfo> *p_list) const;

	static void _bind_methods();

public:
	VectorGraphics();
	virtual ~VectorGraphics();

	void draw(CanvasItem *p_item);

	Error load(const String &p_path, const String &p_units, float p_dpi);
	void load_default();

	inline const Rect2 &get_bounds() {
		if (any_path_dirty) {
			update_mesh_representation();
		}
		return bounds;
	}

	int find_path_at_point(const Point2 &p_point) const;

	Ref<Image> rasterize(
        int p_width, int p_height,
        float p_tx, float p_ty, float p_scale) const;

	void set_backend(ToveBackend p_backend);
	ToveBackend get_backend();

	void set_quality(float quality);
	float get_quality();

	inline int get_num_paths() const {
		return paths.size();
	}
	inline const tove::PathRef &get_tove_path(int p_path) const {
		return paths[p_path].tove_path;
	}

	MeshInstance2D *create_mesh_node();

	void set_points(int p_path, int p_subpath, Array p_points);
	void insert_curve(int p_path, int p_subpath, float p_t);
	void remove_curve(int p_path, int p_subpath, int p_curve);
	void set_dirty(int p_path = -1);
};

#endif // TOVEGD_GRAPHICS_H
