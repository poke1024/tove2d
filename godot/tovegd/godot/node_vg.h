/*************************************************************************/
/*  node_vg.h 			                                                 */
/*************************************************************************/

#ifndef TOVEGD_INSTANCE_H
#define TOVEGD_INSTANCE_H

#include "tove2d.h"
#include "scene/2d/mesh_instance_2d.h"
#include "vector_graphics.h"

class NodeVG : public MeshInstance2D {
	GDCLASS(NodeVG, MeshInstance2D);

    Ref<Resource> svg;
	Ref<VectorGraphics> graphics;

protected:
	static void _bind_methods();

public:
	virtual bool _edit_is_selected_on_click(const Point2 &p_point, double p_tolerance) const;

	const tove::GraphicsRef &get_tove_graphics() const;
	const Ref<VectorGraphics> &get_vector_graphics() const;

	void update_mesh();

	Ref<Resource> get_svg() const;
	void set_svg(const Ref<Resource> &p_svg);

    void set_display(ToveDisplay p_display);
	ToveDisplay get_display();

	float get_quality();
	void set_quality(float quality);

	void set_points(int p_path, int p_subpath, Array p_points);
	void insert_curve(int p_path, int p_subpath, float p_t);
	void remove_curve(int p_path, int p_subpath, int p_curve);

	NodeVG();
};

#endif // TOVEGD_INSTANCE_H
