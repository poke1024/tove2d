/*************************************************************************/
/*  tove_graphics.h 	                                                 */
/*************************************************************************/

#ifndef TOVEGD_GRAPHICS_H
#define TOVEGD_GRAPHICS_H

#include "tove2d.h"
#include "scene/2d/mesh_instance_2d.h"
#include "core/image.h"

class ToveGraphics : public Resource {
	GDCLASS(ToveGraphics, Resource);

	ToveGraphicsRef graphics;
	ToveDisplay display;
	float quality;

protected:
	static void _bind_methods();

public:
	ToveGraphics();
	virtual ~ToveGraphics();

	Error load(const String &p_path, const String &p_units, float p_dpi);

	void update_mesh(MeshInstance2D *p_instance);

	Ref<Image> rasterize(
        int p_width, int p_height,
        float p_tx, float p_ty, float p_scale) const;

	void set_display(ToveDisplay p_display);
	ToveDisplay get_display();

	void set_quality(float quality);
	float get_quality();
};

#endif // TOVEGD_GRAPHICS_H
