/*************************************************************************/
/*  vg_color.h 	                                                         */
/*************************************************************************/

#ifndef VG_COLOR_H
#define VG_COLOR_H

#include "vg_paint.h"

class VGColor : public VGPaint {
	GDCLASS(VGColor, VGPaint);

    Color color;

protected:
	static void _bind_methods();

public:
    VGColor() {
    }

    VGColor(const Color &p_color) : color(p_color) {
    }

    void set_color(const Color &p_color);
    Color get_color() const;
};

#endif // VG_COLOR_H
