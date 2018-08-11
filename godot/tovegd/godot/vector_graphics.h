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

	tove::GraphicsRef graphics;
	ToveDisplay display;
	float quality;

	void release_graphics();

protected:
	static void _bind_methods();

public:
	VectorGraphics();
	virtual ~VectorGraphics();

	Error load(const String &p_path, const String &p_units, float p_dpi);
	void load_default();

	void update_mesh(MeshInstance2D *p_instance);

	Ref<Image> rasterize(
        int p_width, int p_height,
        float p_tx, float p_ty, float p_scale) const;

	void set_display(ToveDisplay p_display);
	ToveDisplay get_display();

	void set_quality(float quality);
	float get_quality();

	const tove::GraphicsRef &get_tove_graphics() const;
};

#endif // TOVEGD_GRAPHICS_H
