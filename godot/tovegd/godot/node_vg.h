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

	void update_mesh();

protected:
	static void _bind_methods();

public:
	const tove::GraphicsRef &get_tove_graphics() const;

	Ref<Resource> get_svg() const;
	void set_svg(const Ref<Resource> &p_svg);

    void set_display(ToveDisplay p_display);
	ToveDisplay get_display();

	float get_quality();
	void set_quality(float quality);

	NodeVG();
};

#endif // TOVEGD_INSTANCE_H
