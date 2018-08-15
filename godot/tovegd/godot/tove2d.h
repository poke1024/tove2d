/*************************************************************************/
/*  tove2d.h     	                                                     */
/*************************************************************************/

#ifndef TOVEGD_TOVE2D_H
#define TOVEGD_TOVE2D_H

#include <stdint.h>
#include "core/class_db.h"

/*#define EXPORT
extern "C" {
#include "../src/cpp/_interface.h"
}*/

#include "../src/cpp/graphics.h"
#include "../src/cpp/path.h"
#include "../src/cpp/subpath.h"
#include "../src/cpp/paint.h"

enum ToveBackend {
    TOVE_BACKEND_TEXTURE,
    TOVE_BACKEND_MESH
};

VARIANT_ENUM_CAST(ToveBackend);

#include "scene/2d/mesh_instance_2d.h"

void tove_graphics_to_mesh(Ref<ArrayMesh> &mesh, const tove::GraphicsRef &graphics, float quality);
void tove_graphics_to_texture(Ref<ArrayMesh> &mesh, const tove::GraphicsRef &graphics, float quality);

#endif // TOVEGD_TOVE2D_H
