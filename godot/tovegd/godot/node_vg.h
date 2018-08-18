/*************************************************************************/
/*  node_vg.h 			                                                 */
/*************************************************************************/

#ifndef TOVEGD_INSTANCE_H
#define TOVEGD_INSTANCE_H

#include "tove2d.h"
#include "scene/2d/mesh_instance_2d.h"
#include "vector_graphics.h"

class NodeVG : public Node2D {
	GDCLASS(NodeVG, Node2D);

	Ref<VectorGraphics> vg;

protected:
	void _notification(int p_what);
	static void _bind_methods();

public:
	virtual Rect2 _edit_get_rect() const;
	virtual bool _edit_is_selected_on_click(const Point2 &p_point, double p_tolerance) const;
	virtual void _changed_callback(Object *p_changed, const char *p_prop);

	inline Ref<VectorGraphics> get_vg() const {
		return vg;
	}

	Ref<Resource> get_graphics() const;
	void set_graphics(const Ref<Resource> &p_graphics);

    void set_backend(ToveBackend p_backend);
	ToveBackend get_backend();

	float get_quality();
	void set_quality(float quality);

	void set_points(int p_path, int p_subpath, Array p_points);
	void insert_curve(int p_path, int p_subpath, float p_t);
	void remove_curve(int p_path, int p_subpath, int p_curve);

	NodeVG();
};

#endif // TOVEGD_INSTANCE_H
