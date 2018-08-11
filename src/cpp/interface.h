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

#ifndef __TOVE_INTERFACE
#define __TOVE_INTERFACE 1

#include <cstdint>

#include "interface/types.h"

#if TOVE_TARGET == TOVE_TARGET_LOVE2D

#if _MSC_VER
	#define EXPORT __declspec(dllexport)
#else
	#define EXPORT __attribute__ ((visibility ("default")))
#endif

extern "C" {
#include "interface/api.h"
}

#endif // TOVE_TARGET_LOVE2D

#endif // __TOVE_INTERFACE
