/*
* TÖVE - Animated vector graphics for LÖVE.
 * https://github.com/poke1024/tove2d
 *
 * Copyright (c) 2018, Bernhard Liebl
 *
 * Distributed under the MIT license. See LICENSE file for details.
 *
 * All rights reserved.
 */

#ifndef __TOVE_WARN
#define __TOVE_WARN 1

#include "interface.h"

extern ToveWarningFunction tove_warn_func;

void tove_warn(const char *file, int line, const char *s);
#define TOVE_WARN(s) tove_warn(__FILE__, __LINE__, s)
#define TOVE_BAD_ALLOC() { TOVE_WARN("Out of memory"); throw std::bad_alloc(); }

#endif // __TOVE_WARN
