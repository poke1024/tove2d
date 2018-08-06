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

#include "common.h"
#include "interface.h"

BEGIN_TOVE_NAMESPACE

extern ToveWarningFunction tove_warn_func;

void tove_warn(const char *file, int line, const char *s);
#define TOVE_WARN(s) tove::tove_warn(__FILE__, __LINE__, s)
#define TOVE_BAD_ALLOC() { TOVE_WARN("Out of memory"); throw std::bad_alloc(); }

END_TOVE_NAMESPACE

#endif // __TOVE_WARN
