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

enum ToveDisplay {
    TOVE_DISPLAY_TEXTURE,
    TOVE_DISPLAY_MESH
};

VARIANT_ENUM_CAST(ToveDisplay);

#endif // TOVEGD_TOVE2D_H
