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

inline Rect2 tove_bounds_to_rect2(const float *bounds) {
	return Rect2(bounds[0], bounds[1],  bounds[2] - bounds[0], bounds[3] - bounds[1]);
}

tove::PathRef new_transformed_path(const tove::PathRef &p_tove_path, const Transform2D &p_transform);

void copy_mesh(Ref<ArrayMesh> &p_mesh, tove::MeshRef &p_tove_mesh);

#endif // TOVEGD_TOVE2D_H
